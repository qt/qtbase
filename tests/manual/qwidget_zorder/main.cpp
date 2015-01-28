/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QApplication>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QCalendarWidget>
#include <QVBoxLayout>

class Widget : public QWidget
{
    Q_OBJECT
public:
    Widget()
    {
        QWidget *stackWidget = new QWidget;
        stackWidget->setFixedSize(400, 300);
        button = new QPushButton("pushbutton", stackWidget);
        plainTextEdit = new QPlainTextEdit(stackWidget);
        plainTextEdit->setWordWrapMode(QTextOption::NoWrap);
        QString s = "foo bar bar foo foo bar bar foo";
        for (int i = 0; i < 10; ++i) {
            plainTextEdit->appendPlainText(s);
            s.remove(1, 2);
        }
        calendar = new QCalendarWidget(stackWidget);
        button->move(10, 10);
        button->resize(100, 100);
        plainTextEdit->move(30, 70);
        plainTextEdit->resize(100, 100);
        calendar->move(80, 40);

        QWidget *buttonOps = new QWidget;
        QVBoxLayout *l = new QVBoxLayout(buttonOps);
        QPushButton *lower = new QPushButton("button: lower");
        connect(lower, SIGNAL(clicked()), button, SLOT(lower()));
        l->addWidget(lower);
        QPushButton *raise = new QPushButton("button: raise");
        connect(raise, SIGNAL(clicked()), button, SLOT(raise()));
        l->addWidget(raise);

        lower = new QPushButton("calendar: lower");
        connect(lower, SIGNAL(clicked()), calendar, SLOT(lower()));
        l->addWidget(lower);
        raise = new QPushButton("calendar: raise");
        connect(raise, SIGNAL(clicked()), calendar, SLOT(raise()));
        l->addWidget(raise);
        QPushButton *stackUnder = new QPushButton("calendar: stack under textedit");
        connect(stackUnder, SIGNAL(clicked()), this, SLOT(stackCalendarUnderTextEdit()));
        l->addWidget(stackUnder);

        lower = new QPushButton("lower textedit");
        connect(lower, SIGNAL(clicked()), plainTextEdit, SLOT(lower()));
        l->addWidget(lower);
        raise = new QPushButton("raise textedit");
        connect(raise, SIGNAL(clicked()), plainTextEdit, SLOT(raise()));
        l->addWidget(raise);

        QHBoxLayout *mainLayout = new QHBoxLayout(this);
        mainLayout->addWidget(buttonOps);
        mainLayout->addWidget(stackWidget);
    }

private Q_SLOTS:
    void stackCalendarUnderTextEdit()
    {
        calendar->stackUnder(plainTextEdit);
    }

private:
    QPushButton *button;
    QPlainTextEdit *plainTextEdit;
    QCalendarWidget *calendar;
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    Widget w;
    w.show();
    return app.exec();
}

#include "main.moc"
