// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef C_COMMENTS_H
#define C_COMMENTS_H
#include <qobject.h>

/* test support for multi-line comments in preprocessor statements */

#if 0 /* comment starts here
       ends here */ || defined(Q_MOC_RUN) || 1

class IfdefedClass : public QObject
{
    Q_OBJECT
public:
    inline IfdefedClass() {}
};

#endif
#endif // C_COMMENTS_H
