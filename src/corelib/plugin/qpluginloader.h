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

#ifndef QPLUGINLOADER_H
#define QPLUGINLOADER_H

#include <QtCore/qglobal.h>
#if QT_CONFIG(library)
#include <QtCore/qlibrary.h>
#endif
#include <QtCore/qplugin.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(library)

class QLibraryPrivate;
class QJsonObject;

class Q_CORE_EXPORT QPluginLoader : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString fileName READ fileName WRITE setFileName)
    Q_PROPERTY(QLibrary::LoadHints loadHints READ loadHints WRITE setLoadHints)
public:
    explicit QPluginLoader(QObject *parent = nullptr);
    explicit QPluginLoader(const QString &fileName, QObject *parent = nullptr);
    ~QPluginLoader();

    QObject *instance();
    QJsonObject metaData() const;

    static QObjectList staticInstances();
    static QVector<QStaticPlugin> staticPlugins();

    bool load();
    bool unload();
    bool isLoaded() const;

    void setFileName(const QString &fileName);
    QString fileName() const;

    QString errorString() const;

    void setLoadHints(QLibrary::LoadHints loadHints);
    QLibrary::LoadHints loadHints() const;

private:
    QLibraryPrivate *d;
    bool did_load;
    Q_DISABLE_COPY(QPluginLoader)
};

#else

class Q_CORE_EXPORT QPluginLoader
{
public:
    static QObjectList staticInstances();
    static QVector<QStaticPlugin> staticPlugins();
};

#endif // QT_CONFIG(library)

QT_END_NAMESPACE

#endif //QPLUGINLOADER_H
