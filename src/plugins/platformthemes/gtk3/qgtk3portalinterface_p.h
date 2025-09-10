// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGTK3PORTALINTERFACE_H
#define QGTK3PORTALINTERFACE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/QObject>
#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE

class QDBusVariant;
class QGtk3Storage;

Q_DECLARE_LOGGING_CATEGORY(lcQGtk3PortalInterface);

class QGtk3PortalInterface : public QObject
{
    Q_OBJECT
public:
    QGtk3PortalInterface(QGtk3Storage *s);
    ~QGtk3PortalInterface() = default;

    Qt::ColorScheme colorScheme() const;

private Q_SLOTS:
    void settingChanged(const QString &group, const QString &key,
                        const QDBusVariant &value);
private:
    void queryColorScheme();

    Qt::ColorScheme m_colorScheme = Qt::ColorScheme::Unknown;
    QGtk3Storage *m_storage = nullptr;
};

QT_END_NAMESPACE

#endif // QGTK3PORTALINTERFACE_H
