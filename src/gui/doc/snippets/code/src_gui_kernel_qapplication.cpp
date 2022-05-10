// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QApplication>
#include <QStyleFactory>
#include <QWidget>


namespace src_gui_kernel_qapplication {
struct MyWidget
{
    QSize sizeHint() const;

    int foo = 0;
    MyWidget operator- (MyWidget& other)
    {
        MyWidget tmp = other;
        return tmp;
    };
    int manhattanLength() { return 0; }
};


//! [0]
QCoreApplication *createApplication(int &argc, char *argv[])
{
    for (int i = 1; i < argc; ++i)
        if (!qstrcmp(argv[i], "-no-gui"))
            return new QCoreApplication(argc, argv);
    return new QApplication(argc, argv);
}

int main(int argc, char *argv[])
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


void wrapper0() {

//! [1]
QApplication::setStyle(QStyleFactory::create("fusion"));
//! [1]

} // wrapper0


//! [3]
QSize MyWidget::sizeHint() const
{
    return QSize(80, 25);
}
//! [3]


//! [4]
void showAllHiddenTopLevelWidgets()
{
    const auto topLevelWidgets = QApplication::topLevelWidgets();
    for (QWidget *widget : topLevelWidgets) {
        if (widget->isHidden())
            widget->show();
    }
}
//! [4]


//! [5]
void updateAllWidgets()
{
    const auto topLevelWidgets = QApplication::topLevelWidgets();
    for (QWidget *widget : topLevelWidgets)
        widget->update();
}
//! [5]


void startTheDrag() {};
void wrapper1() {
MyWidget startPos;
MyWidget currentPos;
int x = 0;
int y = 0;

//! [6]
if ((startPos - currentPos).manhattanLength() >=
        QApplication::startDragDistance())
    startTheDrag();
//! [6]


//! [7]
QWidget *widget = qApp->widgetAt(x, y);
if (widget)
    widget = widget->window();
//! [7]

} // wrapper1


void wrapper2() {
QPoint point;

//! [8]
QWidget *widget = qApp->widgetAt(point);
if (widget)
    widget = widget->window();
//! [8]


} // wrapper2
} // src_gui_kernel_qapplication
