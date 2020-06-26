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

#include "DropArea_p.h"
#include "Logging_p.h"
#include "DockWidget.h" // TODO: DockWidgetBase instead
#include "Draggable_p.h"
#include "FloatingWindow_p.h"
#include "Config.h"
#include "DropIndicatorOverlayInterface_p.h"
#include "FrameworkWidgetFactory.h"
#include "MainWindowBase.h"
#include "multisplitter/Item_p.h"
#include "DockRegistry_p.h"
#include "widgets/FrameWidget_p.h"

// #include "indicators/AnimatedIndicators_p.h"
#include "WindowBeingDragged_p.h"

using namespace KDDockWidgets;

/**
 * @file
 * @brief A MultiSplitter with support for drop indicators when hovering over.
 *
 * @author Sérgio Martins \<sergio.martins@kdab.com\>
 */
DropArea::DropArea(QWidgetOrQuick *parent)
    : MultiSplitter(parent)
    , m_dropIndicatorOverlay(Config::self().frameworkWidgetFactory()->createDropIndicatorOverlay(this))
{
    qCDebug(creation) << "DropArea";
}

DropArea::~DropArea()
{
    m_inDestructor = true;
    qCDebug(creation) << "~DropArea";
}

int DropArea::numFrames() const
{
    return visibleCount();
}

Frame::List DropArea::frames() const
{
    const auto frameWidgets = findChildren<FrameWidget *>(QString(), Qt::FindDirectChildrenOnly);

    Frame::List frames;
    frames.reserve(frameWidgets.size());
    for (FrameWidget *f : frameWidgets)
        frames.push_back(f);

    return frames;
}

Frame *DropArea::frameContainingPos(QPoint globalPos) const
{
    const Layouting::Item::List &items = this->items();
    for (Layouting::Item *item : items) {
        auto frame = dynamic_cast<Frame*>(item->guestAsQObject());
        if (!frame || !frame->isVisible()) {
            continue;
        }

        if (frame->containsMouse(globalPos))
            return frame;
    }
    return nullptr;
}

Layouting::Item *DropArea::centralFrame() const
{
    for (Layouting::Item *item : this->items()) {
        if (auto f = dynamic_cast<Frame*>(item->guestAsQObject())) {
            if (f->isCentralFrame())
                return item;
        }
    }
    return nullptr;
}

void DropArea::addDockWidget(DockWidgetBase *dw, Location location, DockWidgetBase *relativeTo, AddingOption option)
{
    if (!dw || dw == relativeTo || location == Location_None) {
        qWarning() << Q_FUNC_INFO << "Invalid parameters" << dw << relativeTo << location;
        return;
    }

    if ((option & AddingOption_StartHidden) && dw->frame() != nullptr) {
        // StartHidden is just to be used at startup, not to moving stuff around
        qWarning() << Q_FUNC_INFO << "Dock widget already exists in the layout";
        return;
    }

    if (!validateAffinity(dw))
        return;

    Frame *frame = nullptr;
    Frame *relativeToFrame = relativeTo ? relativeTo->frame() : nullptr;

    dw->saveLastFloatingGeometry();

    // Check if the dock widget already exists in the layout
    if (contains(dw)) {
        Frame *oldFrame = dw->frame();
        if (oldFrame->hasSingleDockWidget()) {
            Q_ASSERT(oldFrame->contains(dw));
            // The frame only has this dock widget, and the frame is already in the layout. So move the frame instead
            frame = oldFrame;
        } else {
            frame = Config::self().frameworkWidgetFactory()->createFrame();
            frame->addWidget(dw);
        }
    } else {
        frame = Config::self().frameworkWidgetFactory()->createFrame();
        frame->addWidget(dw);
    }

    if (option & AddingOption_StartHidden) {
        addWidget(dw->asQWidget(), location, relativeToFrame, DefaultSizeMode::Fair, option);
    } else {
        addWidget(frame->asQWidget(), location, relativeToFrame, DefaultSizeMode::Fair, option);
    }
}

bool DropArea::contains(DockWidgetBase *dw) const
{
    return dw->frame() && MultiSplitter::contains(dw->frame());
}

QStringList DropArea::affinities() const
{
    if (auto mw = mainWindow()) {
        return mw->affinities();
    } else if (auto fw = floatingWindow()) {
        return fw->affinities();
    }

    return {};
}

void DropArea::layoutParentContainerEqually(DockWidgetBase *dw)
{
    Layouting::Item *item = itemForFrame(dw->frame());
    if (!item) {
        qWarning() << Q_FUNC_INFO << "Item not found for" << dw << dw->frame();
        return;
    }

    layoutEqually(item->parentContainer());
}

