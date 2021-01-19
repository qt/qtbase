/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QSIGNALMAPPER_H
#define QSIGNALMAPPER_H

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QSignalMapperPrivate;

class Q_CORE_EXPORT QSignalMapper : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSignalMapper)
public:
    explicit QSignalMapper(QObject *parent = nullptr);
    ~QSignalMapper();

    void setMapping(QObject *sender, int id);
    void setMapping(QObject *sender, const QString &text);
    void setMapping(QObject *sender, QWidget *widget);
    void setMapping(QObject *sender, QObject *object);
    void removeMappings(QObject *sender);

    QObject *mapping(int id) const;
    QObject *mapping(const QString &text) const;
    QObject *mapping(QWidget *widget) const;
    QObject *mapping(QObject *object) const;

Q_SIGNALS:
#if QT_DEPRECATED_SINCE(5, 15)
    QT_DEPRECATED_VERSION_X_5_15("Use QSignalMapper::mappedInt(int) instead")
    void mapped(int);
    QT_DEPRECATED_VERSION_X_5_15("Use QSignalMapper::mappedString(const QString&) instead")
    void mapped(const QString &);
    QT_DEPRECATED_VERSION_X_5_15("Use QSignalMapper::mappedWidget(QWidget *) instead")
    void mapped(QWidget *);
    QT_DEPRECATED_VERSION_X_5_15("Use QSignalMapper::mappedObject(QObject *) instead")
    void mapped(QObject *);
#endif
    void mappedInt(int);
    void mappedString(const QString &);
    void mappedWidget(QWidget *);
    void mappedObject(QObject *);

public Q_SLOTS:
    void map();
    void map(QObject *sender);

private:
    Q_DISABLE_COPY(QSignalMapper)
    Q_PRIVATE_SLOT(d_func(), void _q_senderDestroyed())
};

QT_END_NAMESPACE

#endif // QSIGNALMAPPER_H
