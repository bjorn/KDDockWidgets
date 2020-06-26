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
 * @brief The GUI counterpart of Frame. Inherits Frame and implements paintEvent().
 *
 * @author Sérgio Martins \<sergio.martins@kdab.com\>
 */

#include "FrameWidget_p.h"
#include "TitleBar_p.h"
#include "TabWidget_p.h"
#include "Config.h"
#include "FrameworkWidgetFactory.h"
#include "DockWidget.h"

#include <QVBoxLayout>
#include <QPainter>
#include <QTableWidget>
#include <QTabBar>

using namespace KDDockWidgets;

///@brieg a QVBoxLayout that emits layoutInvalidated so that Item can detect minSize changes
class VBoxLayout : public QVBoxLayout
{
public:
    explicit VBoxLayout(FrameWidget *parent)
        : QVBoxLayout(parent)
        , m_frameWidget(parent) {}
    ~VBoxLayout() override;

    void invalidate() override
    {
        QVBoxLayout::invalidate();
        Q_EMIT m_frameWidget->layoutInvalidated();
    }

    FrameWidget *const m_frameWidget;
};

VBoxLayout::~VBoxLayout() = default;

FrameWidget::FrameWidget(QWidget *parent, FrameOptions options)
    : QWidget(parent)
    , Layouting::Widget_qwidget(this)
    , Frame((Layouting::Widget_qwidget*)(this), options)
    , m_tabWidget(Config::self().frameworkWidgetFactory()->createTabWidget(this))
{
    auto vlayout = new VBoxLayout(this);
    vlayout->setContentsMargins(0, 0, 0, 0);
    vlayout->setSpacing(0);
    vlayout->addWidget(titleBar()->asQWidget());
    vlayout->addWidget(m_tabWidget->asWidget());

    m_tabWidget->setTabBarAutoHide(!alwaysShowsTabs());
}

FrameWidget::~FrameWidget()
{
    m_inDtor = true;
}

void FrameWidget::paintEvent(QPaintEvent *)
{
    if (!isFloating()) {
        QPainter p(this);
        QPen pen(QColor(184, 184, 184, 184));
        p.setPen(pen);
        p.drawRoundedRect(QWidget::rect().adjusted(0, 0, -1, -1), 2, 2);
    }
}

void FrameWidget::closeEvent(QCloseEvent *ev)
{
    Frame::onCloseEvent(ev);
}

QSize FrameWidget::maxSizeHint() const
{
    // waste due to QTabWidget margins, tabbar etc.
    const QSize waste = Layouting::Widget_qwidget::minSize() - dockWidgetsMinSize();
    return waste + biggestDockWidgetMaxSize();
}

void FrameWidget::detachTab_impl(DockWidgetBase *dw)
{
    m_tabWidget->detachTab(static_cast<DockWidget*>(dw));
}

int FrameWidget::indexOfDockWidget_impl(DockWidgetBase *dw)
{
    return m_tabWidget->indexOfDockWidget(static_cast<DockWidget*>(dw));
}

void FrameWidget::setCurrentDockWidget_impl(DockWidgetBase *dw)
{
    m_tabWidget->setCurrentDockWidget(static_cast<DockWidget*>(dw));
}

int FrameWidget::currentIndex_impl() const
{
    return m_tabWidget->currentIndex();
}

void FrameWidget::insertDockWidget_impl(DockWidgetBase *dw, int index)
{
    m_tabWidget->insertDockWidget(static_cast<DockWidget*>(dw), index);
}

void FrameWidget::removeWidget_impl(DockWidgetBase *dw)
{
    m_tabWidget->removeDockWidget(static_cast<DockWidget*>(dw));
}

void FrameWidget::setCurrentTabIndex_impl(int index)
{
    m_tabWidget->setCurrentDockWidget(index);
}

DockWidgetBase *FrameWidget::currentDockWidget_impl() const
{
    return m_tabWidget->dockwidgetAt(m_tabWidget->currentIndex());
}

DockWidgetBase *FrameWidget::dockWidgetAt_impl(int index) const
{
    return m_tabWidget->dockwidgetAt(index);
}

QTabBar *FrameWidget::tabBar() const
{
    auto tw = static_cast<QTabWidget*>(m_tabWidget->asWidget());
    return tw->tabBar();
}

TabWidget *FrameWidget::tabWidget() const
{
    return m_tabWidget;
}

int FrameWidget::dockWidgetCount_impl() const
{
    return m_tabWidget->numDockWidgets();
}

QRect FrameWidget::dragRect() const
{
    QRect rect = Frame::dragRect();
    if (rect.isValid())
        return rect;

    if (Config::self().flags() & Config::Flag_HideTitleBarWhenTabsVisible) {
        QTabBar *tabBar = this->tabBar();
        rect.setHeight(tabBar->height());
        rect.setWidth(QWidget::width() - tabBar->width());
        rect.moveTopLeft(QPoint(tabBar->width(), tabBar->y()));
        rect.moveTopLeft(QWidget::mapToGlobal(rect.topLeft()));
    }

    return rect;
}

