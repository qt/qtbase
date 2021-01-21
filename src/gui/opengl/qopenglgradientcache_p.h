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

#ifndef QOPENGLGRADIENTCACHE_P_H
#define QOPENGLGRADIENTCACHE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include <QMultiHash>
#include <QObject>
#include <private/qopenglcontext_p.h>
#include <QtCore/qmutex.h>
#include <QGradient>
#include <qrgba64.h>

QT_BEGIN_NAMESPACE

class QOpenGL2GradientCache : public QOpenGLSharedResource
{
    struct CacheInfo
    {
        inline CacheInfo(QGradientStops s, qreal op, QGradient::InterpolationMode mode) :
            stops(std::move(s)), opacity(op), interpolationMode(mode) {}

        GLuint texId;
        QGradientStops stops;
        qreal opacity;
        QGradient::InterpolationMode interpolationMode;
    };

    typedef QMultiHash<quint64, CacheInfo> QOpenGLGradientColorTableHash;

public:
    static QOpenGL2GradientCache *cacheForContext(QOpenGLContext *context);

    QOpenGL2GradientCache(QOpenGLContext *);
    ~QOpenGL2GradientCache();

    GLuint getBuffer(const QGradient &gradient, qreal opacity);
    inline int paletteSize() const { return 1024; }

    void invalidateResource() override;
    void freeResource(QOpenGLContext *ctx) override;

private:
    inline int maxCacheSize() const { return 60; }
    inline void generateGradientColorTable(const QGradient& gradient,
                                           QRgba64 *colorTable,
                                           int size, qreal opacity) const;
    inline void generateGradientColorTable(const QGradient& gradient,
                                           uint *colorTable,
                                           int size, qreal opacity) const;
    GLuint addCacheElement(quint64 hash_val, const QGradient &gradient, qreal opacity);
    void cleanCache();

    QOpenGLGradientColorTableHash cache;
    QMutex m_mutex;
};

QT_END_NAMESPACE

#endif // QOPENGLGRADIENTCACHE_P_H
