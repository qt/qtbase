/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
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

#include "mainwindow.h"

#include <QApplication>
#include <QPainterPath>
#include <QPainter>
#include <QMap>
#include <QDebug>

void render_qt_text(QPainter *painter, int w, int h, const QColor &color)
{
    QPainterPath path;
    path.moveTo(-0.083695, 0.283849);
    path.cubicTo(-0.049581, 0.349613, -0.012720, 0.397969, 0.026886, 0.428917);
    path.cubicTo(0.066493, 0.459865, 0.111593, 0.477595, 0.162186, 0.482108);
    path.lineTo(0.162186, 0.500000);
    path.cubicTo(0.115929, 0.498066, 0.066565, 0.487669, 0.014094, 0.468810);
    path.cubicTo(-0.038378, 0.449952, -0.088103, 0.423839, -0.135082, 0.390474);
    path.cubicTo(-0.182061, 0.357108, -0.222608, 0.321567, -0.256722, 0.283849);
    path.cubicTo(-0.304712, 0.262250, -0.342874, 0.239362, -0.371206, 0.215184);
    path.cubicTo(-0.411969, 0.179078, -0.443625, 0.134671, -0.466175, 0.081963);
    path.cubicTo(-0.488725, 0.029255, -0.500000, -0.033043, -0.500000, -0.104932);
    path.cubicTo(-0.500000, -0.218407, -0.467042, -0.312621, -0.401127, -0.387573);
    path.cubicTo(-0.335212, -0.462524, -0.255421, -0.500000, -0.161752, -0.500000);
    path.cubicTo(-0.072998, -0.500000, 0.003903, -0.462444, 0.068951, -0.387331);
    path.cubicTo(0.133998, -0.312218, 0.166522, -0.217440, 0.166522, -0.102998);
    path.cubicTo(0.166522, -0.010155, 0.143394, 0.071325, 0.097138, 0.141441);
    path.cubicTo(0.050882, 0.211557, -0.009396, 0.259026, -0.083695, 0.283849);
    path.moveTo(-0.167823, -0.456963);
    path.cubicTo(-0.228823, -0.456963, -0.277826, -0.432624, -0.314831, -0.383946);
    path.cubicTo(-0.361665, -0.323340, -0.385082, -0.230335, -0.385082, -0.104932);
    path.cubicTo(-0.385082, 0.017569, -0.361376, 0.112025, -0.313964, 0.178433);
    path.cubicTo(-0.277248, 0.229368, -0.228534, 0.254836, -0.167823, 0.254836);
    path.cubicTo(-0.105088, 0.254836, -0.054496, 0.229368, -0.016045, 0.178433);
    path.cubicTo(0.029055, 0.117827, 0.051605, 0.028691, 0.051605, -0.088975);
    path.cubicTo(0.051605, -0.179562, 0.039318, -0.255803, 0.014744, -0.317698);
    path.cubicTo(-0.004337, -0.365409, -0.029705, -0.400548, -0.061362, -0.423114);
    path.cubicTo(-0.093018, -0.445680, -0.128505, -0.456963, -0.167823, -0.456963);
    path.moveTo(0.379011, -0.404739);
    path.lineTo(0.379011, -0.236460);
    path.lineTo(0.486123, -0.236460);
    path.lineTo(0.486123, -0.197292);
    path.lineTo(0.379011, -0.197292);
    path.lineTo(0.379011, 0.134913);
    path.cubicTo(0.379011, 0.168117, 0.383276, 0.190442, 0.391804, 0.201886);
    path.cubicTo(0.400332, 0.213330, 0.411246, 0.219052, 0.424545, 0.219052);
    path.cubicTo(0.435531, 0.219052, 0.446227, 0.215264, 0.456635, 0.207689);
    path.cubicTo(0.467042, 0.200113, 0.474993, 0.188910, 0.480486, 0.174081);
    path.lineTo(0.500000, 0.174081);
    path.cubicTo(0.488436, 0.210509, 0.471957, 0.237911, 0.450564, 0.256286);
    path.cubicTo(0.429170, 0.274662, 0.407054, 0.283849, 0.384215, 0.283849);
    path.cubicTo(0.368893, 0.283849, 0.353859, 0.279094, 0.339115, 0.269584);
    path.cubicTo(0.324371, 0.260074, 0.313530, 0.246534, 0.306592, 0.228965);
    path.cubicTo(0.299653, 0.211396, 0.296184, 0.184075, 0.296184, 0.147002);
    path.lineTo(0.296184, -0.197292);
    path.lineTo(0.223330, -0.197292);
    path.lineTo(0.223330, -0.215667);
    path.cubicTo(0.241833, -0.224049, 0.260697, -0.237992, 0.279922, -0.257495);
    path.cubicTo(0.299147, -0.276999, 0.316276, -0.300129, 0.331310, -0.326886);
    path.cubicTo(0.338826, -0.341070, 0.349523, -0.367021, 0.363400, -0.404739);
    path.lineTo(0.379011, -0.404739);
    path.moveTo(-0.535993, 0.275629);

    painter->translate(w / 2, h / 2);
    double scale = qMin(w, h) * 8 / 10.0;
    painter->scale(scale, scale);

    painter->setRenderHint(QPainter::Antialiasing);

    painter->save();
    painter->translate(.1, .1);
    painter->fillPath(path, QColor(0, 0, 0, 63));
    painter->restore();

    painter->setBrush(color);
    painter->setPen(QPen(Qt::black, 0.02, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
    painter->drawPath(path);
}

static void usage()
{
    qWarning() << "Usage: mainwindow [-SizeHint<color> <width>x<height>] ...";
    exit(1);
}

enum ParseCommandLineArgumentsResult {
    CommandLineArgumentsOk,
    CommandLineArgumentsError,
    HelpRequested
};

static ParseCommandLineArgumentsResult
    parseCustomSizeHints(const QStringList &arguments, MainWindow::CustomSizeHintMap *result)
{
    result->clear();
    const int argumentCount = arguments.size();
    for (int i = 1; i < argumentCount; ++i) {
        const QString &arg = arguments.at(i);
        if (arg.startsWith(QLatin1String("-SizeHint"))) {
            const QString name = arg.mid(9);
            if (name.isEmpty())
                return CommandLineArgumentsError;
            if (++i == argumentCount)
                return CommandLineArgumentsError;
            const QString sizeStr = arguments.at(i);
            const int idx = sizeStr.indexOf(QLatin1Char('x'));
            if (idx == -1)
                return CommandLineArgumentsError;
            bool ok;
            const int w = sizeStr.leftRef(idx).toInt(&ok);
            if (!ok)
                return CommandLineArgumentsError;
            const int h = sizeStr.midRef(idx + 1).toInt(&ok);
            if (!ok)
                return CommandLineArgumentsError;
            result->insert(name, QSize(w, h));
        } else if (arg == QLatin1String("-h") || arg == QLatin1String("--help")) {
            return HelpRequested;
        } else {
            return CommandLineArgumentsError;
        }
    }

    return CommandLineArgumentsOk;
}

int main(int argc, char **argv)
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);
    MainWindow::CustomSizeHintMap customSizeHints;
    switch (parseCustomSizeHints(QCoreApplication::arguments(), &customSizeHints)) {
    case CommandLineArgumentsOk:
        break;
    case CommandLineArgumentsError:
        usage();
        return -1;
    case HelpRequested:
        usage();
        return 0;
    }
    MainWindow mainWin(customSizeHints);
    mainWin.resize(800, 600);
    mainWin.show();
    return app.exec();
}
