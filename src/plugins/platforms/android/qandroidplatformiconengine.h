// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDPLATFORMICONENGINE_H
#define QANDROIDPLATFORMICONENGINE_H

#include <QtGui/private/qfonticonengine_p.h>

#ifndef QT_NO_ICON

QT_BEGIN_NAMESPACE

class QAndroidPlatformIconEngine : public QFontIconEngine
{
public:
    QAndroidPlatformIconEngine(const QString &iconName);
    ~QAndroidPlatformIconEngine();

    QString key() const override;
    QIconEngine *clone() const override;

protected:
    QString string() const override;

private:
    const QString m_glyphs;
};

QT_END_NAMESPACE

#endif // QT_NO_ICON

#endif // QANDROIDPLATFORMICONENGINE_H
