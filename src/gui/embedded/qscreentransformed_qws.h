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

#ifndef QSCREENTRANSFORMED_QWS_H
#define QSCREENTRANSFORMED_QWS_H

#include <QtGui/qscreenproxy_qws.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_QWS_TRANSFORMED

class QTransformedScreenPrivate;

class Q_AUTOTEST_EXPORT QTransformedScreen : public QProxyScreen
{
public:
    explicit QTransformedScreen(int display_id);
    ~QTransformedScreen();

    enum Transformation { None, Rot90, Rot180, Rot270 };

    void setTransformation(Transformation t);
    Transformation transformation() const;
    int transformOrientation() const;

    QSize mapToDevice(const QSize &s) const;
    QSize mapFromDevice(const QSize &s) const;

    QPoint mapToDevice(const QPoint &, const QSize &) const;
    QPoint mapFromDevice(const QPoint &, const QSize &) const;

    QRect mapToDevice(const QRect &, const QSize &) const;
    QRect mapFromDevice(const QRect &, const QSize &) const;

    QRegion mapToDevice(const QRegion &, const QSize &) const;
    QRegion mapFromDevice(const QRegion &, const QSize &) const;

    bool connect(const QString &displaySpec);

    bool isTransformed() const { return transformation() != None; }

    void exposeRegion(QRegion region, int changing);
    void blit(const QImage &img, const QPoint &topLeft, const QRegion &region);
    void solidFill(const QColor &color, const QRegion &region);
    void setDirty(const QRect&);

    QRegion region() const;

private:
    friend class QTransformedScreenPrivate;
    QTransformedScreenPrivate *d_ptr;
};

#endif // QT_NO_QWS_TRANSFORMED

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSCREENTRANSFORMED_QWS_H
