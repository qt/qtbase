// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "subscription.h"
#include "user.h"

Subscription::Subscription(User *user) : m_user(user)
{
    Q_ASSERT(user);
}

//! [calculate-price]

void Subscription::calculatePrice()
{
    const auto oldPrice = m_price;

    m_price = qRound(calculateDiscount() * m_duration * basePrice());
    if (m_price != oldPrice)
        emit priceChanged();
}

//! [calculate-price]

//! [set-duration]

void Subscription::setDuration(Duration newDuration)
{
    if (newDuration != m_duration) {
        m_duration = newDuration;
        calculatePrice();
        emit durationChanged();
    }
}

//! [set-duration]

//! [calculate-discount]

double Subscription::calculateDiscount() const
{
    switch (m_duration) {
    case Monthly:
        return 1;
    case Quarterly:
        return 0.9;
    case Yearly:
        return 0.6;
    }
    Q_ASSERT(false);
    return -1;
}

//! [calculate-discount]

//! [calculate-base-price]

int Subscription::basePrice() const
{
    if (m_user->country() == User::Country::AnyTerritory)
        return 0;

    return (m_user->country() == User::Country::Norway) ? 100 : 80;
}

//! [calculate-base-price]

//! [update-validity]

void Subscription::updateValidity()
{
    bool isValid = m_isValid;
    m_isValid = m_user->country() != User::Country::AnyTerritory && m_user->age() > 12;

    if (m_isValid != isValid)
        emit isValidChanged();
}

//! [update-validity]
