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

#ifndef QAHISCREEN_H
#define QAHISCREEN_H

#include <QtGui/qscreenlinuxfb_qws.h>

#ifndef QT_NO_QWS_AHI

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QAhiScreenPrivate;

class QAhiScreen : public QScreen
{
public:
    QAhiScreen(int displayId);
    ~QAhiScreen();

    bool connect(const QString &displaySpec);
    void disconnect();
    bool initDevice();
    void shutdownDevice();
    void setMode(int width, int height, int depth);

    void blit(const QImage &image, const QPoint &topLeft,
              const QRegion &region);
    void solidFill(const QColor &color, const QRegion &region);

private:
    bool configure();

    QAhiScreenPrivate *d_ptr;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_QWS_AHI
#endif // QAHISCREEN_H
