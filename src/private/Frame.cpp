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
 * @brief A DockWidget wrapper that adds a QTabWidget and a TitleBar.
 *
 * @author Sérgio Martins \<sergio.martins@kdab.com\>
 */

#include "Frame_p.h"
#include "DropArea_p.h"
#include "Logging_p.h"
#include "FloatingWindow_p.h"
#include "Utils_p.h"
#include "Position_p.h"
#include "DockRegistry_p.h"
#include "Config.h"
#include "TitleBar_p.h"
#include "FrameworkWidgetFactory.h"

#include <QCloseEvent>
#include <QTimer>

#define MARGIN_THRESHOLD 100

static int s_dbg_numFrames = 0;

using namespace KDDockWidgets;

namespace KDDockWidgets {
static FrameOptions actualOptions(FrameOptions options)
{
    if (Config::self().flags() & Config::Flag_AlwaysShowTabs)
        options |= FrameOption_AlwaysShowsTabs;

    return options;
}

void FrameSignalsAndSlots::updateTitleAndIcon()
{
    q->updateTitleAndIcon();
}

}

Frame::Frame(Layouting::Widget *thisWidget, FrameOptions options)
    : Layouting::Widget_wrapper(thisWidget)
    , s(this)
    , m_thisObj(asQObject())
    , m_titleBar(Config::self().frameworkWidgetFactory()->createTitleBar(this))
    , m_options(actualOptions(options))
{
    s_dbg_numFrames++;
    DockRegistry::self()->registerFrame(this);
    qCDebug(creation) << "Frame" << ((void*)this) << s_dbg_numFrames;
    m_inCtor = false;
}

Frame::~Frame()
{
    m_inDtor = true;
    s_dbg_numFrames--;
    if (m_layoutItem)
        m_layoutItem->unref();

    qCDebug(creation) << "~Frame" << static_cast<void*>(this);
    DockRegistry::self()->unregisterFrame(this);

    // Run some disconnects() too, so we don't receive signals during destruction:
    setDropArea(nullptr);
}

void Frame::updateTitleAndIcon()
{
    if (DockWidgetBase *dw = currentDockWidget()) {
        m_titleBar->setTitle(dw->title());
        m_titleBar->setIcon(dw->icon());

        if (auto fw = floatingWindow()) {
            if (fw->hasSingleFrame()) {
                fw->updateTitleAndIcon();
            }
        }

        setObjectName(dw->uniqueName());
    } else if (currentTabIndex() != -1) {
        qWarning() << Q_FUNC_INFO << "Invalid dock widget for frame." << currentTabIndex();
    }
}

void Frame::addWidget(DockWidgetBase *dockWidget, AddingOption addingOption)
{
    insertWidget(dockWidget, dockWidgetCount(), addingOption); // append
}

void Frame::addWidget(Frame *frame, AddingOption addingOption)
{
    if (frame->isEmpty()) {
        qWarning() << "Frame::addWidget: frame is empty." << frame;
        return;
    }

    const auto &docks = frame->dockWidgets();
    for (DockWidgetBase *dockWidget : docks)
        addWidget(dockWidget, addingOption);
}

void Frame::addWidget(FloatingWindow *floatingWindow, AddingOption addingOption)
{
    Q_ASSERT(floatingWindow);
    for (Frame *f : floatingWindow->frames())
        addWidget(f, addingOption);
}

void Frame::insertWidget(DockWidgetBase *dockWidget, int index, AddingOption addingOption)
{
    qCDebug(addwidget()) << Q_FUNC_INFO << ((void*)this) <<  "; dockWidget="
                         << dockWidget << "; oldFrame=" << dockWidget->frame()
                         << "; addingOption=" << addingOption;

    Q_ASSERT(dockWidget);
    if (contains(dockWidget)) {
        qWarning() << "Frame::addWidget dockWidget already exists. this=" << this << "; dockWidget=" << dockWidget;
        return;
    }
    if (m_layoutItem)
        dockWidget->addPlaceholderItem(m_layoutItem);

    insertDockWidget(dockWidget, index);

    if (addingOption == AddingOption_StartHidden) {
        dockWidget->close(); // Ensure closed
    } else {
        if (hasSingleDockWidget()) {
            emitCurrentDockWidgetChanged(dockWidget);
            setObjectName(dockWidget->uniqueName());

            if (!m_layoutItem) {
                // When adding the 1st dock widget of a fresh frame, let's give the frame the size
                // of the dock widget, so that when adding it to the main window, the main window can
                // use that size as the initial suggested size.
                resize(dockWidget->size());
            }
        }
    }

    QObject::connect(dockWidget->asQObject(), SIGNAL(titleChanged), &s, SLOT(updateTitleAndIcon));
    QObject::connect(dockWidget->asQObject(), SIGNAL(iconChanged), &s, SLOT(updateTitleAndIcon));
}

