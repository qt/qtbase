// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

/*
main.cpp

Provides the main function for the RSS news reader example.
*/

#include <QtWidgets>

#include "rsslisting.h"

/*!
    Create an application and a main widget. Open the main widget for
    user input, and exit with an appropriate return value when it is
    closed.
*/

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    RSSListing *rsslisting = new RSSListing;
    rsslisting->show();
    return app.exec();
}
