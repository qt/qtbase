// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "user.h"

//! [user-setters]

void User::setCountry(Country country)
{
    if (m_country != country) {
        m_country = country;
        emit countryChanged();
    }
}

void User::setAge(int age)
{
    if (m_age != age) {
        m_age = age;
        emit ageChanged();
    }
}

//! [user-setters]
