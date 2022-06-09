// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>

class BasicA11yWidget: public QWidget
{
public:
    BasicA11yWidget() {

        QVBoxLayout *layout = new QVBoxLayout();

        layout->addWidget(new QLabel("This is a text label"));
        layout->addWidget(new QPushButton("This is a push button"));
        layout->addWidget(new QCheckBox("This is a check box"));

        // TODO: Add more widgets

        layout->addStretch();

        setLayout(layout);
    }
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    BasicA11yWidget a11yWidget;
    a11yWidget.show();

    return app.exec();
}
