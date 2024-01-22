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
        user.setCountry(User::Country::Germany);
    });
    QPushButton *finland = w.findChild<QPushButton *>("btnFinland");
    QObject::connect(finland, &QPushButton::clicked, &user, [&] {
        user.setCountry(User::Country::Finland);
    });
    QPushButton *norway = w.findChild<QPushButton *>("btnNorway");
    QObject::connect(norway, &QPushButton::clicked, &user, [&] {
        user.setCountry(User::Country::Norway);
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
        QLocale lc{QLocale::AnyLanguage, user.country()};
        priceDisplay->setText(lc.toCurrencyString(subscription.price() / subscription.duration()));
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
