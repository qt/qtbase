/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCOCOADRAG_H
#define QCOCOADRAG_H

#include <Cocoa/Cocoa.h>
#include <QtGui>
#include <qpa/qplatformdrag.h>
#include <QtPlatformSupport/private/qsimpledrag_p.h>

#include <QtGui/private/qdnd_p.h>

QT_BEGIN_NAMESPACE

class QCocoaDrag : public QPlatformDrag
{
public:
    QCocoaDrag();

    virtual QMimeData *platformDropData();
    virtual Qt::DropAction drag(QDrag *m_drag);

    virtual Qt::DropAction defaultAction(Qt::DropActions possibleActions,
                                               Qt::KeyboardModifiers modifiers) const;

    /**
    * to meet NSView dragImage:at guarantees, we need to record the original
    * event and view when handling an event in QNSView
    */
    void setLastMouseEvent(NSEvent *event, NSView *view);

    void setAcceptedAction(Qt::DropAction act);
private:
    QDrag *m_drag;
    NSEvent *m_lastEvent;
    NSView *m_lastView;
    Qt::DropAction m_executed_drop_action;
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
