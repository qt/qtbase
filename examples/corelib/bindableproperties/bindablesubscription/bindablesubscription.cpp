// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "bindablesubscription.h"
#include "bindableuser.h"

//! [binding-expressions]

BindableSubscription::BindableSubscription(BindableUser *user) : m_user(user)
{
    Q_ASSERT(user);

    m_price.setBinding(
            [this] { return qRound(calculateDiscount() * int(m_duration) * basePrice()); });

    m_isValid.setBinding([this] {
        return m_user->country() != BindableUser::Country::AnyCountry && m_user->age() > 12;
    });
}

//! [binding-expressions]

//! [set-duration]

void BindableSubscription::setDuration(Duration newDuration)
{
    m_duration = newDuration;
}

//! [set-duration]

double BindableSubscription::calculateDiscount() const
{
    switch (m_duration) {
    case Monthly:
        return 1;
    case Quarterly:
        return 0.9;
    case Yearly:
        return 0.6;
    }
    Q_UNREACHABLE_RETURN(-1);
}

int BindableSubscription::basePrice() const
{
    if (m_user->country() == BindableUser::Country::AnyCountry)
        return 0;

    return (m_user->country() == BindableUser::Country::Norway) ? 100 : 80;
}
