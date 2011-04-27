/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSCREENVNC_QWS_H
#define QSCREENVNC_QWS_H

#include <QtGui/qscreenproxy_qws.h>

#ifndef QT_NO_QWS_VNC

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QVNCScreenPrivate;

class QVNCScreen : public QProxyScreen
{
public:
    explicit QVNCScreen(int display_id);
    virtual ~QVNCScreen();

    bool initDevice();
    bool connect(const QString &displaySpec);
    void disconnect();
    void shutdownDevice();

    void setDirty(const QRect&);

private:
    friend class QVNCCursor;
    friend class QVNCClientCursor;
    friend class QVNCServer;
    friend class QVNCScreenPrivate;

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    bool swapBytes() const;
#endif

    QVNCScreenPrivate *d_ptr;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_QWS_VNC
#endif // QSCREENVNC_QWS_H
