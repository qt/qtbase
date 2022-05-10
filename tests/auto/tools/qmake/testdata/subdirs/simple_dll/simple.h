// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#ifndef SIMPLE_H
#define SIMPLE_H

#include <qstring.h>

#ifdef SIMPLEDLL_MAKEDLL
# define SIMPLEDLL_EXPORT Q_DECL_EXPORT
#else
# define SIMPLEDLL_EXPORT Q_DECL_IMPORT
#endif

class SIMPLEDLL_EXPORT Simple
{
public:
    Simple();
    ~Simple();

    QString test();
};

#endif



