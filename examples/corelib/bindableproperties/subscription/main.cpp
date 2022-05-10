// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "../shared/subscriptionwindow.h"
#include "subscription.h"
#include "user.h"

#include <QApplication>
#include <QButtonGroup>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

//! [init]
    User user;
    Subscription subscription(&user);
//! [init]

    SubscriptionWindow w;

    // Initialize subscription data
    QRadioButton *monthly = w.findChild<QRadioButton *>("btnMonthly");
    QObject::connect(monthly, &QRadioButton::clicked, &subscription, [&] {
        subscription.setDuration(Subscription::Monthly);
    });
    QRadioButton *quarterly = w.findChild<QRadioButton *>("btnQuarterly");
    QObject::connect(quarterly, &QRadioButton::clicked, &subscription, [&] {
        subscription.setDuration(Subscription::Quarterly);
    });
    QRadioButton *yearly = w.findChild<QRadioButton *>("btnYearly");
    QObject::connect(yearly, &QRadioButton::clicked, &subscription, [&] {
        subscription.setDuration(Subscription::Yearly);
    });

    // Initialize user data
    QPushButton *germany = w.findChild<QPushButton *>("btnGermany");
    QObject::connect(germany, &QPushButton::clicked, &user, [&] {
        user.setCountry(User::Germany);
    });
    QPushButton *finland = w.findChild<QPushButton *>("btnFinland");
    QObject::connect(finland, &QPushButton::clicked, &user, [&] {
        user.setCountry(User::Finland);
    });
    QPushButton *norway = w.findChild<QPushButton *>("btnNorway");
    QObject::connect(norway, &QPushButton::clicked, &user, [&] {
        user.setCountry(User::Norway);
    });

    QSpinBox *ageSpinBox = w.findChild<QSpinBox *>("ageSpinBox");
    QObject::connect(ageSpinBox, &QSpinBox::valueChanged, &user, [&](int value) {
        user.setAge(value);
    });

    // Initialize price data
    QLabel *priceDisplay = w.findChild<QLabel *>("priceDisplay");
    priceDisplay->setText(QString::number(subscription.price()));
    priceDisplay->setEnabled(subscription.isValid());

    // Track the price changes

//! [connect-price-changed]
    QObject::connect(&subscription, &Subscription::priceChanged, [&] {
        priceDisplay->setText(QString::number(subscription.price()));
    });
//! [connect-price-changed]

//! [connect-validity-changed]
    QObject::connect(&subscription, &Subscription::isValidChanged, [&] {
        priceDisplay->setEnabled(subscription.isValid());
    });
//! [connect-validity-changed]

//! [connect-user]
    QObject::connect(&user, &User::countryChanged, [&] {
        subscription.calculatePrice();
        subscription.updateValidity();
    });

    QObject::connect(&user, &User::ageChanged, [&] {
        subscription.updateValidity();
    });
//! [connect-user]

    w.show();
    return a.exec();
}
