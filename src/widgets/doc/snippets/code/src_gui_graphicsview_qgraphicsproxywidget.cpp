// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QTabWidget *tabWidget = new QTabWidget;

    QGraphicsScene scene;
    QGraphicsProxyWidget *proxy = scene.addWidget(tabWidget);

    QGraphicsView view(&scene);
    view.show();

    return app.exec();
}
//! [0]

//! [1]
QGroupBox *groupBox = new QGroupBox("Contact Details");
QLabel *numberLabel = new QLabel("Telephone number");
QLineEdit *numberEdit = new QLineEdit;

QFormLayout *layout = new QFormLayout(groupBox);
layout->addRow(numberLabel, numberEdit);

QGraphicsScene scene;
QGraphicsProxyWidget *proxy = scene.addWidget(groupBox);

QGraphicsView view(&scene);
view.show();
//! [1]

//! [2]
QGraphicsScene scene;

QLineEdit *edit = new QLineEdit;
QGraphicsProxyWidget *proxy = scene.addWidget(edit);

edit->isVisible();  // returns true
proxy->isVisible(); // also returns true

edit->hide();

edit->isVisible();  // returns false
proxy->isVisible(); // also returns false
//! [2]
