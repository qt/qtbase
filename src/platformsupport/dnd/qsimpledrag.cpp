/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qsimpledrag_p.h"

#include "qbitmap.h"
#include "qdrag.h"
#include "qpixmap.h"
#include "qevent.h"
#include "qfile.h"
#include "qtextcodec.h"
#include "qguiapplication.h"
#include "qpoint.h"
#include "qbuffer.h"
#include "qimage.h"
#include "qregexp.h"
#include "qdir.h"
#include "qimagereader.h"
#include "qimagewriter.h"

#include <private/qguiapplication_p.h>
#include <private/qdnd_p.h>

QT_BEGIN_NAMESPACE

class QDropData : public QInternalMimeData
{
public:
    QDropData();
    ~QDropData();

protected:
    bool hasFormat_sys(const QString &mimeType) const;
    QStringList formats_sys() const;
    QVariant retrieveData_sys(const QString &mimeType, QVariant::Type type) const;
};

QSimpleDrag::QSimpleDrag()
{
    m_dropData = new QDropData();
    currentWindow = 0;
}

QSimpleDrag::~QSimpleDrag()
{
    delete m_dropData;
}

QMimeData *QSimpleDrag::platformDropData()
{
    return m_dropData;
}

void QSimpleDrag::cancel()
{
    QDragManager *m = QDragManager::self();
//    qDebug("QDragManager::cancel");
    if (m->object->target()) {
        QDragLeaveEvent dle;
        QCoreApplication::sendEvent(m->object->target(), &dle);
    }

}

void QSimpleDrag::move(const QMouseEvent *me)
{
    QWindow *window = QGuiApplication::topLevelAt(me->globalPos());
    QPoint pos;
    if (window)
        pos = me->globalPos() - window->geometry().topLeft();

    QDragManager *m = QDragManager::self();

    if (me->buttons()) {
        Qt::DropAction prevAction = m->global_accepted_action;

        if (currentWindow != window) {
            if (currentWindow) {
                QDragLeaveEvent dle;
                QCoreApplication::sendEvent(currentWindow, &dle);
                m->willDrop = false;
                m->global_accepted_action = Qt::IgnoreAction;
            }
            currentWindow = window;
            if (currentWindow) {
                QDragEnterEvent dee(pos, m->possible_actions, m->dropData(), me->buttons(), me->modifiers());
                QCoreApplication::sendEvent(currentWindow, &dee);
                m->willDrop = dee.isAccepted() && dee.dropAction() != Qt::IgnoreAction;
                m->global_accepted_action = m->willDrop ? dee.dropAction() : Qt::IgnoreAction;
            }
            m->updateCursor();
        } else if (window) {
            Q_ASSERT(currentWindow);
            QDragMoveEvent dme(pos, m->possible_actions, m->dropData(), me->buttons(), me->modifiers());
            if (m->global_accepted_action != Qt::IgnoreAction) {
                dme.setDropAction(m->global_accepted_action);
                dme.accept();
            }
            QCoreApplication::sendEvent(currentWindow, &dme);
            m->willDrop = dme.isAccepted();
            m->global_accepted_action = m->willDrop ? dme.dropAction() : Qt::IgnoreAction;
            m->updatePixmap();
            m->updateCursor();
        }
        if (m->global_accepted_action != prevAction)
            m->emitActionChanged(m->global_accepted_action);
    }
}

void QSimpleDrag::drop(const QMouseEvent *me)
{
    QDragManager *m = QDragManager::self();

    QWindow *window = QGuiApplication::topLevelAt(me->globalPos());

    if (window) {
        QPoint pos = me->globalPos() - window->geometry().topLeft();

        QDropEvent de(pos, m->possible_actions, m->dropData(), me->buttons(), me->modifiers());
        QCoreApplication::sendEvent(window, &de);
        if (de.isAccepted())
            m->global_accepted_action = de.dropAction();
        else
            m->global_accepted_action = Qt::IgnoreAction;
    }
    currentWindow = 0;
}



QDropData::QDropData()
    : QInternalMimeData()
{
}

QDropData::~QDropData()
{
}

QVariant QDropData::retrieveData_sys(const QString &mimetype, QVariant::Type type) const
{
    QDrag *object = QDragManager::self()->object;
    if (!object)
        return QVariant();
    QByteArray data =  object->mimeData()->data(mimetype);
    if (type == QVariant::String)
        return QString::fromUtf8(data);
    return data;
}

bool QDropData::hasFormat_sys(const QString &format) const
{
    return formats().contains(format);
}

QStringList QDropData::formats_sys() const
{
    QDrag *object = QDragManager::self()->object;
    if (object)
        return object->mimeData()->formats();
    return QStringList();
}

QT_END_NAMESPACE
