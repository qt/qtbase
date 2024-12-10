// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSICONENGINE_H
#define QWINDOWSICONENGINE_H

#include <QtGui/private/qfonticonengine_p.h>

#ifndef QT_NO_ICON

QT_BEGIN_NAMESPACE

class QWindowsIconEngine : public QFontIconEngine
{
public:
    QWindowsIconEngine(const QString &iconName);
    ~QWindowsIconEngine();

    QString key() const override;
    QIconEngine *clone() const override;

protected:
    QString string() const override;

private:
    const QString m_glyphs;
};

QT_END_NAMESPACE

#endif // QT_NO_ICON

#endif // QWINDOWSICONENGINE_H
