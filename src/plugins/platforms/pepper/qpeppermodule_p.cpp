/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt for Native Client platform plugin.
**
** $QT_BEGIN_LICENSE:LGPL3$
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
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qpeppermodule.h"

#include "qpepperinstance.h"
#include "qpeppermodule_p.h"

#include <ppapi/c/ppp.h>

static pp::Core *g_core = 0;
extern void *qtPepperModule; // QtCore

QPepperModulePrivate *qtCreatePepperModulePrivate(QPepperModule *module)
{
    return new QPepperModulePrivate(module);
}

QPepperModulePrivate::QPepperModulePrivate(QPepperModule *module)
    : q(module)
{
    qtPepperModule = module;
}

bool QPepperModulePrivate::init()
{
    g_core = q->core();
    return true;
}

pp::Instance *QPepperModulePrivate::createInstance(PP_Instance ppInstance)
{
    return new QPepperInstance(ppInstance);
}

pp::Core *QPepperModulePrivate::core() { return g_core; }
