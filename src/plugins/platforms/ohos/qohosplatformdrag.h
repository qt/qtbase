// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSPLATFORMDRAG_H
#define QOHOSPLATFORMDRAG_H

#include <memory>
#include <qpa/qplatformdrag.h>

QT_BEGIN_NAMESPACE

class QOhosPlatformDrag : public QPlatformDrag
{
public:
    QOhosPlatformDrag();
    ~QOhosPlatformDrag() override;

    virtual void handlePreDrop() = 0;

    virtual void updateDropAction(Qt::DropAction dropAction) = 0;
};

std::unique_ptr<QOhosPlatformDrag> makeQOhosPlatformDrag();

QT_END_NAMESPACE

#endif
