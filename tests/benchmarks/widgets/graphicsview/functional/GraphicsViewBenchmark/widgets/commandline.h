// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include "settings.h"

bool readSettingsFromCommandLine(int argc,
                                 char *argv[],
                                 Settings& settings);


#endif // COMMANDLINE_H
