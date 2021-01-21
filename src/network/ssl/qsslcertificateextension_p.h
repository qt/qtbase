/****************************************************************************
**
** Copyright (C) 2011 Richard J. Moore <rich@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#ifndef QSSLCERTIFICATEEXTENSION_P_H
#define QSSLCERTIFICATEEXTENSION_P_H

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

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include "qsslcertificateextension.h"

QT_BEGIN_NAMESPACE

class QSslCertificateExtensionPrivate : public QSharedData
{
public:
    inline QSslCertificateExtensionPrivate()
        : critical(false),
          supported(false)
    {
    }

    QString oid;
    QString name;
    QVariant value;
    bool critical;
    bool supported;
};

QT_END_NAMESPACE

#endif // QSSLCERTIFICATEEXTENSION_P_H

