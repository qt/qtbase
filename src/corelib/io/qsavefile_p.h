/****************************************************************************
**
** Copyright (C) 2013 David Faure <faure@kde.org>
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
****************************************************************************/

#ifndef QSAVEFILE_P_H
#define QSAVEFILE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>

#ifndef QT_NO_TEMPORARYFILE

#include "private/qfiledevice_p.h"

QT_BEGIN_NAMESPACE

class QSaveFilePrivate : public QFileDevicePrivate
{
    Q_DECLARE_PUBLIC(QSaveFile)

protected:
    QSaveFilePrivate();
    ~QSaveFilePrivate();

    QString fileName;
    QString finalFileName; // fileName with symbolic links resolved

    QFileDevice::FileError writeError;

    bool useTemporaryFile;
    bool directWriteFallback;
};

QT_END_NAMESPACE

#endif // QT_NO_TEMPORARYFILE

#endif // QSAVEFILE_P_H
