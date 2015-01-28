/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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
// This file contains benchmarks for QRect/QRectF functions.

#include <QtWidgets/QWidget>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QLineEdit>
#include <qtest.h>

class tst_qstylesheetstyle : public QObject
{
    Q_OBJECT
private slots:
    void empty();
    void empty_events();

    void simple();
    void simple_events();

    void grid_data();
    void grid();

private:
    QWidget *buildSimpleWidgets();

};


QWidget *tst_qstylesheetstyle::buildSimpleWidgets()
{
    QWidget *w = new QWidget();
    QGridLayout *layout = new QGridLayout(w);
    w->setLayout(layout);
    layout->addWidget(new QPushButton("pushButton")     ,0,0);
    layout->addWidget(new QComboBox()                   ,0,1);
    layout->addWidget(new QCheckBox("checkBox")         ,0,2);
    layout->addWidget(new QRadioButton("radioButton")   ,0,3);
    layout->addWidget(new QLineEdit()                   ,1,0);
    layout->addWidget(new QLabel("label")               ,1,1);
    layout->addWidget(new QSpinBox()                    ,1,2);
    layout->addWidget(new QProgressBar()                ,1,3);
    return w;
}

void tst_qstylesheetstyle::empty()
{
    QWidget *w = buildSimpleWidgets();
    w->setStyleSheet("/* */");
    QApplication::processEvents();
    int i = 0;
    QBENCHMARK {
        w->setStyleSheet("/*" + QString::number(i) + "*/");
        i++; // we want a different string in case we have severals iterations
    }
    delete w;
}

void tst_qstylesheetstyle::empty_events()
{
    QWidget *w = buildSimpleWidgets();
    w->setStyleSheet("/* */");
    QApplication::processEvents();
    int i = 0;
    QBENCHMARK {
        w->setStyleSheet("/*" + QString::number(i) + "*/");
        i++; // we want a different string in case we have severals iterations
        qApp->processEvents();
    }
    delete w;
}

static const char *simple_css =
   " QLineEdit { background: red; }   QPushButton { border: 1px solid yellow; color: pink; }  \n"
   " QCheckBox { margin: 3px 5px; background-color:red; } QAbstractButton { background-color: #456; } \n"
   " QFrame { padding: 3px; } QLabel { color: black } QSpinBox:hover { background-color:blue; }  ";

void tst_qstylesheetstyle::simple()
{
    QWidget *w = buildSimpleWidgets();
    w->setStyleSheet("/* */");
    QApplication::processEvents();
    int i = 0;
    QBENCHMARK {
        w->setStyleSheet(QString(simple_css) + "/*" + QString::number(i) + "*/");
        i++; // we want a different string in case we have severals iterations
    }
    delete w;
}

void tst_qstylesheetstyle::simple_events()
{
    QWidget *w = buildSimpleWidgets();
    w->setStyleSheet("/* */");
    QApplication::processEvents();
    int i = 0;
    QBENCHMARK {
        w->setStyleSheet(QString(simple_css) + "/*" + QString::number(i) + "*/");
        i++; // we want a different string in case we have severals iterations
        qApp->processEvents();
    }
    delete w;
}

void tst_qstylesheetstyle::grid_data()
{
        QTest::addColumn<bool>("events");
        QTest::addColumn<bool>("show");
        QTest::addColumn<int>("N");
        for (int n = 5; n <= 25; n += 5) {
           const QByteArray nString = QByteArray::number(n*n);
            QTest::newRow(QByteArray("simple--" + nString).constData()) << false << false << n;
            QTest::newRow(QByteArray("events--" + nString).constData()) << true << false << n;
            QTest::newRow(QByteArray("show--" + nString).constData()) << true << true << n;
        }
}


void tst_qstylesheetstyle::grid()
{
    QFETCH(bool, events);
    QFETCH(bool, show);
    QFETCH(int, N);

    QWidget *w = new QWidget();
    QGridLayout *layout = new QGridLayout(w);
    w->setLayout(layout);
    QString stylesheet;
    for(int x=0; x<N ;x++)
        for(int y=0; y<N ;y++) {
        QLabel *label = new QLabel(QString::number(y * N + x));
        layout->addWidget(label ,x,y);
        label->setObjectName(QString("label%1").arg(y * N + x));
        stylesheet += QString("#label%1 { background-color: rgb(0,%2,%3); color: rgb(%2,%3,255);  } ").arg(y*N+x).arg(y*255/N).arg(x*255/N);
    }

    w->setStyleSheet("/* */");
    if(show) {
        w->show();
        QTest::qWaitForWindowShown(w);
        QApplication::flush();
        QApplication::processEvents();
        QTest::qWait(30);
        QApplication::processEvents();
    }
    QApplication::processEvents();
    int i = 0;
    QBENCHMARK {
        w->setStyleSheet(stylesheet + "/*" + QString::number(i) + "*/");
        i++; // we want a different string in case we have severals iterations
        if(events)
            qApp->processEvents();
    }
    delete w;
}

QTEST_MAIN(tst_qstylesheetstyle)

#include "main.moc"
