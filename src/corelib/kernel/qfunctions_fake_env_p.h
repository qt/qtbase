/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QFUNCTIONS_FAKE_ENV_P_H
#define QFUNCTIONS_FAKE_ENV_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "qbytearray.h"
#include "qvector.h"

QT_BEGIN_NAMESPACE

// Environment ------------------------------------------------------
struct Variable {
    Variable() { }

    Variable(const QByteArray &name, const QByteArray &value)
        : name(name), value(value) { }

    QByteArray name;
    QByteArray value;
};

Q_DECLARE_TYPEINFO(Variable, Q_MOVABLE_TYPE);

struct NameEquals {
    typedef bool result_type;
    const char *name;
    explicit NameEquals(const char *name) Q_DECL_NOTHROW : name(name) {}
    result_type operator()(const Variable &other) const Q_DECL_NOTHROW
    { return qstrcmp(other.name, name) == 0; }
};

Q_GLOBAL_STATIC(QVector<Variable>, qt_app_environment)

errno_t qt_fake_getenv_s(size_t *sizeNeeded, char *buffer, size_t bufferSize, const char *varName)
{
    if (!sizeNeeded)
        return EINVAL;

    QVector<Variable>::const_iterator end = qt_app_environment->constEnd();
    QVector<Variable>::const_iterator iterator = std::find_if(qt_app_environment->constBegin(),
                                                              end,
                                                              NameEquals(varName));
    if (iterator == end) {
        if (buffer)
            buffer[0] = '\0';
        return ENOENT;
    }

    const int size = iterator->value.size() + 1;
    if (bufferSize < size_t(size)) {
        *sizeNeeded = size;
        return ERANGE;
    }

    qstrcpy(buffer, iterator->value.constData());
    return 0;
}

errno_t qt_fake__putenv_s(const char *varName, const char *value)
{
    QVector<Variable>::iterator end = qt_app_environment->end();
    QVector<Variable>::iterator iterator = std::find_if(qt_app_environment->begin(),
                                                        end,
                                                        NameEquals(varName));
    if (!value || !*value) {
        if (iterator != end)
            qt_app_environment->erase(iterator);
    } else {
        if (iterator == end)
            qt_app_environment->append(Variable(varName, value));
        else
            iterator->value = value;
    }

    return 0;
}

QT_END_NAMESPACE

#endif // QFUNCTIONS_FAKE_ENV_P_H
