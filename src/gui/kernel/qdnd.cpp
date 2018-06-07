/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdnd_p.h"

#include "qguiapplication.h"
#include <ctype.h>
#include <qpa/qplatformdrag.h>
#include <qpa/qplatformintegration.h>
#include <private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

// the universe's only drag manager
QDragManager *QDragManager::m_instance = 0;

QDragManager::QDragManager()
    : QObject(qApp), m_currentDropTarget(0),
      m_platformDrag(QGuiApplicationPrivate::platformIntegration()->drag()),
      m_object(0)
{
    Q_ASSERT(!m_instance);
}

QDragManager::~QDragManager()
{
    m_instance = 0;
}

QDragManager *QDragManager::self()
{
    if (!m_instance && !QGuiApplication::closingDown())
        m_instance = new QDragManager;
    return m_instance;
}

QObject *QDragManager::source() const
{
    if (m_object)
        return m_object->source();
    return 0;
}

void QDragManager::setCurrentTarget(QObject *target, bool dropped)
{
    if (m_currentDropTarget == target)
        return;

    m_currentDropTarget = target;
    if (!dropped && m_object) {
        m_object->d_func()->target = target;
        emit m_object->targetChanged(target);
    }
}

QObject *QDragManager::currentTarget() const
{
    return m_currentDropTarget;
}

Qt::DropAction QDragManager::drag(QDrag *o)
{
    if (!o || m_object == o)
         return Qt::IgnoreAction;

    if (!m_platformDrag || !o->source()) {
        o->deleteLater();
        return Qt::IgnoreAction;
    }

    if (m_object) {
        qWarning("QDragManager::drag in possibly invalid state");
        return Qt::IgnoreAction;
    }

    m_object = o;

    m_object->d_func()->target = 0;

    QGuiApplicationPrivate::instance()->notifyDragStarted(o);
    const Qt::DropAction result = m_platformDrag->drag(m_object);
    m_object = 0;
    if (!m_platformDrag->ownsDragObject())
        o->deleteLater();
    return result;
}

QT_END_NAMESPACE
