// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#define BUILD_LIBRARY
#include <qstring.h>
#include "qctfplugin_p.h"
#include "qctflib_p.h"

QT_BEGIN_NAMESPACE

class QCtfTracePlugin : public QObject, public QCtfLib
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QCtfLib" FILE "trace.json")
    Q_INTERFACES(QCtfLib)

public:
    QCtfTracePlugin()
    {

    }
    ~QCtfTracePlugin() = default;

    bool tracepointEnabled(const QCtfTracePointEvent &point) override
    {
        return QCtfLibImpl::instance()->tracepointEnabled(point);
    }
    void doTracepoint(const QCtfTracePointEvent &point, const QByteArray &arr) override
    {
        QCtfLibImpl::instance()->doTracepoint(point, arr);
    }
    bool sessionEnabled() override
    {
        return QCtfLibImpl::instance()->sessionEnabled();
    }
    QCtfTracePointPrivate *initializeTracepoint(const QCtfTracePointEvent &point) override
    {
        return QCtfLibImpl::instance()->initializeTracepoint(point);
    }
};

#include "qctfplugin.moc"

QT_END_NAMESPACE