void Frame::removeWidget(DockWidgetBase *dw)
{
    QObject::disconnect(dw->asQObject(), SIGNAL(titleChanged), &s, SLOT(updateTitleAndIcon));
    QObject::disconnect(dw->asQObject(), SIGNAL(iconChanged), &s, SLOT(updateTitleAndIcon));
    removeWidget_impl(dw);
}

void Frame::detachTab(DockWidgetBase *dw)
{
    if (m_inCtor || m_inDtor) return;

    detachTab_impl(dw);
}

int Frame::indexOfDockWidget(DockWidgetBase *dw)
{
    if (m_inCtor || m_inDtor) return -1;

    return indexOfDockWidget_impl(dw);
}

int Frame::currentIndex() const
{
    if (m_inCtor || m_inDtor) return -1;

    return currentIndex_impl();
}

void Frame::setCurrentTabIndex(int index)
{
    if (m_inCtor || m_inDtor) return;

    setCurrentTabIndex_impl(index);
}

void Frame::setCurrentDockWidget(DockWidgetBase *dw)
{
    if (m_inCtor || m_inDtor) return;

    setCurrentDockWidget_impl(dw);
}

void Frame::insertDockWidget(DockWidgetBase *dw, int index)
{
    if (m_inCtor || m_inDtor) return;

    insertDockWidget_impl(dw, index);
}

DockWidgetBase *Frame::dockWidgetAt(int index) const
{
    if (m_inCtor || m_inDtor) return nullptr;

    return dockWidgetAt_impl(index);
}

DockWidgetBase *Frame::currentDockWidget() const
{
    if (m_inCtor || m_inDtor) return nullptr;

    return currentDockWidget_impl();
}

int Frame::dockWidgetCount() const
{
    if (m_inCtor || m_inDtor) return 0;

    return dockWidgetCount_impl();
}

void Frame::onDockWidgetCountChanged()
{
    qCDebug(docking) << "Frame::onDockWidgetCountChanged:" << this << "; widgetCount=" << dockWidgetCount();
    if (isEmpty() && !isCentralFrame()) {
        scheduleDeleteLater();
    } else {
        updateTitleBarVisibility();

        // We don't really keep track of the state, so emit even if the visibility didn't change. No biggie.
        if (!(m_options & FrameOption_AlwaysShowsTabs))
            Q_EMIT s.hasTabsVisibleChanged();
    }

    Q_EMIT s.numDockWidgetsChanged();
}

void Frame::onCurrentTabChanged(int index)
{
    if (index != -1) {
        if (auto dock = dockWidgetAt(index)) {
            emitCurrentDockWidgetChanged(dock);
        } else {
            qWarning() << "dockWidgetAt" << index << "returned nullptr" << this;
        }
    }
}

void Frame::updateTitleBarVisibility()
{
    if (m_updatingTitleBar || m_beingDeleted) {
        // To break a cyclic dependency
        return;
    }

    QScopedValueRollback<bool> guard(m_updatingTitleBar, true);

    bool visible = false;
    if (isCentralFrame()) {
        visible = false;
    } else if ((Config::self().flags() & Config::Flag_HideTitleBarWhenTabsVisible) && hasTabsVisible()) {
        visible = false;
    } else if (FloatingWindow *fw = floatingWindow()) {
        // If there's nested frames then show each Frame's title bar
        visible = !fw->hasSingleFrame();
    } else {
        visible = true;
    }

    m_titleBar->setVisible(visible);
    if (auto fw = floatingWindow()) {
        // Update the floating window which might be using Flag_HideTitleBarWhenTabsVisible
        // In that case it might not show title bar depending on the number of tabs that the frame has
        fw->updateTitleBarVisibility();
    }
}

bool Frame::containsMouse(QPoint globalPos) const
{
    return rect().contains(mapFromGlobal(globalPos));
}

TitleBar *Frame::titleBar() const
{
    return m_titleBar;
}

TitleBar *Frame::actualTitleBar() const
{
    if (FloatingWindow *fw = floatingWindow()) {
        // If there's nested frames then show each Frame's title bar
        if (fw->hasSingleFrame())
            return fw->titleBar();
    }

    return titleBar();
}

QString Frame::title() const
{
    return m_titleBar->title();
}

QIcon Frame::icon() const
{
    return m_titleBar->icon();
}

const DockWidgetBase::List Frame::dockWidgets() const
{
    if (m_inCtor || m_inDtor)
        return {};

    DockWidgetBase::List dockWidgets;
    const int count = dockWidgetCount();
    dockWidgets.reserve(count);
    for (int i = 0, e = count; i != e; ++i) {
        dockWidgets << dockWidgetAt(i);
    }

    return dockWidgets;
}