void DropArea::hover(FloatingWindow *floatingWindow, QPoint globalPos)
{
    if (!validateAffinity(floatingWindow))
        return;

    Frame *frame = frameContainingPos(globalPos); // Frame is nullptr if MainWindowOption_HasCentralFrame isn't set
    m_dropIndicatorOverlay->setWindowBeingDragged(floatingWindow);
    m_dropIndicatorOverlay->setHoveredFrame(frame);
    m_dropIndicatorOverlay->hover(globalPos);
}

static bool isOutterLocation(DropIndicatorOverlayInterface::DropLocation location)
{
    switch (location) {
    case DropIndicatorOverlayInterface::DropLocation_OutterLeft:
    case DropIndicatorOverlayInterface::DropLocation_OutterTop:
    case DropIndicatorOverlayInterface::DropLocation_OutterRight:
    case DropIndicatorOverlayInterface::DropLocation_OutterBottom:
        return true;
    default:
        return false;
    }
}

bool DropArea::drop(FloatingWindow *droppedWindow, QPoint globalPos)
{
    if (droppedWindow == window()) {
        qWarning() << "Refusing to drop onto itself"; // Doesn't happen
        return false;
    }

    if (m_dropIndicatorOverlay->currentDropLocation() == DropIndicatorOverlayInterface::DropLocation_None) {
        qCDebug(hovering) << "DropArea::drop: bailing out, drop location = none";
        return false;
    }

    qCDebug(dropping) << "DropArea::drop:" << droppedWindow;

    hover(droppedWindow, globalPos);
    Frame *acceptingFrame = m_dropIndicatorOverlay->hoveredFrame();
    if (!(acceptingFrame || isOutterLocation(m_dropIndicatorOverlay->currentDropLocation()))) {
        qWarning() << "DropArea::drop: asserted with frame=" << acceptingFrame << "; Location=" << m_dropIndicatorOverlay->currentDropLocation();
        return false;
    }

    bool result = true;

    auto droploc = m_dropIndicatorOverlay->currentDropLocation();
    switch (droploc) {
    case DropIndicatorOverlayInterface::DropLocation_Left:
    case DropIndicatorOverlayInterface::DropLocation_Top:
    case DropIndicatorOverlayInterface::DropLocation_Bottom:
    case DropIndicatorOverlayInterface::DropLocation_Right:
        result = drop(droppedWindow, DropIndicatorOverlayInterface::multisplitterLocationFor(droploc), acceptingFrame);
        break;
    case DropIndicatorOverlayInterface::DropLocation_OutterLeft:
    case DropIndicatorOverlayInterface::DropLocation_OutterTop:
    case DropIndicatorOverlayInterface::DropLocation_OutterRight:
    case DropIndicatorOverlayInterface::DropLocation_OutterBottom:
        result = drop(droppedWindow, DropIndicatorOverlayInterface::multisplitterLocationFor(droploc), nullptr);
        break;
    case DropIndicatorOverlayInterface::DropLocation_Center:
        qCDebug(hovering) << "Tabbing" << droppedWindow << "into" << acceptingFrame;
        if (!validateAffinity(droppedWindow))
            return false;
        acceptingFrame->addWidget(droppedWindow);
        break;

    default:
        qWarning() << "DropArea::drop: Unexpected drop location" << m_dropIndicatorOverlay->currentDropLocation();
        result = false;
        break;
    }

    if (result)
        raiseAndActivate();

    return result;
}

bool DropArea::drop(QWidgetOrQuick *droppedWindow, KDDockWidgets::Location location, Frame *relativeTo)
{
    qCDebug(docking) << "DropArea::addFrame";

    if (auto dock = qobject_cast<DockWidget *>(droppedWindow)) {
        if (!validateAffinity(dock))
            return false;

        auto frame = Config::self().frameworkWidgetFactory()->createFrame();
        frame->addWidget(dock);
        addWidget(frame->asQWidget(), location, relativeTo, DefaultSizeMode::FairButFloor);
    } else if (auto floatingWindow = qobject_cast<FloatingWindow *>(droppedWindow)) {
        if (!validateAffinity(floatingWindow))
            return false;

        addMultiSplitter(floatingWindow->dropArea(), location, relativeTo, DefaultSizeMode::FairButFloor);
        floatingWindow->scheduleDeleteLater();
        return true;
    } else {
        qWarning() << "Unknown dropped widget" << droppedWindow;
        return false;
    }

    return true;
}

void DropArea::removeHover()
{
    m_dropIndicatorOverlay->setWindowBeingDragged(nullptr);
    m_dropIndicatorOverlay->setCurrentDropLocation(DropIndicatorOverlayInterface::DropLocation_None);
}

template<typename T>
bool DropArea::validateAffinity(T *window) const
{
    if (!DockRegistry::self()->affinitiesMatch(window->affinities(), affinities())) {
        // Commented the warning, so we don't warn when hovering over
        //qWarning() << Q_FUNC_INFO << "Refusing to dock widget with incompatible affinity."
                   //<< window->affinityName() << affinityName();
        return false;
    }

    return true;
}
