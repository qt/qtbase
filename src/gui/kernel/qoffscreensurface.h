/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QOFFSCREENSURFACE_H
#define QOFFSCREENSURFACE_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/QObject>
#include <QtGui/qsurface.h>

QT_BEGIN_NAMESPACE

class QOffscreenSurfacePrivate;

class QScreen;
class QPlatformOffscreenSurface;

class Q_GUI_EXPORT QOffscreenSurface : public QObject, public QSurface
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QOffscreenSurface)

public:
    // ### Qt 6: merge overloads
    explicit QOffscreenSurface(QScreen *screen, QObject *parent);
    explicit QOffscreenSurface(QScreen *screen = nullptr);
    ~QOffscreenSurface();

    SurfaceType surfaceType() const override;

    void create();
    void destroy();

    bool isValid() const;

    void setFormat(const QSurfaceFormat &format);
    QSurfaceFormat format() const override;
    QSurfaceFormat requestedFormat() const;

    QSize size() const override;

    QScreen *screen() const;
    void setScreen(QScreen *screen);

    QPlatformOffscreenSurface *handle() const;

    void *nativeHandle() const;
    void setNativeHandle(void *handle);

Q_SIGNALS:
    void screenChanged(QScreen *screen);

private Q_SLOTS:
    void screenDestroyed(QObject *screen);

private:

    QPlatformSurface *surfaceHandle() const override;

    Q_DISABLE_COPY(QOffscreenSurface)
};

QT_END_NAMESPACE

#endif // QOFFSCREENSURFACE_H
