/*
  This file is part of KDDockWidgets.

  Copyright (C) 2018-2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file
 * @brief Implements a QTabWidget derived class with support for docking and undocking
 * KDockWidget::DockWidget as tabs .
 *
 * @author Sérgio Martins \<sergio.martins@kdab.com\>
 */

#include "TabWidget_p.h"
#include "DragController_p.h"
#include "FloatingWindow_p.h"
#include "Frame_p.h"
#include "WindowBeingDragged_p.h"
#include "Logging_p.h"
#include "Utils_p.h"
#include "Config.h"
#include "FrameworkWidgetFactory.h"

#ifdef QT_WIDGETS_LIB
# include <QTabWidget>
#endif

#include <memory>

using namespace KDDockWidgets;

TabBar::TabBar(QWidgetOrQuick *thisWidget, TabWidget *tabWidget)
    : Draggable(thisWidget)
    , m_tabWidget(tabWidget)
    , m_thisWidget(thisWidget)
{
}

DockWidget *TabBar::dockWidgetAt(int index) const
{
    if (index < 0 || index >= numDockWidgets())
        return nullptr;

    return m_tabWidget->dockwidgetAt(index);
}

DockWidget *TabBar::dockWidgetAt(QPoint localPos) const
{
    return dockWidgetAt(tabAt(localPos));
}

std::unique_ptr<WindowBeingDragged> TabBar::makeWindow()
{
    auto dock = m_lastPressedDockWidget;
    m_lastPressedDockWidget = nullptr; // TODO check if we still have this dock, it might have been deleted

    const bool hideTitleBarWhenTabsVisible = Config::self().flags() & Config::Flag_HideTitleBarWhenTabsVisible;
    const bool alwaysShowTabs = Config::self().flags() & Config::Flag_AlwaysShowTabs;

    if (hideTitleBarWhenTabsVisible) {
        if (dock) {
            if (alwaysShowTabs && hasSingleDockWidget()) {
                // Case #1. User is dragging a tab but there's only 1 tab (and tabs are always visible), so drag everything instead, no detaching happens
               return m_tabWidget->makeWindow();
            }
        } else {
            // Case #2. User is dragging on the QTabBar background, not on an actual tab.
            // As Flag_HideTitleBarWhenTabsVisible is set, we let the user drag through the tab widget background.
            return m_tabWidget->makeWindow();
        }
    } else {
        if (dock && hasSingleDockWidget() && alwaysShowTabs) {
            // Case #3. window with title bar and single tab, no detaching should happen, just use the title bar.
            return {};
        }
    }

    if (!dock)
        return {};

    FloatingWindow *floatingWindow = detachTab(dock);

    auto draggable = KDDockWidgets::usesNativeTitleBar() ? static_cast<Draggable*>(floatingWindow)
                                                         : static_cast<Draggable*>(this);
    return std::unique_ptr<WindowBeingDragged>(new WindowBeingDragged(floatingWindow, draggable));
}

FloatingWindow * TabBar::detachTab(DockWidget *dockWidget)
{
    QRect r = dockWidget->QWidget::geometry();
    m_tabWidget->removeDockWidget(dockWidget);

    auto newFrame = Config::self().frameworkWidgetFactory()->createFrame();
    const QPoint globalPoint = m_thisWidget->mapToGlobal(QPoint(0, 0));
    newFrame->addWidget(dockWidget);

    // We're potentially already dead at this point, as frames with 0 tabs auto-destruct. Don't access members from this point.

    auto floatingWindow = Config::self().frameworkWidgetFactory()->createFloatingWindow(newFrame);
    r.moveTopLeft(globalPoint);
    floatingWindow->setSuggestedGeometry(r);
    floatingWindow->show();

    return floatingWindow;
}

void TabBar::onMousePress(QPoint localPos)
{
    m_lastPressedDockWidget = dockWidgetAt(localPos);
}

bool TabBar::hasSingleDockWidget() const
{
    return numDockWidgets() == 1;
}

QWidgetOrQuick *TabBar::asWidget() const
{
    return m_thisWidget;
}

DockWidget *TabBar::singleDockWidget() const
{
    return m_tabWidget->singleDockWidget();
}

