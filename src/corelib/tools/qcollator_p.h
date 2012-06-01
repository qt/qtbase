/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCOLLATOR_H
#define QCOLLATOR_H

#include <QString>
#include <QLocale>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QCollatorPrivate;

class Q_CORE_EXPORT QCollator
{
public:
    enum Collation {
        Default,
        Big5Han,
        Dictionary,
        Direct,
        GB2312Han,
        PhoneBook,
        Pinyin,
        Phonetic,
        Reformed,
        Standard,
        Stroke,
        Traditional,
        UniHan
    };

    QCollator(const QLocale &locale = QLocale(), QCollator::Collation collation = QCollator::Default);
    QCollator(const QCollator &);
    ~QCollator();
    QCollator &operator=(const QCollator &);

    void setLocale(const QLocale &locale);
    QLocale locale() const;

    void setCollation(Collation collation);
    Collation collation() const;

    QString identifier() const;
    static QCollator fromIdentifier(const QString &identifier);

    enum CasePreference {
        CasePreferenceOff = 0x0,
        CasePreferenceUpper  = 0x1,
        CasePreferenceLower  = 0x2
    };

    CasePreference casePreference() const;
    void setCasePreference(CasePreference c);

    void setNumericMode(bool on);
    bool numericMode() const;

    void setIgnorePunctuation(bool on);
    bool ignorePunctuation() const;

    int compare(const QString &s1, const QString &s2) const;
    int compare(const QStringRef &s1, const QStringRef &s2) const;
    int compare(const QChar *s1, int len1, const QChar *s2, int len2) const;

    bool operator()(const QString &s1, const QString &s2) const
    { return compare(s1, s2) < 0; }

    QByteArray sortKey(const QString &string) const;

    QStringList indexCharacters() const;

private:
    QCollatorPrivate *d;

    void detach();
    void init();
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QCOLLATOR_H
