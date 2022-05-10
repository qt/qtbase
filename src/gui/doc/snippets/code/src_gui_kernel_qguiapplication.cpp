// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QApplication>
#include <QMessageBox>
#include <QSessionManager>
#include <QWidget>

namespace src_gui_kernel_qguiapplication {
struct MyMainWidget : public QWidget
{
    MyMainWidget(QWidget *parent);
    void commitData(QSessionManager& manager);
    bool saveDocument() { return true; };
    QStringList restartCommand() { return QStringList(); };
    QStringList discardCommand() { return QStringList(); };
};
MyMainWidget *mainWindow = nullptr;
void do_something(QString command) { Q_UNUSED(command); };
MyMainWidget mySession(nullptr);

//! [0]
int main(int argc, char *argv[])
{
    QApplication::setDesktopSettingsAware(false);
    QApplication app(argc, argv);
    // ...
    return app.exec();
}
//! [0]


//! [1]
MyMainWidget::MyMainWidget(QWidget *parent)
    : QWidget(parent)
{
    connect(qApp, &QGuiApplication::commitDataRequest,
            this, &MyMainWidget::commitData);
}

void MyMainWidget::commitData(QSessionManager& manager)
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
//! [1]


/* wrap snippet 2

//! [2]
appname -session id
//! [2]

*/ // wrap snippet 2


void wrapper0() {


//! [3]
const QStringList commands = mySession.restartCommand();
for (const QString &command : commands)
    do_something(command);
//! [3]

} // wrapper0


void wrapper1() {
//! [4]
const QStringList commands = mySession.discardCommand();
for (const QString &command : mySession.discardCommand())
    do_something(command);
//! [4]


} // wrapper1
} // src_gui_kernel_qguiapplication
