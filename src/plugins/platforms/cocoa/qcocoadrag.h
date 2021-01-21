/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QCOCOADRAG_H
#define QCOCOADRAG_H

#include <AppKit/AppKit.h>
#include <QtGui>
#include <qpa/qplatformdrag.h>
#include <private/qsimpledrag_p.h>

#include <QtGui/private/qdnd_p.h>
#include <QtGui/private/qinternalmimedata_p.h>

#include <QtCore/qeventloop.h>

QT_BEGIN_NAMESPACE

class QCocoaDrag : public QPlatformDrag
{
public:
    QCocoaDrag();
    ~QCocoaDrag();

    QMimeData *dragMimeData();
    Qt::DropAction drag(QDrag *m_drag) override;

    Qt::DropAction defaultAction(Qt::DropActions possibleActions,
                                 Qt::KeyboardModifiers modifiers) const override;

    /**
    * to meet NSView dragImage:at guarantees, we need to record the original
    * event and view when handling an event in QNSView
    */
    void setLastMouseEvent(NSEvent *event, NSView *view);

    void setAcceptedAction(Qt::DropAction act);
    void exitDragLoop();
private:
    QDrag *m_drag;
    NSEvent *m_lastEvent;
    NSView *m_lastView;
    Qt::DropAction m_executed_drop_action;
    QEventLoop internalDragLoop;

    bool maybeDragMultipleItems();

    QPixmap dragPixmap(QDrag *drag, QPoint &hotSpot) const;
};

class QCocoaDropData : public QInternalMimeData
{
public:
    QCocoaDropData(NSPasteboard *pasteboard);
    ~QCocoaDropData();
protected:
    bool hasFormat_sys(const QString &mimeType) const;
    QStringList formats_sys() const;
    QVariant retrieveData_sys(const QString &mimeType, QVariant::Type type) const;
public:
    CFStringRef dropPasteboard;
};


QT_END_NAMESPACE

#endif
