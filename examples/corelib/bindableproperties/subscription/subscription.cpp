/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
