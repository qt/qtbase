// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "../shared/subscriptionwindow.h"
#include "bindablesubscription.h"
#include "bindableuser.h"

#include <QApplication>
#include <QBindable>
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
    BindableUser user;
    BindableSubscription subscription(&user);

    SubscriptionWindow w;
    // clazy:excludeall=lambda-in-connect
    // when subscription is out of scope so is window

    // Initialize subscription data
    QRadioButton *monthly = w.findChild<QRadioButton *>(u"btnMonthly"_s);
    QObject::connect(monthly, &QRadioButton::clicked, monthly, [&] {
        subscription.setDuration(BindableSubscription::Monthly);
    });
    QRadioButton *quarterly = w.findChild<QRadioButton *>(u"btnQuarterly"_s);
    QObject::connect(quarterly, &QRadioButton::clicked, quarterly, [&] {
        subscription.setDuration(BindableSubscription::Quarterly);
    });
    QRadioButton *yearly = w.findChild<QRadioButton *>(u"btnYearly"_s);
    QObject::connect(yearly, &QRadioButton::clicked, yearly, [&] {
        subscription.setDuration(BindableSubscription::Yearly);
    });

    // Initialize user data
    QPushButton *germany = w.findChild<QPushButton *>(u"btnGermany"_s);
    QObject::connect(germany, &QPushButton::clicked, germany, [&] {
        user.setCountry(BindableUser::Country::Germany);
    });
    QPushButton *finland = w.findChild<QPushButton *>(u"btnFinland"_s);
    QObject::connect(finland, &QPushButton::clicked, finland, [&] {
        user.setCountry(BindableUser::Country::Finland);
    });
    QPushButton *norway = w.findChild<QPushButton *>(u"btnNorway"_s);
    QObject::connect(norway, &QPushButton::clicked, norway, [&] {
        user.setCountry(BindableUser::Country::Norway);
    });

    QSpinBox *ageSpinBox = w.findChild<QSpinBox *>(u"ageSpinBox"_s);
    QBindable<int> ageBindable(ageSpinBox, "value");
    user.bindableAge().setBinding([ageBindable](){ return ageBindable.value();});

    QLabel *priceDisplay = w.findChild<QLabel *>(u"priceDisplay"_s);

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
