/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qapplication.h"
#ifndef QT_NO_DRAGANDDROP
#include "qbitmap.h"
#include "qcursor.h"
#include "qevent.h"
#include "qpainter.h"
#include "qurl.h"
#include "qwidget.h"
#include "qfile.h"
#include "qfileinfo.h"
#include <stdlib.h>
#include <string.h>
#ifndef QT_NO_ACCESSIBILITY
# include "qaccessible.h"
#endif

#include <private/qapplication_p.h>
#include <private/qwidget_p.h>
#include <private/qdnd_p.h>
#include <private/qt_mac_p.h>

QT_BEGIN_NAMESPACE

QT_USE_NAMESPACE

QMacDndAnswerRecord qt_mac_dnd_answer_rec; 

/*****************************************************************************
  QDnD debug facilities
 *****************************************************************************/
//#define DEBUG_DRAG_EVENTS
//#define DEBUG_DRAG_PROMISES

/*****************************************************************************
  QDnD globals
 *****************************************************************************/
bool qt_mac_in_drag = false;

/*****************************************************************************
  Externals
 *****************************************************************************/
extern void qt_mac_send_modifiers_changed(quint32, QObject *); //qapplication_mac.cpp
extern uint qGlobalPostedEventsCount(); //qapplication.cpp
extern RgnHandle qt_mac_get_rgn(); // qregion_mac.cpp
extern void qt_mac_dispose_rgn(RgnHandle); // qregion_mac.cpp
/*****************************************************************************
  QDnD utility functions
 *****************************************************************************/

//action management
#ifdef DEBUG_DRAG_EVENTS
# define MAP_MAC_ENUM(x) x, #x
#else
# define MAP_MAC_ENUM(x) x
#endif
struct mac_enum_mapper
{
    int mac_code;
    int qt_code;
#ifdef DEBUG_DRAG_EVENTS
    char *qt_desc;
#endif
};

/*****************************************************************************
  DnD functions
 *****************************************************************************/
bool QDropData::hasFormat_sys(const QString &mime) const
{
    Q_UNUSED(mime);
    return false;
}

QVariant QDropData::retrieveData_sys(const QString &mime, QVariant::Type type) const
{
    Q_UNUSED(mime);
    Q_UNUSED(type);
    return QVariant();
}

QStringList QDropData::formats_sys() const
{
    return QStringList();
}

void QDragManager::timerEvent(QTimerEvent*)
{
}

bool QDragManager::eventFilter(QObject *, QEvent *)
{
    return false;
}

void QDragManager::updateCursor()
{
}

void QDragManager::cancel(bool)
{
    if(object) {
        beingCancelled = true;
        object = 0;
    }
#ifndef QT_NO_ACCESSIBILITY
    QAccessible::updateAccessibility(this, 0, QAccessible::DragDropEnd);
#endif
}

void QDragManager::move(const QPoint &)
{
}

void QDragManager::drop()
{
}

/**
    If a drop action is already set on the carbon event
    (from e.g. an earlier enter event), we insert the same
    action on the new Qt event that has yet to be sendt.
*/
static inline bool qt_mac_set_existing_drop_action(const DragRef &dragRef, QDropEvent &event)
{
    Q_UNUSED(dragRef);
    Q_UNUSED(event);
    return false;
}

/**
    If an answer rect has been set on the event (after being sent
    to the global event processor), we store that rect so we can
    check if the mouse is in the same area upon next drag move event. 
*/
void qt_mac_copy_answer_rect(const QDragMoveEvent &event)
{
    if (!event.answerRect().isEmpty()) {
        qt_mac_dnd_answer_rec.rect = event.answerRect();
        qt_mac_dnd_answer_rec.buttons = event.mouseButtons();
        qt_mac_dnd_answer_rec.modifiers = event.keyboardModifiers();
        qt_mac_dnd_answer_rec.lastAction = event.dropAction();
    }    
}

bool qt_mac_mouse_inside_answer_rect(QPoint mouse)
{
    if (!qt_mac_dnd_answer_rec.rect.isEmpty()
            && qt_mac_dnd_answer_rec.rect.contains(mouse)
            && QApplication::mouseButtons() == qt_mac_dnd_answer_rec.buttons
            && QApplication::keyboardModifiers() == qt_mac_dnd_answer_rec.modifiers)
        return true;
    else
        return false;
}

bool QWidgetPrivate::qt_mac_dnd_event(uint kind, DragRef dragRef)
{
    Q_UNUSED(kind);
    Q_UNUSED(dragRef);
    return false;
}

void QDragManager::updatePixmap()
{
}

QCocoaDropData::QCocoaDropData(CFStringRef pasteboard)
    : QInternalMimeData()
{
    NSString* pasteboardName = (NSString*)pasteboard;
    [pasteboardName retain];
    dropPasteboard = pasteboard;
}

QCocoaDropData::~QCocoaDropData()
{
    NSString* pasteboardName = (NSString*)dropPasteboard;
    [pasteboardName release];
}

QStringList QCocoaDropData::formats_sys() const
{
    QStringList formats; 
    OSPasteboardRef board;
    if (PasteboardCreate(dropPasteboard, &board) != noErr) {
        qDebug("DnD: Cannot get PasteBoard!");
        return formats;
    }
    formats = QMacPasteboard(board, QMacPasteboardMime::MIME_DND).formats();
    return formats;
}

QVariant QCocoaDropData::retrieveData_sys(const QString &mimeType, QVariant::Type type) const
{
    QVariant data;
    OSPasteboardRef board;
    if (PasteboardCreate(dropPasteboard, &board) != noErr) {
        qDebug("DnD: Cannot get PasteBoard!");
        return data;
    }
    data = QMacPasteboard(board, QMacPasteboardMime::MIME_DND).retrieveData(mimeType, type);
    CFRelease(board);
    return data;
}

bool QCocoaDropData::hasFormat_sys(const QString &mimeType) const
{
    bool has = false;
    OSPasteboardRef board;
    if (PasteboardCreate(dropPasteboard, &board) != noErr) {
        qDebug("DnD: Cannot get PasteBoard!");
        return has;
    }
    has = QMacPasteboard(board, QMacPasteboardMime::MIME_DND).hasFormat(mimeType);
    CFRelease(board);
    return has;
}

#endif // QT_NO_DRAGANDDROP
QT_END_NAMESPACE
