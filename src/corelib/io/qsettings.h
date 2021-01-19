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

#ifndef QSETTINGS_H
#define QSETTINGS_H

#include <QtCore/qobject.h>
#include <QtCore/qvariant.h>
#include <QtCore/qstring.h>
#include <QtCore/qscopedpointer.h>

QT_REQUIRE_CONFIG(settings);

#include <ctype.h>

QT_BEGIN_NAMESPACE

#ifdef Status // we seem to pick up a macro Status --> int somewhere
#undef Status
#endif

class QIODevice;
class QSettingsPrivate;

#ifndef QT_NO_QOBJECT
class Q_CORE_EXPORT QSettings : public QObject
#else
class Q_CORE_EXPORT QSettings
#endif
{
#ifndef QT_NO_QOBJECT
    Q_OBJECT
#else
    QScopedPointer<QSettingsPrivate> d_ptr;
#endif
    Q_DECLARE_PRIVATE(QSettings)

public:
    enum Status {
        NoError = 0,
        AccessError,
        FormatError
    };
#ifndef QT_NO_QOBJECT
    Q_ENUM(Status)
#endif

    enum Format {
        NativeFormat,
        IniFormat,

#if defined(Q_OS_WIN) || defined(Q_CLANG_QDOC)
        Registry32Format,
        Registry64Format,
#endif

        InvalidFormat = 16,
        CustomFormat1,
        CustomFormat2,
        CustomFormat3,
        CustomFormat4,
        CustomFormat5,
        CustomFormat6,
        CustomFormat7,
        CustomFormat8,
        CustomFormat9,
        CustomFormat10,
        CustomFormat11,
        CustomFormat12,
        CustomFormat13,
        CustomFormat14,
        CustomFormat15,
        CustomFormat16
    };
#ifndef QT_NO_QOBJECT
    Q_ENUM(Format)
#endif

    enum Scope {
        UserScope,
        SystemScope
    };
#ifndef QT_NO_QOBJECT
    Q_ENUM(Scope)
#endif

#ifndef QT_NO_QOBJECT
    explicit QSettings(const QString &organization,
                       const QString &application = QString(), QObject *parent = nullptr);
    QSettings(Scope scope, const QString &organization,
              const QString &application = QString(), QObject *parent = nullptr);
    QSettings(Format format, Scope scope, const QString &organization,
              const QString &application = QString(), QObject *parent = nullptr);
    QSettings(const QString &fileName, Format format, QObject *parent = nullptr);
    explicit QSettings(QObject *parent = nullptr);
    explicit QSettings(Scope scope, QObject *parent = nullptr);
#else
    explicit QSettings(const QString &organization,
                       const QString &application = QString());
    QSettings(Scope scope, const QString &organization,
              const QString &application = QString());
    QSettings(Format format, Scope scope, const QString &organization,
              const QString &application = QString());
    QSettings(const QString &fileName, Format format);
#  ifndef QT_BUILD_QMAKE
    explicit QSettings(Scope scope = UserScope);
#  endif
#endif
    ~QSettings();

    void clear();
    void sync();
    Status status() const;
    bool isAtomicSyncRequired() const;
    void setAtomicSyncRequired(bool enable);

    void beginGroup(const QString &prefix);
    void endGroup();
    QString group() const;

    int beginReadArray(const QString &prefix);
    void beginWriteArray(const QString &prefix, int size = -1);
    void endArray();
    void setArrayIndex(int i);

    QStringList allKeys() const;
    QStringList childKeys() const;
    QStringList childGroups() const;
    bool isWritable() const;

    void setValue(const QString &key, const QVariant &value);
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;

    void remove(const QString &key);
    bool contains(const QString &key) const;

    void setFallbacksEnabled(bool b);
    bool fallbacksEnabled() const;

    QString fileName() const;
    Format format() const;
    Scope scope() const;
    QString organizationName() const;
    QString applicationName() const;

#if QT_CONFIG(textcodec)
    void setIniCodec(QTextCodec *codec);
    void setIniCodec(const char *codecName);
    QTextCodec *iniCodec() const;
#endif

    static void setDefaultFormat(Format format);
    static Format defaultFormat();
#if QT_DEPRECATED_SINCE(5, 13)
    QT_DEPRECATED_X("Use QSettings::setPath() instead")
    static void setSystemIniPath(const QString &dir);
    QT_DEPRECATED_X("Use QSettings::setPath() instead")
    static void setUserIniPath(const QString &dir);
#endif
    static void setPath(Format format, Scope scope, const QString &path);

    typedef QMap<QString, QVariant> SettingsMap;
    typedef bool (*ReadFunc)(QIODevice &device, SettingsMap &map);
    typedef bool (*WriteFunc)(QIODevice &device, const SettingsMap &map);

    static Format registerFormat(const QString &extension, ReadFunc readFunc, WriteFunc writeFunc,
                                 Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive);

protected:
#ifndef QT_NO_QOBJECT
    bool event(QEvent *event) override;
#endif

private:
    Q_DISABLE_COPY(QSettings)
};

QT_END_NAMESPACE

#endif // QSETTINGS_H
