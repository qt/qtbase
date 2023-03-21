// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QLoggingCategory>
#include <QTreeView>

#include "inputdevicemodel.h"

int main(int argc, char **argv)
{
    QLoggingCategory::setFilterRules(QStringLiteral("qt.qpa.input.devices=true"));

    QApplication app(argc, argv);

    QTreeView view;
    view.setModel(new InputDeviceModel(&view));
    view.resize(1280, 600);
    view.show();
    view.resizeColumnToContents(0);

    app.exec();
}

