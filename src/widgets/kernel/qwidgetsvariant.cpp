// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvariant.h"

#include "qsizepolicy.h"
#include "qwidget.h"

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
