// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mailheader.h"
#include "window.h"

#include <QtWidgets/QApplication>

#include <QtCore/QRangeModel>

#include <array>

using namespace Qt::StringLiterals;

//! [0]
class MailModel : public QRangeModel
{
public:
    explicit MailModel(QObject *parent = nullptr) : QRangeModel(mails, parent) {}

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
            switch (section) {
            case 0:
                return Window::tr("Subject");
            case 1:
                return Window::tr("Sender");
            case 2:
                return Window::tr("Date");
            default:
                break;
            }
        }
        return QRangeModel::headerData(section, orientation, role);
    }

private:
    static const std::array<MailHeader, 10> mails;
};

const std::array<MailHeader, 10> MailModel::mails = {
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
//! [0]

//! [1]
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Window window;
    window.setSourceModel(new MailModel(&window));
    window.show();
    return QApplication::exec();
}
//! [1]
