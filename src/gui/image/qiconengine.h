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

#ifndef QICONENGINE_H
#define QICONENGINE_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qlist.h>
#include <QtGui/qicon.h>

QT_BEGIN_NAMESPACE


class Q_GUI_EXPORT QIconEngine
{
public:
    QIconEngine();
    QIconEngine(const QIconEngine &other);  // ### Qt6: make protected
    virtual ~QIconEngine();
    virtual void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) = 0;
    virtual QSize actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state);
    virtual QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state);

    virtual void addPixmap(const QPixmap &pixmap, QIcon::Mode mode, QIcon::State state);
    virtual void addFile(const QString &fileName, const QSize &size, QIcon::Mode mode, QIcon::State state);

    virtual QString key() const;
    virtual QIconEngine *clone() const = 0;
    virtual bool read(QDataStream &in);
    virtual bool write(QDataStream &out) const;

    enum IconEngineHook { AvailableSizesHook = 1, IconNameHook, IsNullHook, ScaledPixmapHook };

    struct AvailableSizesArgument
    {
        QIcon::Mode mode;
        QIcon::State state;
        QList<QSize> sizes;
    };

    virtual QList<QSize> availableSizes(QIcon::Mode mode = QIcon::Normal,
                                    QIcon::State state = QIcon::Off) const;

    virtual QString iconName() const;
    bool isNull() const; // ### Qt6 make virtual
    QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale); // ### Qt6 make virtual

    struct ScaledPixmapArgument
    {
        QSize size;
        QIcon::Mode mode;
        QIcon::State state;
        qreal scale;
        QPixmap pixmap;
    };

    // ### Qt6: move content to proper virtual functions
    virtual void virtual_hook(int id, void *data);

private:
    QIconEngine &operator=(const QIconEngine &other) = delete;
};

#if QT_DEPRECATED_SINCE(5, 0)
typedef QIconEngine QIconEngineV2;
#endif

QT_END_NAMESPACE

#endif // QICONENGINE_H
