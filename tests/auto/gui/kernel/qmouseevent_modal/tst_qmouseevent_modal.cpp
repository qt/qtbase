/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <qapplication.h>
#include <qfontinfo.h>


#include <qpushbutton.h>
#include <qscrollbar.h>
#include <qtimer.h>

#include <qdialog.h>

class TstWidget;
class TstDialog;
QT_FORWARD_DECLARE_CLASS(QPushButton)

class tst_qmouseevent_modal : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void mousePressRelease();

private:
    TstWidget *w;
};

class TstWidget : public QWidget
{
    Q_OBJECT
public:
    TstWidget();
public slots:
    void buttonPressed();
public:
    QPushButton *pb;
    TstDialog *d;
};


class TstDialog : public QDialog
{
    Q_OBJECT
public:
    TstDialog( QWidget *mouseWidget, QWidget *parent, const char *name );
    int count() { return c; }
protected:
    void showEvent ( QShowEvent * );
public slots:
    void releaseMouse();
    void closeDialog();
private:
    QWidget *m;
    int c;
};

void tst_qmouseevent_modal::initTestCase()
{
    w = new TstWidget;
    w->show();
}

void tst_qmouseevent_modal::cleanupTestCase()
{
    delete w;
    w = 0;
}

/*
  Test for task 22500
*/
void tst_qmouseevent_modal::mousePressRelease()
{

    QVERIFY( !w->d->isVisible() );
    QVERIFY( w->d->count() == 0 );

    QTest::mousePress( w->pb, Qt::LeftButton );

    QTRY_VERIFY( !w->d->isVisible() );
    QVERIFY( w->d->count() == 1 );
    QVERIFY( !w->pb->isDown() );

    QTest::mousePress( w->pb, Qt::LeftButton );

    QTRY_VERIFY( !w->d->isVisible() );
    QVERIFY( w->d->count() == 2 );
    QVERIFY( !w->pb->isDown() );

    // With the current QWS mouse handling, the 3rd press would fail...

    QTest::mousePress( w->pb, Qt::LeftButton );

    QTRY_VERIFY( !w->d->isVisible() );
    QVERIFY( w->d->count() == 3 );
    QVERIFY( !w->pb->isDown() );

    QTest::mousePress( w->pb, Qt::LeftButton );

    QTRY_VERIFY( !w->d->isVisible() );
    QVERIFY( w->d->count() == 4 );
    QVERIFY( !w->pb->isDown() );
}


TstWidget::TstWidget()
{
    pb = new QPushButton( "Press me", this );
    pb->setObjectName("testbutton");
    QSize s = pb->sizeHint();
    pb->setGeometry( 5, 5, s.width(), s.height() );

    connect( pb, SIGNAL(pressed()), this, SLOT(buttonPressed()) );

//    QScrollBar *sb = new QScrollBar( Qt::Horizontal,  this );

//    sb->setGeometry( 5, pb->geometry().bottom() + 5, 100, sb->sizeHint().height() );

    d = new TstDialog( pb, this , 0 );
}

void TstWidget::buttonPressed()
{
    d->exec();
}

TstDialog::TstDialog( QWidget *mouseWidget, QWidget *parent, const char *name )
    :QDialog( parent )
{
    setObjectName(name);
    setModal(true);
    m = mouseWidget;
    c = 0;
}

void TstDialog::showEvent ( QShowEvent * )
{
    QTimer::singleShot(1, this, SLOT(releaseMouse()));
    QTimer::singleShot(100, this, SLOT(closeDialog()));
}

void TstDialog::releaseMouse()
{
    QTest::mouseRelease(m, Qt::LeftButton);
}

void TstDialog::closeDialog()
{
    if ( isVisible() ) {
        c++;
        accept();
    }
}

QTEST_MAIN(tst_qmouseevent_modal)
#include "tst_qmouseevent_modal.moc"
