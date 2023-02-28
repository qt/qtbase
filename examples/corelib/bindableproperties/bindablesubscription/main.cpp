// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "../shared/subscriptionwindow.h"
#include "bindablesubscription.h"
#include "bindableuser.h"

#include <QApplication>
#include <QButtonGroup>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    BindableUser user;
    BindableSubscription subscription(&user);

    SubscriptionWindow w;

    // Initialize subscription data
    QRadioButton *monthly = w.findChild<QRadioButton *>("btnMonthly");
    QObject::connect(monthly, &QRadioButton::clicked, [&] {
        subscription.setDuration(BindableSubscription::Monthly);
    });
    QRadioButton *quarterly = w.findChild<QRadioButton *>("btnQuarterly");
    QObject::connect(quarterly, &QRadioButton::clicked, [&] {
        subscription.setDuration(BindableSubscription::Quarterly);
    });
    QRadioButton *yearly = w.findChild<QRadioButton *>("btnYearly");
    QObject::connect(yearly, &QRadioButton::clicked, [&] {
        subscription.setDuration(BindableSubscription::Yearly);
    });

    // Initialize user data
    QPushButton *germany = w.findChild<QPushButton *>("btnGermany");
    QObject::connect(germany, &QPushButton::clicked, [&] {
        user.setCountry(BindableUser::Country::Germany);
    });
    QPushButton *finland = w.findChild<QPushButton *>("btnFinland");
    QObject::connect(finland, &QPushButton::clicked, [&] {
        user.setCountry(BindableUser::Country::Finland);
    });
    QPushButton *norway = w.findChild<QPushButton *>("btnNorway");
    QObject::connect(norway, &QPushButton::clicked, [&] {
        user.setCountry(BindableUser::Country::Norway);
    });

    QSpinBox *ageSpinBox = w.findChild<QSpinBox *>("ageSpinBox");
    QObject::connect(ageSpinBox, &QSpinBox::valueChanged, [&](int value) {
        user.setAge(value);
    });

    QLabel *priceDisplay = w.findChild<QLabel *>("priceDisplay");

    // Track price changes
//! [update-ui]
    auto priceChangeHandler = subscription.bindablePrice().subscribe([&] {
        QLocale lc{QLocale::AnyLanguage, user.country()};
        priceDisplay->setText(lc.toCurrencyString(subscription.price() / subscription.duration()));
    });

    auto priceValidHandler = subscription.bindableIsValid().subscribe([&] {
        priceDisplay->setEnabled(subscription.isValid());
    });
//! [update-ui]

    w.show();
    return a.exec();
}
