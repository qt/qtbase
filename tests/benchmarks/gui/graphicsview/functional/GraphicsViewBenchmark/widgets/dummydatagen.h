/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
