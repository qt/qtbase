/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QLIBRARYINFO_P_H
#define QLIBRARYINFO_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qlibraryinfo.h"

#if QT_CONFIG(settings)
#    include "QtCore/qsettings.h"
#endif
#include "QtCore/qstring.h"

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QLibraryInfoPrivate final
{
public:
#if QT_CONFIG(settings)
    static QSettings *configuration();
    static void reload();
    static QString qtconfManualPath;
#endif
    static void keyAndDefault(QLibraryInfo::LibraryPath loc, QString *key,
                                                  QString *value);
};

QT_END_NAMESPACE

#endif // QLIBRARYINFO_P_H
