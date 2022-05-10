// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QFile>
#include <QRandomGenerator>
#include "theme.h"

#include "dummydatagen.h"

DummyDataGenerator::DummyDataGenerator() : m_isMale(false)
{
    QFile countryCodeFile(":/contact/areacodes.txt");
    countryCodeFile.open(QIODevice::ReadOnly);
    while (!countryCodeFile.atEnd()) {
        m_countryCodes << QString(countryCodeFile.readLine()).remove("\n");
    }

    QFile firstNameFFile(":/contact/firstnamesF.txt");
    firstNameFFile.open(QIODevice::ReadOnly);
    while (!firstNameFFile.atEnd()) {
        m_firstNamesF << QString(firstNameFFile.readLine()).remove("\n");
    }

    QFile firstNameMFile(":/contact/firstnamesM.txt");
    firstNameMFile.open(QIODevice::ReadOnly);
    while (!firstNameMFile.atEnd()) {
        m_firstNamesM << QString(firstNameMFile.readLine()).remove("\n");
    }

    QFile lastNameFile(":/contact/lastnames.txt");
    lastNameFile.open(QIODevice::ReadOnly);
    while (!lastNameFile.atEnd()) {
        m_lastNames << QString(lastNameFile.readLine()).remove("\n");
    }
    Reset();
}

DummyDataGenerator::~DummyDataGenerator()
{
}

void DummyDataGenerator::Reset()
{
}

QString DummyDataGenerator::randomPhoneNumber(QString indexNumber)
{
    int index = QRandomGenerator::global()->bounded(m_countryCodes.count());
    QString countryCode = m_countryCodes.at(index);
    QString areaCode = QString::number(index) + QString("0").repeated(2-QString::number(index).length());
    QString beginNumber = QString::number(555-index*2);
    QString endNumber = QString("0").repeated(4-indexNumber.length()) + indexNumber;

    return countryCode + QLatin1Char(' ') + areaCode +QLatin1Char(' ') + beginNumber
           + QLatin1Char(' ') + endNumber;
}

QString DummyDataGenerator::randomFirstName()
{
    m_isMale = !m_isMale;
    if (m_isMale)
        return m_firstNamesM.at(QRandomGenerator::global()->bounded(m_firstNamesM.count()));
    return m_firstNamesF.at(QRandomGenerator::global()->bounded(m_firstNamesF.count()));
}

QString DummyDataGenerator::randomLastName()
{
    return m_lastNames.at(QRandomGenerator::global()->bounded(m_lastNames.count()));
}

QString DummyDataGenerator::randomName()
{
    return QString(randomFirstName()+QString(", ")+randomLastName());
}

QString DummyDataGenerator::randomIconItem()
{
    QString avatar = Theme::p()->pixmapPath() + "contact_default_icon.svg";
    if (QRandomGenerator::global()->bounded(4)) {
      int randVal = 1+QRandomGenerator::global()->bounded(25);

      if (m_isMale && randVal > 15) {
          randVal -= 15;
      }
      if (!m_isMale && randVal <= 10) {
          randVal += 10;
      }

      avatar = QString(":/avatars/avatar_%1.png").arg(randVal, 3, 10, QChar('0'));
    }
    return avatar;
}

QString DummyDataGenerator::randomStatusItem()
{
    switch (QRandomGenerator::global()->bounded(3)) {
        case 0: return Theme::p()->pixmapPath() + "contact_status_online.svg";
        case 1: return Theme::p()->pixmapPath() + "contact_status_offline.svg";
        case 2: return Theme::p()->pixmapPath() + "contact_status_idle.svg";
    }
    return {};
}
