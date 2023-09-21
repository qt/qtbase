// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef BINDABLESUBSCRIPTION_H
#define BINDABLESUBSCRIPTION_H

#include <QBindable>
#include <QProperty>

class BindableUser;

//! [bindable-subscription-class]

class BindableSubscription
{
public:
    enum Duration { Monthly = 1, Quarterly = 3, Yearly = 12 };

    BindableSubscription(BindableUser *user);
    BindableSubscription(const BindableSubscription &) = delete;

    int price() const { return m_price; }
    QBindable<int> bindablePrice() { return &m_price; }

    Duration duration() const { return m_duration; }
    void setDuration(Duration newDuration);
    QBindable<Duration> bindableDuration() { return &m_duration; }

    bool isValid() const { return m_isValid; }
    QBindable<bool> bindableIsValid() { return &m_isValid; }

private:
    double calculateDiscount() const;
    int basePrice() const;

    BindableUser *m_user;
    QProperty<Duration> m_duration { Monthly };
    QProperty<int> m_price { 0 };
    QProperty<bool> m_isValid { false };
};

//! [bindable-subscription-class]

#endif // BNDABLESUBSCRIPTION_H
