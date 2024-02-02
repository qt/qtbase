// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef __DUMMYDATAGEN_H__
#define __DUMMYDATAGEN_H__

#include <QObject>
#include <QStringList>

class DummyDataGenerator : public QObject
{
    Q_OBJECT
public:
    DummyDataGenerator();
    ~DummyDataGenerator();

public:
    void Reset();
    QString randomPhoneNumber(QString indexNumber);
    QString randomFirstName();
    QString randomLastName();
    QString randomName();
    QString randomIconItem();
    QString randomStatusItem();

private:
    QStringList m_countryCodes;
    QStringList m_firstNamesF;
    QStringList m_firstNamesM;
    QStringList m_lastNames;
    bool m_isMale;
};

#endif // __DUMMYDATAGEN_H__
