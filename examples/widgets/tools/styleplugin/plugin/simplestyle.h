// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SIMPLESTYLE_H
#define SIMPLESTYLE_H

#include <QProxyStyle>

class SimpleStyle : public QProxyStyle
{
    Q_OBJECT

public:
    SimpleStyle() = default;

    void polish(QPalette &palette) override;
};

#endif
