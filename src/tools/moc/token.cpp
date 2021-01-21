/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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
****************************************************************************/

#include "token.h"

QT_BEGIN_NAMESPACE

#if defined(DEBUG_MOC)
const char *tokenTypeName(Token t)
{
    switch (t) {
#define CREATE_CASE(Name) case Name: return #Name;
    FOR_ALL_TOKENS(CREATE_CASE)
#undef CREATE_CASE
    }
    return "";
}
#endif

QT_END_NAMESPACE
