/*
  This file is part of KDDockWidgets.

  Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
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

#pragma once

#include "Widget.h"


///@file
///@brief A Layouting::Widget that just forwards the call to another Widget

namespace Layouting {

///@brief A Layouting::Widget that just forwards the call to another Widget
/// This is used to avoid diamond multiple-virtual-inheritance, which introduces
/// unneeded complexity, forces most-derived-classes to call the ctor of the
/// most-base class, etc.
///
/// Example usage
///     TitleBar derives from Widget_wrapper
///     TitleBarWidget derives from TitleBar and Widget_qwidget, and passes a
///     pointer to the Widget to TitleBar, which wraps it. So a call to
///     TitleBar::geometry() will end up calling TitleBarWidget::geometry() via
///     the Widget_qwidget base
class MULTISPLITTER_EXPORT Widget_wrapper : public Widget
{
public:
    explicit Widget_wrapper(Widget *widget);

    ~Widget_wrapper() override;

    QWidget* asQWidget() const override;
    QSize sizeHint() const override;
    QSize minSize() const override;
    QSize maxSizeHint() const override;
    QRect geometry() const override;
    void setGeometry(QRect) override;
    void setParent(Widget *) override;
    QDebug& dumpDebug(QDebug&) const override;
    bool isVisible() const override;
    void setVisible(bool) const override;
    std::unique_ptr<Widget> parentWidget() const override;
    std::unique_ptr<Widget> topLevel() const override;
    void setLayoutItem(Item *) override;
    void show() override;
    void hide() override;
    void close() override;
    void move(int x, int y) override;
    void setSize(int width, int height) override;
    void setWidth(int width) override;
    void setHeight(int height) override;
    void update() override;
    QPoint mapFromGlobal(QPoint p) const override;
    QPoint mapToGlobal(QPoint p) const override;
    void resize(QSize) override;
    void onCloseEvent(QCloseEvent *) override;
    bool eventFilter(QEvent*) override;

    QSize widgetMinSize(const QObject *) const override;
    QSize widgetMaxSize(const QObject *) const override;
    QString id() const override;

private:
    Widget *const m_widget;
    Q_DISABLE_COPY(Widget_wrapper)
};


}
