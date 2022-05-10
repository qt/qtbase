// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsettings.h"
#ifndef QT_NO_SETTINGS

#include "qsettings_p.h"
#ifndef QT_NO_QOBJECT
#include "qcoreapplication.h"
#include <QFile>
#endif // QT_NO_QOBJECT
#include <QDebug>

#include <QFileInfo>
#include <QDir>
#include <emscripten.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static bool isReadReady = false;

class QWasmSettingsPrivate : public QConfFileSettingsPrivate
{
public:
    QWasmSettingsPrivate(QSettings::Scope scope, const QString &organization,
                        const QString &application);
    ~QWasmSettingsPrivate();

    std::optional<QVariant> get(const QString &key) const override;
    QStringList children(const QString &prefix, ChildSpec spec) const override;
    void clear() override;
    void sync() override;
    void flush() override;
    bool isWritable() const override;

    void syncToLocal(const char *data, int size);
    void loadLocal(const QByteArray &filename);
    void setReady();
    void initAccess() override;

private:
    QString databaseName;
    QString id;
};

static void QWasmSettingsPrivate_onLoad(void *userData, void *dataPtr, int size)
{
    QWasmSettingsPrivate *wasm = reinterpret_cast<QWasmSettingsPrivate *>(userData);

    QFile file(wasm->fileName());
    QFileInfo fileInfo(wasm->fileName());
    QDir dir(fileInfo.path());
    if (!dir.exists())
        dir.mkpath(fileInfo.path());

    if (file.open(QFile::WriteOnly)) {
        file.write(reinterpret_cast<char *>(dataPtr), size);
        file.close();
        wasm->setReady();
    }
}

static void QWasmSettingsPrivate_onError(void *userData)
{
    QWasmSettingsPrivate *wasm = reinterpret_cast<QWasmSettingsPrivate *>(userData);
    if (wasm)
        wasm->setStatus(QSettings::AccessError);
}

static void QWasmSettingsPrivate_onStore(void *userData)
{
    QWasmSettingsPrivate *wasm = reinterpret_cast<QWasmSettingsPrivate *>(userData);
    if (wasm)
        wasm->setStatus(QSettings::NoError);
}

static void QWasmSettingsPrivate_onCheck(void *userData, int exists)
{
    QWasmSettingsPrivate *wasm = reinterpret_cast<QWasmSettingsPrivate *>(userData);
    if (wasm) {
        if (exists)
            wasm->loadLocal(wasm->fileName().toLocal8Bit());
        else
            wasm->setReady();
    }
}

QSettingsPrivate *QSettingsPrivate::create(QSettings::Format format,
                                           QSettings::Scope scope,
                                           const QString &organization,
                                           const QString &application)
{
    Q_UNUSED(format);
    if (organization == "Qt"_L1)
    {
        QString organizationDomain = QCoreApplication::organizationDomain();
        QString applicationName = QCoreApplication::applicationName();

        QSettingsPrivate *newSettings;
        newSettings = new QWasmSettingsPrivate(scope, organizationDomain, applicationName);

        newSettings->beginGroupOrArray(QSettingsGroup(normalizedKey(organization)));
        if (!application.isEmpty())
            newSettings->beginGroupOrArray(QSettingsGroup(normalizedKey(application)));

        return newSettings;
    }
    return new QWasmSettingsPrivate(scope, organization, application);
}

QWasmSettingsPrivate::QWasmSettingsPrivate(QSettings::Scope scope, const QString &organization,
                                           const QString &application)
    : QConfFileSettingsPrivate(QSettings::NativeFormat, scope, organization, application)
{
    setStatus(QSettings::AccessError); // access error until sandbox gets loaded
    databaseName = organization;
    id = application;

    emscripten_idb_async_exists("/home/web_user",
                                fileName().toLocal8Bit(),
                                reinterpret_cast<void*>(this),
                                QWasmSettingsPrivate_onCheck,
                                QWasmSettingsPrivate_onError);
}

QWasmSettingsPrivate::~QWasmSettingsPrivate()
{
}

 void QWasmSettingsPrivate::initAccess()
{
     if (isReadReady)
         QConfFileSettingsPrivate::initAccess();
}

std::optional<QVariant> QWasmSettingsPrivate::get(const QString &key) const
{
    if (isReadReady)
        return QConfFileSettingsPrivate::get(key);

    return std::nullopt;
}

QStringList QWasmSettingsPrivate::children(const QString &prefix, ChildSpec spec) const
{
    return QConfFileSettingsPrivate::children(prefix, spec);
}

void QWasmSettingsPrivate::clear()
{
    QConfFileSettingsPrivate::clear();
    emscripten_idb_async_delete("/home/web_user",
                                fileName().toLocal8Bit(),
                                reinterpret_cast<void*>(this),
                                QWasmSettingsPrivate_onStore,
                                QWasmSettingsPrivate_onError);
}

void QWasmSettingsPrivate::sync()
{
    QConfFileSettingsPrivate::sync();

    QFile file(fileName());
    if (file.open(QFile::ReadOnly)) {
        QByteArray dataPointer = file.readAll();

        emscripten_idb_async_store("/home/web_user",
                                  fileName().toLocal8Bit(),
                                   reinterpret_cast<void *>(dataPointer.data()),
                                   dataPointer.length(),
                                   reinterpret_cast<void*>(this),
                                   QWasmSettingsPrivate_onStore,
                                   QWasmSettingsPrivate_onError);
    }
}

void QWasmSettingsPrivate::flush()
{
    sync();
}

bool QWasmSettingsPrivate::isWritable() const
{
    return isReadReady && QConfFileSettingsPrivate::isWritable();
}

void QWasmSettingsPrivate::syncToLocal(const char *data, int size)
{
    QFile file(fileName());

    if (file.open(QFile::WriteOnly)) {
        file.write(data, size + 1);
        QByteArray data = file.readAll();

        emscripten_idb_async_store("/home/web_user",
                                   fileName().toLocal8Bit(),
                                   reinterpret_cast<void *>(data.data()),
                                   data.length(),
                                   reinterpret_cast<void*>(this),
                                   QWasmSettingsPrivate_onStore,
                                   QWasmSettingsPrivate_onError);
        setReady();
    }
}

void QWasmSettingsPrivate::loadLocal(const QByteArray &filename)
{
    emscripten_idb_async_load("/home/web_user",
                              filename.data(),
                              reinterpret_cast<void*>(this),
                              QWasmSettingsPrivate_onLoad,
                              QWasmSettingsPrivate_onError);
}

void QWasmSettingsPrivate::setReady()
{
    isReadReady = true;
    setStatus(QSettings::NoError);
    QConfFileSettingsPrivate::initAccess();
}

QT_END_NAMESPACE
#endif // QT_NO_SETTINGS
