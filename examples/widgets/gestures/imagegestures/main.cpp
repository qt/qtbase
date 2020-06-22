/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QtWidgets>

#include "mainwidget.h"

static void showHelp(QCommandLineParser &parser, const QString errorMessage = QString())
{
    QString text;
    QTextStream str(&text);
    str << "<html><head/><body>";
    if (!errorMessage.isEmpty())
        str << "<p>" << errorMessage << "</p>";
    str << "<pre>" << parser.helpText() << "</pre></body></html>";
    QMessageBox box(errorMessage.isEmpty() ? QMessageBox::Information : QMessageBox::Warning,
                    QGuiApplication::applicationDisplayName(), text,
                    QMessageBox::Ok);
    box.setTextInteractionFlags(Qt::TextBrowserInteraction);
    box.exec();
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);
    QCoreApplication::setApplicationName(QStringLiteral("imagegestures"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("Image Gestures Example"));

    QCommandLineParser commandLineParser;
    const QCommandLineOption disablePanOption("no-pan", "Disable pan gesture");
    commandLineParser.addOption(disablePanOption);
    const QCommandLineOption disablePinchOption("no-pinch", "Disable pinch gesture");
    commandLineParser.addOption(disablePinchOption);
    const QCommandLineOption disableSwipeOption("no-swipe", "Disable swipe gesture");
    commandLineParser.addOption(disableSwipeOption);
    const QCommandLineOption helpOption = commandLineParser.addHelpOption();
    commandLineParser.addPositionalArgument(QStringLiteral("Directory"),
                                            QStringLiteral("Directory to display"));

    const QString description = QGuiApplication::applicationDisplayName()
        + QLatin1String("\n\nEnable \"debug\" on the logging category \"qt.examples.imagegestures\" in order to\n"
                        "in order to obtain verbose information about Qt's gesture event processing,\n"
                        "for example by setting the environment variables QT_LOGGING_RULES to\n"
                        "qt.examples.imagegestures.debug=true\n");
    commandLineParser.setApplicationDescription(description);

    commandLineParser.process(QCoreApplication::arguments());

    QStringList arguments = commandLineParser.positionalArguments();
    if (!arguments.isEmpty() && !QFileInfo(arguments.front()).isDir()) {
        showHelp(commandLineParser,
                 QLatin1Char('"') + QDir::toNativeSeparators(arguments.front())
                 + QStringLiteral("\" is not a directory."));
        return -1;
    }

    QList<Qt::GestureType> gestures;
    if (!commandLineParser.isSet(disablePanOption))
        gestures << Qt::PanGesture;
    if (!commandLineParser.isSet(disablePinchOption))
        gestures << Qt::PinchGesture;
    if (!commandLineParser.isSet(disableSwipeOption))
        gestures << Qt::SwipeGesture;

    MainWidget w;
    w.grabGestures(gestures);
    w.show();

    if (arguments.isEmpty()) {
        const QStringList picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
        const QString directory =
            QFileDialog::getExistingDirectory(&w, "Select image folder",
                                              picturesLocations.isEmpty() ? QString() : picturesLocations.front());
        if (directory.isEmpty())
            return 0;
        arguments.append(directory);
    }

    w.openDirectory(arguments.front());

    return app.exec();
}
