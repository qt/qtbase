/****************************************************************************
**
** Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qshaderlanguage_p.h"

#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

// Note: to be invoked explicitly. Relying for example on
// Q_COREAPP_STARTUP_FUNCTION would not be acceptable in static builds.
void qt_register_ShaderLanguage_enums()
{
    qRegisterMetaType<QShaderLanguage::StorageQualifier>();
    qRegisterMetaType<QShaderLanguage::VariableType>();
}

QT_END_NAMESPACE
