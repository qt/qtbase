// Copyright (C) 2016 David Faure <faure@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <qcommandlineparser.h>

int main(int argc, char **argv)
{

{
QCommandLineParser parser;
//! [0]
bool verbose = parser.isSet("verbose");
//! [0]
}

{
//! [1]
QCoreApplication app(argc, argv);
QCommandLineParser parser;
QCommandLineOption verboseOption("verbose");
parser.addOption(verboseOption);
parser.process(app);
bool verbose = parser.isSet(verboseOption);
//! [1]
}

{
QCommandLineParser parser;
//! [2]
// Usage: image-editor file
//
// Arguments:
//   file                  The file to open.
parser.addPositionalArgument("file", QCoreApplication::translate("main", "The file to open."));

// Usage: web-browser [urls...]
//
// Arguments:
//   urls                URLs to open, optionally.
parser.addPositionalArgument("urls", QCoreApplication::translate("main", "URLs to open, optionally."), "[urls...]");

// Usage: cp source destination
//
// Arguments:
//   source                Source file to copy.
//   destination           Destination directory.
parser.addPositionalArgument("source", QCoreApplication::translate("main", "Source file to copy."));
parser.addPositionalArgument("destination", QCoreApplication::translate("main", "Destination directory."));
//! [2]
}

{
//! [3]
QCoreApplication app(argc, argv);
QCommandLineParser parser;

parser.addPositionalArgument("command", "The command to execute.");

// Call parse() to find out the positional arguments.
parser.parse(QCoreApplication::arguments());

const QStringList args = parser.positionalArguments();
const QString command = args.isEmpty() ? QString() : args.first();
if (command == "resize") {
    parser.clearPositionalArguments();
    parser.addPositionalArgument("resize", "Resize the object to a new size.", "resize [resize_options]");
    parser.addOption(QCommandLineOption("size", "New size.", "new_size"));
    parser.process(app);
    // ...
}

/*
This code results in context-dependent help:

$ tool --help
Usage: tool command

Arguments:
  command  The command to execute.

$ tool resize --help
Usage: tool resize [resize_options]

Options:
  --size <size>  New size.

Arguments:
  resize         Resize the object to a new size.
*/
//! [3]
}

}
