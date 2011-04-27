/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
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
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QApplication>
#include <QMessageBox>
#include <QTimer>
#include <QWSServer>

#include "calibration.h"
#include "scribblewidget.h"

//! [0]
int main(int argc, char **argv)
{
    QApplication app(argc, argv, QApplication::GuiServer);

    if (!QWSServer::mouseHandler())
        qFatal("No mouse handler installed");

    {
        QMessageBox message;
        message.setText("<p>Please press once at each of the marks "
                        "shown in the next screen.</p>"
                        "<p>This messagebox will timout after 10 seconds "
                        "if you are unable to close it.</p>");
        QTimer::singleShot(10 * 1000, &message, SLOT(accept()));
        message.exec();
    }

//! [0] //! [1]
    Calibration cal;
    cal.exec();
//! [1]

//! [2]
    {
        QMessageBox message;
        message.setText("<p>The next screen will let you test the calibration "
                        "by drawing into a widget.</p><p>This program will "
                        "automatically close after 20 seconds.<p>");
        QTimer::singleShot(10 * 1000, &message, SLOT(accept()));
        message.exec();
    }

    ScribbleWidget scribble;
    scribble.showMaximized();
    scribble.show();

    app.setActiveWindow(&scribble);
    QTimer::singleShot(20 * 1000, &app, SLOT(quit()));

    return app.exec();
}
//! [2]

