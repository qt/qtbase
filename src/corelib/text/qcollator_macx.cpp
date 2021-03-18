/****************************************************************************
**
** Copyright (C) 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcollator_p.h"
#include "qlocale_p.h"
#include "qstringlist.h"
#include "qstring.h"

#include <QtCore/private/qcore_mac_p.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFLocale.h>

#include <cstring>
#include <QDebug>

QT_BEGIN_NAMESPACE

void QCollatorPrivate::init()
{
    cleanup();
    /*
      LocaleRefFromLocaleString() will accept "POSIX" as the locale name, but
      the locale it produces (named "pos") doesn't implement the [A-Z] < [a-z]
      behavior we expect of the C locale.  We can use QStringView to get round
      that for collation, but this leaves no way to do a sort key.
    */
    if (isC())
        return;

    LocaleRef localeRef;
    OSStatus status =
        LocaleRefFromLocaleString(QLocalePrivate::get(locale)->bcp47Name().constData(), &localeRef);
    if (status != 0)
        qWarning("Couldn't initialize the locale (%d)", int(status));

    UInt32 options = 0;
    if (caseSensitivity == Qt::CaseInsensitive)
        options |= kUCCollateCaseInsensitiveMask;
    if (numericMode)
        options |= kUCCollateDigitsAsNumberMask | kUCCollateDigitsOverrideMask;
    if (!ignorePunctuation)
        options |= kUCCollatePunctuationSignificantMask;

    status = UCCreateCollator(localeRef, 0, options, &collator);
    if (status != 0)
        qWarning("Couldn't initialize the collator (%d)", int(status));

    dirty = false;
}

void QCollatorPrivate::cleanup()
{
    if (collator)
        UCDisposeCollator(&collator);
    collator = 0;
}

int QCollator::compare(QStringView s1, QStringView s2) const
{
    if (!s1.size())
        return s2.size() ? -1 : 0;
    if (!s2.size())
        return +1;

    if (d->dirty)
        d->init();
    if (!d->collator)
        return s1.compare(s2, caseSensitivity());

    SInt32 result;
    Boolean equivalent;
    UCCompareText(d->collator,
                  reinterpret_cast<const UniChar *>(s1.data()), s1.size(),
                  reinterpret_cast<const UniChar *>(s2.data()), s2.size(),
                  &equivalent,
                  &result);
    if (equivalent)
        return 0;
    return result < 0 ? -1 : 1;
}

QCollatorSortKey QCollator::sortKey(const QString &string) const
{
    if (d->dirty)
        d->init();
    if (!d->collator) {
        // What should (or even *can*) we do here ? (See init()'s comment.)
        qWarning("QCollator doesn't support sort keys for the C locale on Darwin");
        return QCollatorSortKey(nullptr);
    }

    //Documentation recommends having it 5 times as big as the input
    QVector<UCCollationValue> ret(string.size() * 5);
    ItemCount actualSize;
    int status = UCGetCollationKey(d->collator,
                                   reinterpret_cast<const UniChar *>(string.constData()),
                                   string.count(), ret.size(), &actualSize, ret.data());

    ret.resize(actualSize + 1);
    if (status == kUCOutputBufferTooSmall) {
        UCGetCollationKey(d->collator, reinterpret_cast<const UniChar *>(string.constData()),
                          string.count(), ret.size(), &actualSize, ret.data());
    }
    ret[actualSize] = 0;
    return QCollatorSortKey(new QCollatorSortKeyPrivate(std::move(ret)));
}

int QCollatorSortKey::compare(const QCollatorSortKey &key) const
{
    if (!d.data())
        return 0;

    SInt32 order;
    UCCompareCollationKeys(d->m_key.data(), d->m_key.size(),
                           key.d->m_key.data(), key.d->m_key.size(),
                           0, &order);
    return order;
}

QT_END_NAMESPACE
