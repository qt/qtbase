// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMACMIME_H
#define QMACMIME_H

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

#include <CoreFoundation/CoreFoundation.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QMacMime
{
public:
    enum class HandlerScope : uchar
    {
        DnD            = 0x01,
        Clipboard      = 0x02,
        Qt_compatible  = 0x04,
        Qt3_compatible = 0x08,
        All            = DnD|Clipboard,
        AllCompatible  = All|Qt_compatible
    };

    QMacMime();
    explicit QMacMime(HandlerScope scope); // internal
    virtual ~QMacMime();

    HandlerScope scope() const { return m_scope; }
    bool canConvert(const QString &mime, const QString &uti) const { return mimeForUti(uti) == mime; }

    // for converting from Qt
    virtual QList<QByteArray> convertFromMime(const QString &mime, const QVariant &data, const QString &uti) const = 0;
    virtual QString utiForMime(const QString &mime) const = 0;

    // for converting to Qt
    virtual QString mimeForUti(const QString &uti) const = 0;
    virtual QVariant convertToMime(const QString &mime, const QList<QByteArray> &data, const QString &uti) const = 0;
    virtual int count(const QMimeData *mimeData) const;

private:
    const HandlerScope m_scope;
};

QT_END_NAMESPACE

#endif

