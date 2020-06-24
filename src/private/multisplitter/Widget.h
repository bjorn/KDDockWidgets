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

///@file
///@brief An abstraction/wrapper around QWidget, QtQuickItem or anything else

#pragma once

#include "multisplitter_export.h"

#include <QRect>
#include <QSize>
#include <QDebug>
#include <QObject>
#include <qglobal.h>
#include <QSizePolicy>

#include <memory>

QT_BEGIN_NAMESPACE
class QWidget;
class QCloseEvent;
class QSizePolicy;
QT_END_NAMESPACE

namespace Layouting {

class Item;

/**
 * @brief An abstraction/wrapper around QWidget, QtQuickItem or anything else
 *
 * So the layout can host QWidget, etc without depending on it.
 *
 * Inherit from it via multi-inheritance so this wrapper is deleted when the actual QWidget/QQuickItem
 * is deleted.
 */
class MULTISPLITTER_EXPORT Widget
{
public:
    explicit Widget(QObject *thisObj);
    virtual ~Widget();

    virtual void setLayoutItem(Item *) = 0;

    // Not strickly necessary, but it's nice conveniance for kddw which is widget based.
    virtual QWidget *asQWidget() const { return nullptr; }

    virtual QSize sizeHint() const { return {}; }
    virtual QSize minSize() const = 0;
    virtual QSize maxSizeHint() const = 0;
    virtual QRect geometry() const = 0;
    virtual void setGeometry(QRect) = 0;
    virtual void setParent(Widget *) = 0;
    virtual QDebug& dumpDebug(QDebug&) const = 0;
    virtual bool isVisible() const = 0;
    virtual void setVisible(bool) const = 0;
    virtual void move(int x, int y) = 0;
    virtual void setSize(int width, int height) = 0;
    virtual void resize(QSize) = 0;
    virtual void setWidth(int width) = 0;
    virtual void setHeight(int height) = 0;
    virtual std::unique_ptr<Widget> parentWidget() const = 0;
    virtual std::unique_ptr<Widget> topLevel() const = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual bool close() = 0;
    virtual void update() = 0;
    virtual QPoint mapFromGlobal(QPoint) const = 0;
    virtual QPoint mapToGlobal(QPoint) const = 0;
    QString objectName() const;
    void setObjectName(const QString &);
    virtual QSize widgetMinSize(const QObject *) const = 0; // TODO: Change to Widget
    virtual QSize widgetMaxSize(const QObject *) const = 0; // TODO: Change to Widget
    virtual void setWindowTitle(const QString &) = 0;
    virtual void setSizePolicy(QSizePolicy) = 0; // TODO: Get our own struct ?

    void move(QPoint);
    void resize(int width, int height);
    QSize size() const {
        return geometry().size();
    }

    QRect rect() const {
        return QRect(QPoint(0, 0), size());
    }

    int width() const;
    int height() const;
    bool isWindow() const;

    QObject *asQObject() const { return m_thisObj; }
    QObject *parent() const { return m_thisObj->parent(); }

    int x() const {
        return geometry().x();
    }

    int y() const {
        return geometry().y();
    }

    QPoint pos() const { return geometry().topLeft(); }

    virtual void onCloseEvent(QCloseEvent *) {};
    virtual bool eventFilter(QEvent*) { return false; }

    bool operator==(const QObject *obj) const {
        return obj == m_thisObj;
    }

    ///@brief returns an id for corelation purposes for saving layouts
    virtual QString id() const;

protected:
    static QSize boundedMaxSize(QSize min, QSize max);

private:
    const QString m_id;
    QObject *const m_thisObj;
    QObject *const m_eventFilter;
    Q_DISABLE_COPY(Widget)
};

inline bool operator==(const Widget *w, const QObject &obj)
{
    //if (!w && !obj)
        //return true;

    return w && w->operator==(&obj);
}

template <typename T>
struct WidgetQPointer
{
    WidgetQPointer() = default;

    WidgetQPointer(T *w)
        : widget(w)
    {
        if (w) {
            w->asQObject()->connect(w->asQObject(), &QObject::destroyed, w->asQObject(), [this] {
                widget = nullptr;
            });
        }
    }

    WidgetQPointer<T> &operator=(T *w) { widget = w; return *this; }
    T* operator->() const { return widget; }
    T* data() const { return widget; }
    operator T*() const { return widget; }
    T& operator*() const { return *widget; }

    operator bool() const { return widget != nullptr; }

    T *widget = nullptr;
};


}
