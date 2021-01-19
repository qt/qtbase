/****************************************************************************
**
** Copyright (C) 2018 Intel Corporation.
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

#ifndef QPLUGIN_P_H
#define QPLUGIN_P_H

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

#include <private/qglobal_p.h>

QT_BEGIN_NAMESPACE

enum class QtPluginMetaDataKeys {
    QtVersion,
    Requirements,
    IID,
    ClassName,
    MetaData,
    URI
};

// F(IntKey, StringKey, Description)
// Keep this list sorted in the order moc should output.
#define QT_PLUGIN_FOREACH_METADATA(F) \
    F(QtPluginMetaDataKeys::IID, "IID", "Plugin's Interface ID")                \
    F(QtPluginMetaDataKeys::ClassName, "className", "Plugin class name")        \
    F(QtPluginMetaDataKeys::MetaData, "MetaData", "Other meta data")            \
    F(QtPluginMetaDataKeys::URI, "URI", "Plugin URI")

QT_END_NAMESPACE

#endif // QPLUGIN_P_H
