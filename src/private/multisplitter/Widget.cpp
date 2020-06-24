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

#include "Widget.h"
#include "Item_p.h"

#include <QObject>

using namespace Layouting;


static qint64 s_nextFrameId = 1;

namespace Layouting {
///@brief helper class to use an event filter, as Widget is not a QObject
class EventFilter : public QObject {
public:
    EventFilter(Widget *q)
        : QObject(q->asQObject())
        , q(q)
    {
        q->asQObject()->installEventFilter(this);
    }

    bool eventFilter(QObject *, QEvent *event) override
    {
        return q->eventFilter(event);
    }

    Widget *const q;
};
}

Widget::Widget(QObject *thisObj)
    : m_id(QString::number(s_nextFrameId++))
    , m_thisObj(thisObj)
    , m_eventFilter(new EventFilter(this))
{
}

Widget::~Widget()
{
    delete m_eventFilter;
}

QString Widget::id() const
{
    return m_id;
}

QString Widget::objectName() const
{
    return m_thisObj->objectName();
}

void Widget::setObjectName(const QString &name)
{
    m_thisObj->setObjectName(name);
}

void Widget::move(QPoint p)
{
    move(p.x(), p.y());
}

void Widget::resize(int width, int height)
{
    resize(QSize(width, height));
}

bool Widget::isWindow() const
{
    return m_thisObj == topLevel()->asQObject();
}

int Widget::width() const
{
    return size().width();
}

int Widget::height() const
{
    return size().height();
}

QSize Widget::boundedMaxSize(QSize min, QSize max)
{
    // Max should be bigger than min, but not bigger than the hardcoded max
    max = max.boundedTo(QSize(KDDOCKWIDGETS_MAX_WIDTH, KDDOCKWIDGETS_MAX_HEIGHT));

    // 0 interpreted as not having max
    if (max.width() <= 0)
        max.setWidth(KDDOCKWIDGETS_MAX_WIDTH);
    if (max.height() <= 0)
        max.setHeight(KDDOCKWIDGETS_MAX_HEIGHT);

    max = max.expandedTo(min);

    return max;
}
