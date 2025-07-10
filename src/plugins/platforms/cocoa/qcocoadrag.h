// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOCOADRAG_H
#define QCOCOADRAG_H

#include <qpa/qplatformdrag.h>
#include <QtGui/private/qsimpledrag_p.h>
#include <QtGui/private/qinternalmimedata_p.h>

#include <QtCore/private/qcore_mac_p.h>


Q_FORWARD_DECLARE_OBJC_CLASS(NSView);
Q_FORWARD_DECLARE_OBJC_CLASS(NSEvent);
Q_FORWARD_DECLARE_OBJC_CLASS(NSPasteboard);

QT_BEGIN_NAMESPACE

class QDrag;
class QEventLoop;
class QMimeData;

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
    void setLastInputEvent(NSEvent *event, NSView *view);

    void setAcceptedAction(Qt::DropAction act);
    void exitDragLoop();
private:
    QDrag *m_drag;
    NSEvent *m_lastEvent;
    QObjCWeakPointer<NSView> m_lastView;
    Qt::DropAction m_executed_drop_action;
    QEventLoop *m_internalDragLoop = nullptr;

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
    QVariant retrieveData_sys(const QString &mimeType, QMetaType type) const;
public:
    CFStringRef dropPasteboard;
};


QT_END_NAMESPACE

#endif
