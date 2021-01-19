/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2013 Aleix Pol Gonzalez <aleixpol@kde.org>
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

#ifndef QCOLLATOR_P_H
#define QCOLLATOR_P_H

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

#include <QtCore/private/qglobal_p.h>
#include "qcollator.h"
#include <QVector>
#if QT_CONFIG(icu)
#include <unicode/ucol.h>
#elif defined(Q_OS_MACOS)
#include <CoreServices/CoreServices.h>
#elif defined(Q_OS_WIN)
#include <qt_windows.h>
#endif

QT_BEGIN_NAMESPACE

#if QT_CONFIG(icu)
typedef UCollator *CollatorType;
typedef QByteArray CollatorKeyType;
const CollatorType NoCollator = nullptr;

#elif defined(Q_OS_MACOS)
typedef CollatorRef CollatorType;
typedef QVector<UCCollationValue> CollatorKeyType;
const CollatorType NoCollator = 0;

#elif defined(Q_OS_WIN)
typedef QString CollatorKeyType;
typedef int CollatorType;
const CollatorType NoCollator = 0;
#  ifdef Q_OS_WINRT
#    define USE_COMPARESTRINGEX
#  endif

#else // posix - ignores CollatorType collator, only handles system locale
typedef QVector<wchar_t> CollatorKeyType;
typedef bool CollatorType;
const CollatorType NoCollator = false;
#endif

class QCollatorPrivate
{
public:
    QAtomicInt ref = 1;
    QLocale locale;
#if defined(Q_OS_WIN) && !QT_CONFIG(icu)
#ifdef USE_COMPARESTRINGEX
    QString localeName;
#else
    LCID localeID;
#endif
#endif
    Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive;
    bool numericMode = false;
    bool ignorePunctuation = false;
    bool dirty = true;

    CollatorType collator = NoCollator;

    QCollatorPrivate(const QLocale &locale) : locale(locale) {}
    ~QCollatorPrivate() { cleanup(); }
    bool isC() { return locale.language() == QLocale::C; }

    void clear() {
        cleanup();
        collator = NoCollator;
    }

    // Implemented by each back-end, in its own way:
    void init();
    void cleanup();

private:
    Q_DISABLE_COPY_MOVE(QCollatorPrivate)
};

class QCollatorSortKeyPrivate : public QSharedData
{
    friend class QCollator;
public:
    template <typename...T>
    explicit QCollatorSortKeyPrivate(T &&...args)
        : QSharedData()
        , m_key(std::forward<T>(args)...)
    {
    }

    CollatorKeyType m_key;

private:
    Q_DISABLE_COPY_MOVE(QCollatorSortKeyPrivate)
};


QT_END_NAMESPACE

#endif // QCOLLATOR_P_H
