// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QMainWindow>
#include <QScreen>
#include <QShortcut>
#include <QSurfaceFormat>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
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
    auto *closeShortcut = new QShortcut(Qt::CTRL | Qt::Key_Q, &a, QApplication::closeAllWindows);
    closeShortcut->setContext(Qt::ApplicationShortcut);

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
