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

#ifndef QWINREGISTRY_H
#define QWINREGISTRY_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qpair.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringview.h>
#include <QtCore/qt_windows.h>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QWinRegistryKey
{
public:
    Q_DISABLE_COPY(QWinRegistryKey)

    QWinRegistryKey();
    explicit QWinRegistryKey(HKEY parentHandle, QStringView subKey,
                             REGSAM permissions = KEY_READ, REGSAM access = 0);
    ~QWinRegistryKey();

    QWinRegistryKey(QWinRegistryKey &&other) noexcept { swap(other); }
    QWinRegistryKey &operator=(QWinRegistryKey &&other) noexcept { swap(other); return *this; }

    void swap(QWinRegistryKey &other) noexcept { qSwap(m_key, other.m_key); }

    bool isValid() const { return m_key != nullptr; }
    operator HKEY() const { return m_key; }
    void close();

    QString stringValue(QStringView subKey) const;
    QPair<DWORD, bool> dwordValue(QStringView subKey) const;

private:
    HKEY m_key;
};

QT_END_NAMESPACE

#endif // QWINREGISTRY_H
