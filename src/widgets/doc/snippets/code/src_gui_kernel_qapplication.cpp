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


// ### fixme: Qt 6: Remove [2]
//! [2]
int main(int argc, char *argv[])
{
    QApplication::setColorSpec(QApplication::ManyColor);
    QApplication app(argc, argv);
    ...
    return app.exec();
}
//! [2]


//! [3]
QSize MyWidget::sizeHint() const
{
    return QSize(80, 25);
}
//! [3]


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


//! [6]
int main(int argc, char *argv[])
{
    QApplication::setDesktopSettingsAware(false);
    QApplication app(argc, argv);
    ...
    return app.exec();
}
//! [6]


//! [7]
if ((startPos - currentPos).manhattanLength() >=
        QApplication::startDragDistance())
    startTheDrag();
//! [7]


//! [8]
void MyApplication::commitData(QSessionManager& manager)
{
    if (manager.allowsInteraction()) {
        int ret = QMessageBox::warning(
                    mainWindow,
                    tr("My Application"),
                    tr("Save changes to document?"),
                    QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        switch (ret) {
        case QMessageBox::Save:
            manager.release();
            if (!saveDocument())
                manager.cancel();
            break;
        case QMessageBox::Discard:
            break;
        case QMessageBox::Cancel:
        default:
            manager.cancel();
        }
    } else {
        // we did not get permission to interact, then
        // do something reasonable instead
    }
}
//! [8]


//! [9]
appname -session id
//! [9]


//! [10]
const QStringList commands = mySession.restartCommand();
for (const QString &command : commands)
    do_something(command);
//! [10]


//! [11]
const QStringList commands = mySession.discardCommand();
for (const QString &command : commands)
    do_something(command);
//! [11]


//! [12]
QWidget *widget = QApplication::widgetAt(x, y);
if (widget)
    widget = widget->window();
//! [12]


//! [13]
QWidget *widget = QApplication::widgetAt(point);
if (widget)
    widget = widget->window();
//! [13]
