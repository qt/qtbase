// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets/qapplication.h>
#include "primecounter.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setOrganizationName("QtProject");
    app.setApplicationName(QApplication::translate("main", "Prime Counter"));

    PrimeCounter dialog;
    dialog.setWindowTitle(QApplication::translate("main", "Prime Counter"));
    dialog.show();

    return app.exec();
}
