// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>
#include "addressbook.h"

//! [main function]
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    AddressBook addressBook;
    addressBook.show();

    return app.exec();
}
//! [main function]
