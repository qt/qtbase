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

#include <QApplication>
#include <QMainWindow>
#include <QScreen>
#include <QSurfaceFormat>
#include <QOpenGLContext>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include "mainwindow.h"
#include "glwidget.h"

#include <memory>

static QString getGlString(QOpenGLFunctions *functions, GLenum name)
{
    if (const GLubyte *p = functions->glGetString(name))
        return QString::fromLatin1(reinterpret_cast<const char *>(p));
    return QString();
}

int main( int argc, char ** argv )
{
    QApplication a( argc, argv );

    QCoreApplication::setApplicationName("Qt Threaded QOpenGLWidget Example");
    QCoreApplication::setOrganizationName("QtProject");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);
    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::applicationName());
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption singleOption("single", "Single thread");
    parser.addOption(singleOption);
    parser.process(a);

    QSurfaceFormat format;
    format.setDepthBufferSize(16);
    QSurfaceFormat::setDefaultFormat(format);

    // Two top-level windows with two QOpenGLWidget children in each.
    // The rendering for the four QOpenGLWidgets happens on four separate threads.

    GLWidget topLevelGlWidget;
    QPoint pos = topLevelGlWidget.screen()->availableGeometry().topLeft() + QPoint(200, 200);
    topLevelGlWidget.setWindowTitle(QStringLiteral("Threaded QOpenGLWidget example top level"));
    topLevelGlWidget.resize(200, 200);
    topLevelGlWidget.move(pos);
    topLevelGlWidget.show();

    const QString glInfo = getGlString(topLevelGlWidget.context()->functions(), GL_VENDOR)
        + QLatin1Char('/') + getGlString(topLevelGlWidget.context()->functions(), GL_RENDERER);

    const bool supportsThreading = !glInfo.contains(QLatin1String("nouveau"), Qt::CaseInsensitive)
        && !glInfo.contains(QLatin1String("ANGLE"), Qt::CaseInsensitive)
        && !glInfo.contains(QLatin1String("llvmpipe"), Qt::CaseInsensitive);

    const QString toolTip = supportsThreading ? glInfo : glInfo + QStringLiteral("\ndoes not support threaded OpenGL.");
    topLevelGlWidget.setToolTip(toolTip);

    std::unique_ptr<MainWindow> mw1;
    std::unique_ptr<MainWindow> mw2;
    if (!parser.isSet(singleOption)) {
        if (supportsThreading) {
            pos += QPoint(100, 100);
            mw1.reset(new MainWindow);
            mw1->setToolTip(toolTip);
            mw1->move(pos);
            mw1->setWindowTitle(QStringLiteral("Threaded QOpenGLWidget example #1"));
            mw1->show();
            pos += QPoint(100, 100);
            mw2.reset(new MainWindow);
            mw2->setToolTip(toolTip);
            mw2->move(pos);
            mw2->setWindowTitle(QStringLiteral("Threaded QOpenGLWidget example #2"));
            mw2->show();
        } else {
            qWarning() << toolTip;
        }
    }

    return a.exec();
}
