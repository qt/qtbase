/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPROPERTYANIMATION_P_H
#define QPROPERTYANIMATION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qpropertyanimation.h"

#include "private/qvariantanimation_p.h"
#include "private/qproperty_p.h"

QT_REQUIRE_CONFIG(animation);

QT_BEGIN_NAMESPACE

class QPropertyAnimationPrivate : public QVariantAnimationPrivate
{
   Q_DECLARE_PUBLIC(QPropertyAnimation)
public:
    QPropertyAnimationPrivate() : propertyType(0), propertyIndex(-1) { }

    void setTargetObjectForwarder(QObject *target) { q_func()->setTargetObject(target); }
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QPropertyAnimationPrivate, QObject *, targetObject,
                                       &QPropertyAnimationPrivate::setTargetObjectForwarder,
                                       nullptr)
    void targetObjectDestroyed()
    {
        // stop() has to be called before targetObject is set to nullptr.
        // targetObject must not be nullptr in states unequal to "Stopped".
        q_func()->stop();
        targetObject.setValueBypassingBindings(nullptr);
        targetObject.notify();
    }

    //for the QProperty
    int propertyType;
    int propertyIndex;

    void setPropertyName(const QByteArray &propertyName)
    {
        q_func()->setPropertyName(propertyName);
    }
    Q_OBJECT_COMPAT_PROPERTY(QPropertyAnimationPrivate, QByteArray, propertyName,
                             &QPropertyAnimationPrivate::setPropertyName)
    void updateProperty(const QVariant &);
    void updateMetaProperty();
};

QT_END_NAMESPACE

#endif //QPROPERTYANIMATION_P_H
