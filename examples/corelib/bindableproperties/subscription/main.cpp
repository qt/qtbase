// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "../shared/subscriptionwindow.h"
#include "subscription.h"
#include "user.h"

#include <QApplication>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QString>

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

//! [init]
    User user;
    Subscription subscription(&user);
//! [init]

    SubscriptionWindow w;

    // Initialize subscription data
    QRadioButton *monthly = w.findChild<QRadioButton *>(u"btnMonthly"_s);
    QObject::connect(monthly, &QRadioButton::clicked, &subscription, [&] {
        subscription.setDuration(Subscription::Monthly);
    });
    QRadioButton *quarterly = w.findChild<QRadioButton *>(u"btnQuarterly"_s);
    QObject::connect(quarterly, &QRadioButton::clicked, &subscription, [&] {
        subscription.setDuration(Subscription::Quarterly);
    });
    QRadioButton *yearly = w.findChild<QRadioButton *>(u"btnYearly"_s);
    QObject::connect(yearly, &QRadioButton::clicked, &subscription, [&] {
        subscription.setDuration(Subscription::Yearly);
    });

    // Initialize user data
    QPushButton *germany = w.findChild<QPushButton *>(u"btnGermany"_s);
    QObject::connect(germany, &QPushButton::clicked, &user, [&] {
        user.setCountry(User::Country::Germany);
    });
    QPushButton *finland = w.findChild<QPushButton *>(u"btnFinland"_s);
    QObject::connect(finland, &QPushButton::clicked, &user, [&] {
        user.setCountry(User::Country::Finland);
    });
    QPushButton *norway = w.findChild<QPushButton *>(u"btnNorway"_s);
    QObject::connect(norway, &QPushButton::clicked, &user, [&] {
        user.setCountry(User::Country::Norway);
    });

    QSpinBox *ageSpinBox = w.findChild<QSpinBox *>(u"ageSpinBox"_s);
    QObject::connect(ageSpinBox, &QSpinBox::valueChanged, &user, [&](int value) {
        user.setAge(value);
    });

    // Initialize price data
    QLabel *priceDisplay = w.findChild<QLabel *>(u"priceDisplay"_s);
    priceDisplay->setText(QString::number(subscription.price()));
    priceDisplay->setEnabled(subscription.isValid());

    // Track the price changes

//! [connect-price-changed]
    QObject::connect(&subscription, &Subscription::priceChanged, priceDisplay, [&] {
        QLocale lc{QLocale::AnyLanguage, user.country()};
        priceDisplay->setText(lc.toCurrencyString(subscription.price() / subscription.duration()));
    });
//! [connect-price-changed]

//! [connect-validity-changed]
    QObject::connect(&subscription, &Subscription::isValidChanged, priceDisplay, [&] {
        priceDisplay->setEnabled(subscription.isValid());
    });
//! [connect-validity-changed]

//! [connect-user]
    QObject::connect(&user, &User::countryChanged, &subscription, [&] {
        subscription.calculatePrice();
        subscription.updateValidity();
    });

    QObject::connect(&user, &User::ageChanged, &subscription, [&] {
        subscription.updateValidity();
    });
//! [connect-user]

    w.show();
    return a.exec();
}
