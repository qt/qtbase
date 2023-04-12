// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QCoreApplication* createApplication(int &argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (!qstrcmp(argv[i], "-no-gui"))
            return new QCoreApplication(argc, argv);
    }
    return new QApplication(argc, argv);
}

int main(int argc, char* argv[])
{
    QScopedPointer<QCoreApplication> app(createApplication(argc, argv));

    if (qobject_cast<QApplication *>(app.data())) {
       // start GUI version...
    } else {
       // start non-GUI version...
    }

    return app->exec();
}
//! [0]


//! [1]
QApplication::setStyle(QStyleFactory::create("Fusion"));
//! [1]


//! [4]
void showAllHiddenTopLevelWidgets()
{
    const QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
    for (QWidget *widget : topLevelWidgets) {
        if (widget->isHidden())
            widget->show();
    }
}
//! [4]


//! [5]
void updateAllWidgets()
{
    const QWidgetList allWidgets = QApplication::allWidgets();
    for (QWidget *widget : allWidgets)
        widget->update();
}
//! [5]


//! [7]
if ((startPos - currentPos).manhattanLength() >=
        QApplication::startDragDistance())
    startTheDrag();
//! [7]