TabWidget::TabWidget(QWidgetOrQuick *thisWidget, Frame *frame)
    : Draggable(thisWidget, Config::self().flags() & (Config::Flag_HideTitleBarWhenTabsVisible | Config::Flag_AlwaysShowTabs))
    , m_frame(frame)
    , m_thisWidget(thisWidget)
{

#ifdef KDDOCKWIDGETS_QTWIDGETS
    // Little ifdefery, as this is not so easy to abstract
    QObject::connect(static_cast<QTabWidget*>(thisWidget), &QTabWidget::currentChanged,
                     frame->asQObject(), [frame] (int index) { frame->onCurrentTabChanged(index); });
#else
    qWarning() << Q_FUNC_INFO << "Implement me";
#endif

}

void TabWidget::setCurrentDockWidget(DockWidget *dw)
{
    setCurrentDockWidget(indexOfDockWidget(dw));
}

void TabWidget::addDockWidget(DockWidget *dock)
{
    insertDockWidget(dock, numDockWidgets());
}

void TabWidget::insertDockWidget(DockWidget *dock, int index)
{
    Q_ASSERT(dock);
    qCDebug(addwidget) << Q_FUNC_INFO << dock << "; count before=" << numDockWidgets();

    if (index < 0)
        index = 0;
    if (index > numDockWidgets())
        index = numDockWidgets();

    if (contains(dock)) {
        qWarning() << Q_FUNC_INFO << "Refusing to add already existing widget";
        return;
    }

    Frame *oldFrame = dock->frame();
    QPointer<QObject> stillValid = oldFrame ? oldFrame->asQObject()
                                            : nullptr;
    insertDockWidget(index, dock, dock->icon(), dock->title());
    setCurrentDockWidget(index);

    if (stillValid && oldFrame->beingDeletedLater()) {
        // give it a push and delete it immediately.
        // Having too many deleteLater() puts us in an inconsistent state. For example if LayoutSaver::saveState()
        // would to be called while the Frame hadn't been deleted yet it would count with that frame unless hacks.
        // Also the unit-tests are full of waitForDeleted() due to deleteLater.

        // Ideally we would just remove the deleteLater from frame.cpp, but QTabWidget::insertTab()
        // would crash, as it accesses the old tab-widget we're stealing from

        delete oldFrame;
    }
}

bool TabWidget::contains(DockWidget *dw) const
{
    return indexOfDockWidget(dw) != -1;
}

QWidgetOrQuick *TabWidget::asWidget() const
{
    return m_thisWidget;
}

Frame *TabWidget::frame() const
{
    return m_frame;
}

std::unique_ptr<WindowBeingDragged> TabWidget::makeWindow()
{
    // This is called when using Flag_HideTitleBarWhenTabsVisible
    // For detaching individual tabs, TabBar::makeWindow() is called.
    if (auto floatingWindow = qobject_cast<FloatingWindow*>(asWidget()->window())) {

        if (floatingWindow->hasSingleFrame()) {
            // We're already in a floating window, and it only has 1 dock widget.
            // So there's no detachment to be made, we just move the window.
            return std::unique_ptr<WindowBeingDragged>(new WindowBeingDragged(floatingWindow, this));
        }
    }

    QRect r = m_frame->geometry();

    const QPoint globalPoint = m_thisWidget->mapToGlobal(QPoint(0, 0));

    auto floatingWindow = Config::self().frameworkWidgetFactory()->createFloatingWindow(m_frame);
    r.moveTopLeft(globalPoint);
    floatingWindow->setSuggestedGeometry(r);
    floatingWindow->show();

    return std::unique_ptr<WindowBeingDragged>(new WindowBeingDragged(floatingWindow, this));
}

DockWidget *TabWidget::singleDockWidget() const
{
    if (m_frame->hasSingleDockWidget())
        return static_cast<DockWidget*>(m_frame->dockWidgets().first());

    return nullptr;
}

void TabWidget::onTabInserted()
{
    m_frame->onDockWidgetCountChanged();
}

void TabWidget::onTabRemoved()
{
    m_frame->onDockWidgetCountChanged();
}