bool Frame::contains(DockWidgetBase *dockWidget) const
{
    const int count = dockWidgetCount();
    for (int i = 0, e = count; i != e; ++i) {
        if (dockWidget == dockWidgetAt(i))
            return true;
    }
    return false;
}

FloatingWindow *Frame::floatingWindow() const
{
    // Returns the first FloatingWindow* parent in the hierarchy.
    // However, if there's a MainWindow in the hierarchy it stops, which can
    // happen with nested main windows.

    auto p = parent();
    while (p) {
        if (qobject_cast<KDDockWidgets::MainWindowBase*>(p))
            return nullptr;

        if (auto fw = qobject_cast<FloatingWindow*>(p))
            return fw;

        if (p == topLevel()->asQObject()) {
            // We stop at the window. (top-levels can have parent, but we're not interested)
            return nullptr;
        }

        p = p->parent();
    }

    return nullptr;
}

void Frame::restoreToPreviousPosition()
{
    if (hasSingleDockWidget()) {
        qWarning() << Q_FUNC_INFO << "Invalid usage, there's no tabs";
        return;
    }

    if (!m_layoutItem) {
        qCDebug(placeholder) << Q_FUNC_INFO << "There's no previous position known";
        return;
    }

    if (!m_layoutItem->isPlaceholder()) {
        // Maybe in this case just fold the frame into the placeholder, which probably has other dockwidgets which were added meanwhile. TODO
        qCDebug(placeholder) << Q_FUNC_INFO << "Previous position isn't a placeholder";
        return;
    }

    m_layoutItem->restore(this);
}

int Frame::currentTabIndex() const
{
    return currentIndex();
}

void Frame::onCloseEvent(QCloseEvent *e)
{
    qCDebug(closing) << "Frame::closeEvent";
    e->accept(); // Accepted by default (will close unless ignored)
    const DockWidgetBase::List docks = dockWidgets();
    for (DockWidgetBase *dock : docks) {
        qApp->sendEvent(dock->asQObject(), e);
        if (!e->isAccepted())
            break; // Stop when the first dockwidget prevents closing
    }
}

bool Frame::anyNonClosable() const
{
    for (auto dw : dockWidgets()) {
        if ((dw->options() & DockWidgetBase::Option_NotClosable) && !DockRegistry::self()->isProcessingAppQuitEvent())
            return true;
    }

    return false;
}

bool Frame::anyNonDockable() const
{
    for (auto dw : dockWidgets()) {
        if (dw->options() & DockWidgetBase::Option_NotDockable)
            return true;
    }

    return false;
}

void Frame::onDockWidgetShown(DockWidgetBase *w)
{
    if (hasSingleDockWidget() && contains(w)) { // We have to call contains because it might be being in process of being reparented
        if (!isVisible()) {
            qCDebug(hiding) << "Widget" << w << " was shown, we're=" << "; visible="
                            << isVisible();
            setVisible(true);
        }
    }
}

void Frame::onDockWidgetHidden(DockWidgetBase *w)
{
    if (hasSingleDockWidget() && contains(w)) { // We have to call contains because it might be being in process of being reparented
        if (isVisible()) {
            qCDebug(hiding) << "Widget" << w << " was hidden, we're="
                            << "; visible=" << isVisible()
                            << "; dockWidgets=" << dockWidgets();
            setVisible(false);
        }
    }
}

void Frame::setLayoutItem(Layouting::Item *item)
{
    if (item == m_layoutItem)
        return;

    if (m_layoutItem)
        m_layoutItem->unref();

    if (item)
        item->ref();

    m_layoutItem = item;
    if (item) {
        for (DockWidgetBase *dw : dockWidgets())
            dw->addPlaceholderItem(item);
    } else {
        for (DockWidgetBase *dw : dockWidgets())
            dw->lastPositions().removePlaceholders();
    }
}

Layouting::Item *Frame::layoutItem() const
{
    return m_layoutItem;
}

int Frame::dbg_numFrames()
{
    return s_dbg_numFrames;
}

bool Frame::beingDeletedLater() const
{
    return m_beingDeleted;
}

bool Frame::hasTabsVisible() const
{
    if (m_beingDeleted)
        return false;

    return alwaysShowsTabs() || dockWidgetCount() > 1;
}

QStringList Frame::affinities() const
{
    if (isEmpty()) {
        return {};
    } else {
        return dockWidgetAt(0)->affinities();
    }
}

