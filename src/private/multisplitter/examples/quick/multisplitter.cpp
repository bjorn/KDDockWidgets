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

#include "multisplitter.h"

#include <QQmlComponent>
#include <QQmlEngine>
#include <QDebug>
#include <QQuickItem>

using namespace Layouting;

namespace Layouting {

class QuickItem : public QQuickItem
                , public Layouting::Widget_quick
{
    Q_OBJECT
public:
    explicit QuickItem(const QString &filename, MultiSplitter *parent)
        : QQuickItem(parent)
        , Layouting::Widget_quick(this)
    {
        createQQuickItem(filename, this);
    }

    QSize minSize() const override
    {
        return QSize(150, 150);
    }

Q_SIGNALS:
    void layoutInvalidated();
};

}

MultiSplitter::MultiSplitter()
    : QQuickItem()
    , Layouting::Widget_quick(this)
    , m_rootContainer(new ItemContainer(this))
{
    onSizeChanged();
    connect(this, &QQuickItem::widthChanged, this, &MultiSplitter::onSizeChanged);
    connect(this, &QQuickItem::heightChanged, this, &MultiSplitter::onSizeChanged);
}

MultiSplitter::~MultiSplitter()
{
    delete m_rootContainer;
}

void MultiSplitter::addItem(const QString &filename, Layouting::Item::Location loc, QQuickItem *relativeTo)
{
    Layouting::Item *relativeToItem = m_rootContainer->itemForObject(relativeTo);
    addItem(filename, loc, relativeToItem);
}

void MultiSplitter::addItem(const QString &filename, Layouting::Item::Location loc, Layouting::Item *relativeTo)
{
    auto item = new Item(this);
    item->setGuestWidget(new QuickItem(filename, this));

    relativeTo = relativeTo ? relativeTo : m_rootContainer;
    relativeTo->insertItem(item, loc);
}

void MultiSplitter::onSizeChanged()
{
    m_rootContainer->setSize_recursive(QQuickItem::size().toSize().expandedTo(m_rootContainer->minSize()));
}


#include <multisplitter.moc>
