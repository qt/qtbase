// Copyright (C) 2016 David Faure <faure@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QCommandLineOption>
#include <QCommandLineParser>

int main()
{

//! [0]
QCommandLineOption verboseOption("verbose", "Verbose mode. Prints out more information.");
QCommandLineOption outputOption(QStringList() << "o" << "output", "Write generated data into <file>.", "file");
//! [0]

//! [cxx11-init]
QCommandLineParser parser;
parser.addOption({"verbose", "Verbose mode. Prints out more information."});
//! [cxx11-init]

//! [cxx11-init-list]
QCommandLineParser parser;
parser.addOption({{"o", "output"}, "Write generated data into <file>.", "file"});
//! [cxx11-init-list]

}
