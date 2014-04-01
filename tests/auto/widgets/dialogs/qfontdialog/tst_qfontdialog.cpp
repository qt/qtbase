/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>


#include <qapplication.h>
#include <qfontinfo.h>
#include <qtimer.h>
#include <qmainwindow.h>
#include <qlistview.h>
#include "qfontdialog.h"
#include <private/qfontdialog_p.h>

QT_FORWARD_DECLARE_CLASS(QtTestEventThread)

class tst_QFontDialog : public QObject
{
    Q_OBJECT

public:
    tst_QFontDialog();
    virtual ~tst_QFontDialog();


public slots:
    void postKeyReturn();
    void testGetFont();
    void testSetFont();
    void testNonStandardFontSize();

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
private slots:
    void defaultOkButton();
    void setFont();
    void task256466_wrongStyle();
    void setNonStandardFontSize();

private:
    void runSlotWithFailsafeTimer(const char *member);
};

tst_QFontDialog::tst_QFontDialog()
{
}

tst_QFontDialog::~tst_QFontDialog()
{
}

void tst_QFontDialog::initTestCase()
{
}

void tst_QFontDialog::cleanupTestCase()
{
}

void tst_QFontDialog::init()
{
}

void tst_QFontDialog::cleanup()
{
}

void tst_QFontDialog::postKeyReturn() {
    QWidgetList list = QApplication::topLevelWidgets();
    for (int i=0; i<list.count(); ++i) {
        QFontDialog *dialog = qobject_cast<QFontDialog*>(list[i]);
        if (dialog) {
            QTest::keyClick( list[i], Qt::Key_Return, Qt::NoModifier );
            return;
        }
    }
}

void tst_QFontDialog::testGetFont()
{
#ifdef Q_OS_MAC
    QEXPECT_FAIL("", "Sending QTest::keyClick to OSX font dialog helper fails, see QTBUG-24321", Continue);
#endif
    bool ok = false;
    QTimer::singleShot(2000, this, SLOT(postKeyReturn()));
    QFontDialog::getFont(&ok);
    QVERIFY(ok);
}

void tst_QFontDialog::runSlotWithFailsafeTimer(const char *member)
{
    // FailSafeTimer quits the nested event loop if the dialog closing doesn't do it.
    QTimer failSafeTimer;
    failSafeTimer.setInterval(4000);
    failSafeTimer.setSingleShot(true);
    connect(&failSafeTimer, SIGNAL(timeout()), qApp, SLOT(quit()));
    failSafeTimer.start();

    QTimer::singleShot(0, this, member);
    qApp->exec();

    // FailSafeTimer stops once it goes out of scope.
}

void tst_QFontDialog::defaultOkButton()
{
    runSlotWithFailsafeTimer(SLOT(testGetFont()));
}

void tst_QFontDialog::testSetFont()
{
    bool ok = false;
#if defined Q_OS_HPUX
    QString fontName = "Courier";
    int fontSize = 25;
#elif defined Q_OS_AIX
    QString fontName = "Charter";
    int fontSize = 13;
#else
    QString fontName = "Arial";
    int fontSize = 24;
#endif
    QFont f1(fontName, fontSize);
    f1.setPixelSize(QFontInfo(f1).pixelSize());
    QTimer::singleShot(2000, this, SLOT(postKeyReturn()));
    QFont f2 = QFontDialog::getFont(&ok, f1);
    QCOMPARE(QFontInfo(f2).pointSize(), QFontInfo(f1).pointSize());
}

void tst_QFontDialog::setFont()
{
    /* The font should be the same before as it is after if nothing changed
              while the font dialog was open.
              Task #27662
    */
    runSlotWithFailsafeTimer(SLOT(testSetFont()));
}


class FriendlyFontDialog : public QFontDialog
{
    friend class tst_QFontDialog;
    Q_DECLARE_PRIVATE(QFontDialog)
};

void tst_QFontDialog::task256466_wrongStyle()
{
    QFontDatabase fdb;
    FriendlyFontDialog dialog;
    dialog.setOption(QFontDialog::DontUseNativeDialog);
    QListView *familyList = reinterpret_cast<QListView*>(dialog.d_func()->familyList);
    QListView *styleList = reinterpret_cast<QListView*>(dialog.d_func()->styleList);
    QListView *sizeList = reinterpret_cast<QListView*>(dialog.d_func()->sizeList);
    for (int i = 0; i < familyList->model()->rowCount(); ++i) {
        QModelIndex currentFamily = familyList->model()->index(i, 0);
        familyList->setCurrentIndex(currentFamily);
        const QFont current = dialog.currentFont(),
                    expected = fdb.font(currentFamily.data().toString(),
            styleList->currentIndex().data().toString(), sizeList->currentIndex().data().toInt());
        QCOMPARE(current.family(), expected.family());
        QCOMPARE(current.style(), expected.style());
        QCOMPARE(current.pointSizeF(), expected.pointSizeF());
    }
}

void tst_QFontDialog::setNonStandardFontSize()
{
    runSlotWithFailsafeTimer(SLOT(testNonStandardFontSize()));
}

void tst_QFontDialog::testNonStandardFontSize()
{
    QList<int> standardSizesList = QFontDatabase::standardSizes();
    int nonStandardFontSize;
    if (!standardSizesList.isEmpty()) {
        nonStandardFontSize = standardSizesList.at(standardSizesList.count()-1); // get the maximum standard size.
        nonStandardFontSize += 1; // the increment of 1 to mock a non-standard font size.
    } else {
        QSKIP("QFontDatabase::standardSizes() is empty.");
    }

    QFont testFont;
    testFont.setPointSize(nonStandardFontSize);

    bool accepted = false;
    QTimer::singleShot(2000, this, SLOT(postKeyReturn()));
    QFont resultFont = QFontDialog::getFont(&accepted, testFont,
        QApplication::activeWindow(),
        QLatin1String("QFontDialog - NonStandardFontSize Test"),
        QFontDialog::DontUseNativeDialog);
    QVERIFY(accepted);

    if (accepted)
        QCOMPARE(testFont.pointSize(), resultFont.pointSize());
    else
        QWARN("Fail using a non-standard font size.");
}

QTEST_MAIN(tst_QFontDialog)
#include "tst_qfontdialog.moc"
