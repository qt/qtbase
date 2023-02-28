// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SUBSCRIPTION_H
#define SUBSCRIPTION_H

#include <QObject>
#include <QPointer>

class User;

//! [subscription-class]

class Subscription : public QObject
{
    Q_OBJECT
public:
    enum Duration { Monthly = 1, Quarterly = 3, Yearly = 12 };

    Subscription(User *user);

    void calculatePrice();
    int price() const { return m_price; }

    Duration duration() const { return m_duration; }
    void setDuration(Duration newDuration);

    bool isValid() const { return m_isValid; }
    void updateValidity();

signals:
    void priceChanged();
    void durationChanged();
    void isValidChanged();

private:
    double calculateDiscount() const;
    int basePrice() const;

    QPointer<User> m_user;
    Duration m_duration = Monthly;
    int m_price = 0;
    bool m_isValid = false;
};

//! [subscription-class]

#endif // SUBSCRIPTION_H