void Frame::setDropArea(DropArea *dt)
{
    if (dt != m_dropArea) {
        qCDebug(docking) << "Frame::setDropArea dt=" << dt;
        const bool wasInMainWindow = dt && isInMainWindow();
        if (m_dropArea)
            m_thisObj->disconnect(m_visibleWidgetCountChangedConnection);

        m_dropArea = dt;

        if (m_dropArea) {
            // We keep the connect result so we don't dereference m_dropArea at shutdown
            m_visibleWidgetCountChangedConnection = QObject::connect(m_dropArea, &MultiSplitter::visibleWidgetCountChanged,
                                                                     m_thisObj, [this] { updateTitleBarVisibility(); });
            updateTitleBarVisibility();
            if (wasInMainWindow != isInMainWindow())
                Q_EMIT s.isInMainWindowChanged();
        }
    }
}

bool Frame::isTheOnlyFrame() const
{
    qCDebug(docking) << "Frame::isTheOnlyFrame() m_dropArea=" << m_dropArea << "; numFrames"
                     << (m_dropArea ? m_dropArea->numFrames() : 0);
    return m_dropArea && m_dropArea->numFrames() == 1;
}

bool Frame::isFloating() const
{
    if (isInMainWindow())
        return false;

    return isTheOnlyFrame();
}

bool Frame::isInFloatingWindow() const
{
    return floatingWindow() != nullptr;
}

bool Frame::isInMainWindow() const
{
    return m_dropArea && m_dropArea->isInMainWindow();
}

bool Frame::eventFilter(QEvent *e)
{
    if (e->type() == QEvent::ParentChange) {
        qCDebug(docking) << "Frame: parent changed to =" << parent();
        if (auto dropArea = qobject_cast<DropArea *>(parent())) {
            setDropArea(dropArea);
        } else {
            Q_ASSERT(!parent());
            setDropArea(nullptr);
        }
    }

    return false;
}

Frame *Frame::deserialize(const LayoutSaver::Frame &f)
{
    if (!f.isValid())
        return nullptr;

    auto frame = Config::self().frameworkWidgetFactory()->createFrame(/*parent=*/nullptr, FrameOptions(f.options));
    frame->setObjectName(f.objectName);

    for (const auto &savedDock : qAsConst(f.dockWidgets)) {
        if (DockWidgetBase *dw = DockWidgetBase::deserialize(savedDock)) {
            frame->addWidget(dw);
        }
    }

    frame->setCurrentTabIndex(f.currentTabIndex);
    frame->setGeometry(f.geometry);

    return frame;
}

LayoutSaver::Frame Frame::serialize() const
{
    LayoutSaver::Frame frame;
    frame.isNull = false;

    const DockWidgetBase::List docks = dockWidgets();

    frame.objectName = objectName();
    frame.geometry = geometry();
    frame.options = options();
    frame.currentTabIndex = currentTabIndex();
    frame.id = id(); // for coorelation purposes

    for (DockWidgetBase *dock : docks)
        frame.dockWidgets.push_back(dock->serialize());

    return frame;
}

void Frame::scheduleDeleteLater()
{
    qCDebug(creation) << Q_FUNC_INFO << this;
    m_beingDeleted = true;
    QTimer::singleShot(0, m_thisObj, [this] {
        // Can't use deleteLater() here due to QTBUG-83030 (deleteLater() never delivered if triggered by a sendEvent() before event loop starts)
        delete this;
    });
}

QSize Frame::dockWidgetsMinSize() const
{
    QSize size = Layouting::Item::hardcodedMinimumSize;
    for (DockWidgetBase *dw : dockWidgets())
        size = size.expandedTo(dw->widgetMinSize(dw->asQObject()));

    return size;
}

QSize Frame::biggestDockWidgetMaxSize() const
{
    QSize size = Layouting::Item::hardcodedMaximumSize;
    for (DockWidgetBase *dw : dockWidgets()) {
        const QSize dwMax = dw->widgetMaxSize(dw->asQObject());
        if (size == Layouting::Item::hardcodedMaximumSize) {
            size = dwMax;
            continue;
        }

        const bool hasMaxSize = dwMax != Layouting::Item::hardcodedMaximumSize;
        if (hasMaxSize)
            size = dwMax.expandedTo(size);
    }

    // Interpret 0 max-size as not having one too.
    if (size.width() == 0)
        size.setWidth(Layouting::Item::hardcodedMaximumSize.width());
    if (size.height() == 0)
        size.setHeight(Layouting::Item::hardcodedMaximumSize.height());

    return size;
}

QRect Frame::dragRect() const
{
    QRect rect;
    if (m_titleBar->isVisible()) {
        rect = m_titleBar->rect();
        rect.moveTopLeft(m_titleBar->mapToGlobal(QPoint(0, 0)));
    }

    return rect;
}

void Frame::emitCurrentDockWidgetChanged(DockWidgetBase *dw)
{
    Q_EMIT s.currentDockWidgetChanged(dw);
    updateTitleAndIcon();
}
