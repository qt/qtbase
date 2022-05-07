/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qvariant.h"

#include "qsizepolicy.h"
#include "qwidget.h"

#include "private/qvariant_p.h"
#include <private/qmetatype_p.h>

QT_BEGIN_NAMESPACE

namespace {

static const struct : QMetaTypeModuleHelper
{
    const QtPrivate::QMetaTypeInterface *interfaceForType(int type) const override {
        switch (type) {
            QT_FOR_EACH_STATIC_WIDGETS_CLASS(QT_METATYPE_CONVERT_ID_TO_TYPE)
            default: return nullptr;
        }
    }
}  qVariantWidgetsHelper;


#undef QT_IMPL_METATYPEINTERFACE_WIDGETS_TYPES

}  // namespace

void qRegisterWidgetsVariant()
{
    qMetaTypeWidgetsHelper = &qVariantWidgetsHelper;
}
Q_CONSTRUCTOR_FUNCTION(qRegisterWidgetsVariant)

QT_END_NAMESPACE
