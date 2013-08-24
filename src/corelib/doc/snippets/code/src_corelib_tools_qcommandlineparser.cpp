/****************************************************************************
**
** Copyright (C) 2013 David Faure <faure@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
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

//! [0]
bool verbose = parser.isSet("verbose");
//! [0]

//! [1]
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
//! [1]

//! [2]
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

//! [2]

//! [3]
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("my-copy-program");
    app.setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.addHelpOption("Test helper");
    parser.addVersionOption();
    parser.addRemainingArgument("source", QCoreApplication::translate("main", "Source file to copy."));
    parser.addRemainingArgument("destination", QCoreApplication::translate("main", "Destination directory."));

    // A boolean option with a single name (-p)
    QCommandLineOption showProgressOption("p", QCoreApplication::translate("main", "Show progress during copy"));
    parser.addOption(showProgressOption);

    // A boolean option with multiple names (-f, --force)
    QCommandLineOption forceOption(QStringList() << "f" << "force", "Overwrite existing files.");
    parser.addOption(forceOption);

    // An option with a value
    QCommandLineOption targetDirectoryOption(QStringList() << "t" << "target-directory",
            QCoreApplication::translate("main", "Copy all source files into <directory>."),
            QCoreApplication::translate("main", "directory"));
    parser.addOption(targetDirectoryOption);

    // Process the actual command line arguments given by the user
    parser.process(app);

    const QStringList args = parser.remainingArguments();
    // source is args.at(0), destination is args.at(1)

    bool showProgress = parser.isSet(showProgressOption);
    bool force = parser.isSet(forceOption);
    QString targetDir = parser.value(targetDirectoryOption);
    // ...
}

//! [3]
