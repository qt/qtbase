/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QStringList>
#include <QDebug>

#include "commandline.h"

static void usage(const char *appname)
{
    Q_UNUSED(appname);
    printf(" GraphicsViewBenchmark related options:\n");
    printf(" -h,-help,--help: This help\n");
    printf(" -resolution    : UI resolution in format WxH where width and height are positive values\n");
    printf(" -opengl        : Enables OpenGL usage. Building PRECONDITIONS: QT_NO_OPENGL is off.\n");
    printf(" -manual        : Run test manually \n");
    printf("\n The following options are available in manual mode:\n");
    printf(" -rotation      : UI rotation in degrees\n");
    printf(" -subtree-cache : Enables usage of subtree caching method\n");
    printf(" -fps           : Output FPS count to stdout during application execution\n");
    printf(" -items         : Count of items created to the list\n");
    printf("\n");
}

static inline bool argumentOnlyAvailableInManualMode(const char *arg)
{
    return (strcmp(arg, "-rotation") == 0)
        || (strcmp(arg, "-subtree-cache") == 0)
        || (strcmp(arg, "-fps") == 0)
        || (strcmp(arg, "-items") == 0);
}

bool readSettingsFromCommandLine(int argc, char *argv[],
                  Settings& config)
{
    bool builtWithOpenGL = false;
    Settings::Options options;

#ifndef QT_NO_OPENGL
    builtWithOpenGL = true;
#endif
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-manual") == 0) {
            options |= Settings::ManualTest;
            argv[i] = 0;
            break;
        }
    }

    for (int i = 1; i < argc; ++i) {
        if (!argv[i])
            continue;
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return true;
        }
        if (strcmp(argv[i], "-opengl") == 0) {
            if (builtWithOpenGL) {
                options |= Settings::UseOpenGL;
                argv[i] = 0;
            } else {
                printf("-opengl parameter can be used only with building PRECONDITIONS: QT_NO_OPENGL is off.\n");
                usage(argv[0]);
                return false;
            }
        } else if (strcmp(argv[i], "-resolution") == 0) {
            if (i + 1 >= argc) {
                printf("-resolution needs an extra parameter specifying the application UI resolution\n");
                usage(argv[0]);
                return false;
            }
            else {
                QStringList res = QString(argv[i+1]).split("x");
                if (res.count() != 2) {
                    printf("-resolution parameter UI resolution should be set in format WxH where width and height are positive values\n");
                    usage(argv[0]);
                    return false;
                }
                int width = res.at(0).toInt();
                int height = res.at(1).toInt();

                config.setSize(QSize(width, height));

                if (width <=0 || height <=0) {
                    printf("-resolution parameter UI resolution should be set in format WxH where width and height are positive values\n");
                    usage(argv[0]);
                    return false;
                }
                argv[i] = 0;
                i++;
                argv[i] = 0;
            }
        }

        if (!argv[i])
            continue;

        if (!(options & Settings::ManualTest)) {
            if (argumentOnlyAvailableInManualMode(argv[i])) {
                printf("\nWrong option: '%s' is only available in manual mode\n\n", argv[i]);
                usage(argv[0]);
                return false;
            }
            continue;
        }

        if (strcmp(argv[i], "-rotation") == 0) {
            if (i + 1 >= argc) {
                printf("-rotation needs an extra parameter specifying the application UI rotation in degrees\n");
                usage(argv[0]);
                return false;
            }
            else {
                bool ok;
                int angle = QString(argv[i+1]).toInt(&ok);
                if (!ok) {
                    printf("-rotation parameter should specify rotation angle in degrees\n");
                    usage(argv[0]);
                    return false;
                }
                config.setAngle(angle);
                argv[i] = 0;
                i++;
                argv[i] = 0;
            }
        } else if (strcmp(argv[i], "-subtree-cache") == 0) {
            options |= Settings::UseListItemCache;
            argv[i] = 0;
        } else if (strcmp(argv[i], "-fps") == 0) {
             options |= Settings::OutputFps;
             argv[i] = 0;
        } else if (strcmp(argv[i], "-items") == 0) {
            if (i + 1 >= argc) {
                printf("-items needs an extra parameter specifying amount of list items\n");
                usage(argv[0]);
                return false;
            }
            else {
                bool ok;
                int amount = QString(argv[i+1]).toInt(&ok);
                if (!ok) {
                    printf("-items needs an extra parameter specifying amount (integer) of list items\n");
                    usage(argv[0]);
                    return false;
                }
                config.setListItemCount(amount);
                argv[i] = 0;
                i++;
                argv[i] = 0;
            }
        }
    }

    config.setOptions(options);

    return true;
}

