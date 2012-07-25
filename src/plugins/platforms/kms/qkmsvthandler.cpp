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

#include <qkmsvthandler.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <QDebug>

#ifdef K_OFF
#define KBD_OFF_MODE K_OFF
#else
#define KBD_OFF_MODE K_RAW
#endif

QT_BEGIN_NAMESPACE

QKmsVTHandler::QKmsVTHandler(QObject *parent)
    : QObject(parent), m_tty(-1)
{
    if (!isatty(0))
        return;

    m_tty = 0;

    ioctl(m_tty, KDGKBMODE, &m_oldKbdMode);
    if (!qgetenv("QT_KMS_TTYKBD").toInt())
        ioctl(m_tty, KDSKBMODE, KBD_OFF_MODE);
}

QKmsVTHandler::~QKmsVTHandler()
{
    if (m_tty == -1)
        return;

    ioctl(m_tty, KDSKBMODE, m_oldKbdMode);
}

QT_END_NAMESPACE
