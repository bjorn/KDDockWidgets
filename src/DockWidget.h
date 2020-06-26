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
 * @brief Represents a dock widget.
 *
 * @author Sérgio Martins \<sergio.martins@kdab.com\>
 */

#ifndef KD_DOCKWIDGET_H
#define KD_DOCKWIDGET_H

#include "DockWidgetBase.h"
#include "../multisplitter/Widget_qwidget.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QCloseEvent;
QT_END_NAMESPACE

namespace KDDockWidgets {

/**
 * @brief Represents a dock widget.
 *
 * Most of the interface lives in DockWidgetBase, to facilitate sharing with QtQuick.
 */
class DOCKS_EXPORT DockWidget
        : public QWidget
        , public Layouting::Widget_qwidget
        , public DockWidgetBase
{
    Q_OBJECT
public:
    /**
     * @brief constructs a new DockWidget
     * @param uniqueName Mandatory name that should be unique between all DockWidget instances.
     *        This name won't be user visible and just used internally for the save/restore.
     *        Use setTitle() for user visible text.
     * @param options optional options controlling behaviour
     *
     * There's no parent argument. The DockWidget is either parented to FloatingWindow or MainWindow
     * when visible, or stays without a parent when hidden. This allows to support docking
     * to different main windows.
     */
    explicit DockWidget(const QString &uniqueName, Options options = DockWidgetBase::Options());

    ///@brief destructor
    ~DockWidget() override;

Q_SIGNALS:
    ///@brief signal emitted when the parent changed
    void parentChanged();

    ///@brief signal emitted when the DockWidget is shown. As in QEvent::Show.
    void shown();

    ///@brief signal emitted when the DockWidget is hidden. As in QEvent::Hide.
    void hidden();

    ///@brief signal emitted when the icon changed
    void iconChanged();

    ///@brief signal emitted when the title changed
    void titleChanged();

    ///@brief emitted when the hosted widget changed
    void widgetChanged(QWidget*);

    ///@brief emitted when the options change
    ///@sa setOptions(), options()
    void optionsChanged(KDDockWidgets::DockWidgetBase::Options);

protected:
    bool event(QEvent *) override;
    void closeEvent(QCloseEvent *) override;

private:
    class Private;
    Private *const d;
};

}

#endif
