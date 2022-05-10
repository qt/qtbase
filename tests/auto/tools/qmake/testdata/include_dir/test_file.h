// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <qwidget.h>

#include "ui_untitled.h"

class SomeObject : public QWidget, public Ui_Form
{
    Q_OBJECT
public:
    SomeObject();
signals:
    void someSignal();
};
