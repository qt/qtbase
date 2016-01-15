/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef PLUGININTERFACE1_H
#define PLUGININTERFACE1_H

#include <QtCore/QtGlobal>

struct PluginInterface1 {
    virtual ~PluginInterface1() {}
    virtual QString pluginName() const = 0;
};

QT_BEGIN_NAMESPACE

#define PluginInterface1_iid "org.qt-project.Qt.autotests.plugininterface1"

Q_DECLARE_INTERFACE(PluginInterface1, PluginInterface1_iid)

QT_END_NAMESPACE

#endif // PLUGININTERFACE1_H
