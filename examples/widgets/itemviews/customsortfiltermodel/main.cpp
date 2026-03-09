// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "window.h"

#include <QtWidgets/QApplication>

#include <QtGui/QStandardItemModel>

#include <QtCore/QTime>

#include <array>

using namespace Qt::StringLiterals;

struct MailHeader
{
    QString subject;
    QString sender;
    QDateTime date;
};

static const std::array<MailHeader, 10> mails = {
    MailHeader{ u"RE: Sports"_s, u"Petra Schmidt <petras@nospam.com>"_s,
                QDateTime(QDate(2007, 01, 05), QTime(12, 01)) },
    MailHeader{ u"AW: Sports"_s, u"Rolf Newschweinstein <rolfn@nospam.com>"_s,
                QDateTime(QDate(2007, 01, 05), QTime(12, 00)) },
    MailHeader{ u"Sports"_s, u"Linda Smith <linda.smith@nospam.com>"_s,
                QDateTime(QDate(2007, 01, 05), QTime(11, 33)) },
    MailHeader{ u"Re: Accounts"_s, u"Andy <andy@nospam.com>"_s,
                QDateTime(QDate(2007, 01, 03), QTime(14, 26)) },
    MailHeader{ u"Re: Accounts"_s, u"Joe Bloggs <joe@bloggs.com>"_s,
                QDateTime(QDate(2007, 01, 03), QTime(14, 18)) },
    MailHeader{ u"Re: Expenses"_s, u"Andy <andy@nospam.com>"_s,
                QDateTime(QDate(2007, 01, 02), QTime(16, 05)) },
    MailHeader{ u"Expenses"_s, u"Joe Bloggs <joe@bloggs.com>"_s,
                QDateTime(QDate(2006, 12, 25), QTime(11, 39)) },
    MailHeader{ u"Accounts"_s, u"pascale@nospam.com"_s,
                QDateTime(QDate(2006, 12, 31), QTime(12, 50)) },
    MailHeader{ u"Radically new concept"_s, u"Grace K. <grace@software-inc.com>"_s,
                QDateTime(QDate(2006, 12, 22), QTime(9, 44)) },
    MailHeader{ u"Happy New Year!"_s, u"Grace K. <grace@software-inc.com>"_s,
                QDateTime(QDate(2006, 12, 31), QTime(17, 03)) }
};

QAbstractItemModel *createMailModel(QObject *parent)
{
    auto *model = new QStandardItemModel(0, 3, parent);

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("Subject"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Sender"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Date"));

    for (const auto &mh : mails) {
        QList<QStandardItem *> row = { new QStandardItem(mh.subject),
                                       new QStandardItem(mh.sender),
                                       new QStandardItem };
        row[2]->setData(QVariant::fromValue(mh.date), Qt::DisplayRole);
        model->appendRow(row);
    }

    return model;
}

//! [0]
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Window window;
    window.setSourceModel(createMailModel(&window));
    window.show();
    return QCoreApplication::exec();
}
//! [0]
