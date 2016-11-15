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


#include "qlabel.h"
#include <qapplication.h>
#include <qboxlayout.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qmovie.h>
#include <qpicture.h>
#include <qmessagebox.h>
#include <private/qlabel_p.h>

class Widget : public QWidget
{
    Q_OBJECT
public:
    QList<QEvent::Type> events;

protected:
    bool event(QEvent *ev) Q_DECL_OVERRIDE {
        events.append(ev->type());
        return QWidget::event(ev);
    }

};

class tst_QLabel : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void getSetCheck();
    void setText_data();
    void setText();
    void setTextFormat();
#ifndef Q_OS_MAC
    void setBuddy();
#endif
    void setNum();
    void clear();
    void wordWrap();
    void eventPropagation_data();
    void eventPropagation();
    void focusPolicy();

    void task190318_sizes();

    void sizeHint();

    void task226479_movieResize();
    void emptyPixmap();

    void unicodeText_data();
    void unicodeText();

    void mnemonic_data();
    void mnemonic();
    void selection();

#ifndef QT_NO_CONTEXTMENU
    void taskQTBUG_7902_contextMenuCrash();
#endif

    void taskQTBUG_48157_dprPixmap();
    void taskQTBUG_48157_dprMovie();

private:
    QLabel *testWidget;
    QPointer<Widget> test_box;
    QPointer<QLabel> test_label;
};

// Testing get/set functions
void tst_QLabel::getSetCheck()
{
    QLabel obj1;
    // bool QLabel::wordWrap()
    // void QLabel::setWordWrap(bool)
    obj1.setWordWrap(false);
    QCOMPARE(false, obj1.wordWrap());
    obj1.setWordWrap(true);
    QCOMPARE(true, obj1.wordWrap());

    // QWidget * QLabel::buddy()
    // void QLabel::setBuddy(QWidget *)
    QWidget *var2 = new QWidget();
    obj1.setBuddy(var2);
    QCOMPARE(var2, obj1.buddy());
    obj1.setBuddy((QWidget *)0);
    QCOMPARE((QWidget *)0, obj1.buddy());
    delete var2;

    // QMovie * QLabel::movie()
    // void QLabel::setMovie(QMovie *)
    QMovie *var3 = new QMovie;
    obj1.setMovie(var3);
    QCOMPARE(var3, obj1.movie());
    obj1.setMovie((QMovie *)0);
    QCOMPARE((QMovie *)0, obj1.movie());
    delete var3;
}

void tst_QLabel::initTestCase()
{
    // Create the test class
    testWidget = new QLabel(0);
    testWidget->resize( 200, 200 );
    testWidget->show();
}

void tst_QLabel::cleanupTestCase()
{
    delete testWidget;
    testWidget = 0;
    delete test_box;
}

void tst_QLabel::init()
{
    testWidget->setTextFormat( Qt::AutoText );
    testWidget->setBuddy( 0 );
    testWidget->setIndent( 0 );
    testWidget->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
    testWidget->setScaledContents( false );
}

void tst_QLabel::cleanup()
{
    if (QTest::currentTestFunction() == QLatin1String("setBuddy")) {
        testWidget->show();

        delete test_box; // this should delete tst_labl and test_edit as well.
    }
}

// Set buddy doesn't make much sense on OS X
#ifndef Q_OS_MAC
void tst_QLabel::setBuddy()
{
    testWidget->hide();

    test_box = new Widget;
    test_label= new QLabel( test_box );
    test_label->setText( "&Test with a buddy" );
    QWidget *test_edit = new QLineEdit( test_box );
    QVBoxLayout *layout = new QVBoxLayout(test_box);
    layout->addWidget(test_label);
    layout->addWidget(test_edit);
    test_box->show();
    qApp->setActiveWindow(test_box);
    QVERIFY(test_box->isActiveWindow());

    test_label->setBuddy( test_edit );
    test_label->setFocus();
    QVERIFY( !test_edit->hasFocus() );
    QTest::keyClick( test_box, 't', Qt::AltModifier );
    QVERIFY( test_edit->hasFocus() );
    delete test_box;
}
#endif

void tst_QLabel::setText_data()
{
    QTest::addColumn<QString>("txt");
    QTest::addColumn<QString>("font");

    QString prefix = "";
#ifdef Q_OS_WIN32
    prefix = "win32_";
#endif

    QTest::newRow( QString(prefix + "data0").toLatin1() ) << QString("This is a single line") << QString("Helvetica");
    QTest::newRow( QString(prefix + "data1").toLatin1() ) << QString("This is the first line\nThis is the second line") << QString("Courier");
    QTest::newRow( QString(prefix + "data2").toLatin1() ) << QString("This is the first line\nThis is the second line\nThis is the third line") << QString("Helvetica");
    QTest::newRow( QString(prefix + "data3").toLatin1() ) << QString("This is <b>bold</b> richtext") << QString("Courier");
    QTest::newRow( QString(prefix + "data4").toLatin1() ) << QString("I Have a &shortcut") << QString("Helvetica");
}

void tst_QLabel::setText()
{
    QFETCH( QString, txt );
    QFETCH( QString, font );
    QFont f( font, 8 );
    testWidget->setFont( f );
    testWidget->setText( txt );
    QCOMPARE( testWidget->text(), txt );
}

void tst_QLabel::setTextFormat()
{
    // lets' start with the simple stuff...
    testWidget->setTextFormat( Qt::PlainText );
    QVERIFY( testWidget->textFormat() == Qt::PlainText );

    testWidget->setTextFormat( Qt::RichText );
    QVERIFY( testWidget->textFormat() == Qt::RichText );

    testWidget->setTextFormat( Qt::AutoText );
    QVERIFY( testWidget->textFormat() == Qt::AutoText );
}

void tst_QLabel::setNum()
{
    testWidget->setText( "This is a text" );
    testWidget->setNum( 12 );
    QCOMPARE( testWidget->text(), QString("12") );
    testWidget->setNum( 12.345 );
    QCOMPARE( testWidget->text(), QString("12.345") );
}

void tst_QLabel::clear()
{
    const QString TEXT = "blah blah";
    testWidget->setText(TEXT);
    QCOMPARE(testWidget->text(), TEXT);
    testWidget->clear();
    QVERIFY(testWidget->text().isEmpty());
}

void tst_QLabel::wordWrap()
{
    QLabel label;

    QVERIFY(!label.wordWrap());

    label.setText("Plain Text");
    QVERIFY(!label.wordWrap());

    label.setText("<b>rich text</b>");
    QVERIFY(!label.wordWrap());

    label.setWordWrap(false);
    label.setText("<b>rich text</b>");
    QVERIFY(!label.wordWrap());
}

void tst_QLabel::eventPropagation_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("textInteractionFlags");
    QTest::addColumn<int>("focusPolicy");
    QTest::addColumn<bool>("propagation");

    QTest::newRow("plain text1") << QString("plain text") << int(Qt::LinksAccessibleByMouse) << int(Qt::NoFocus) << true;
    QTest::newRow("plain text2") << QString("plain text") << (int)Qt::TextSelectableByKeyboard << (int)Qt::ClickFocus << true;
    QTest::newRow("plain text3") << QString("plain text") << (int)Qt::TextSelectableByMouse << (int)Qt::ClickFocus << false;
    QTest::newRow("plain text4") << QString("plain text") << (int)Qt::NoTextInteraction << (int)Qt::NoFocus << true;
    QTest::newRow("rich text1") << QString("<b>rich text</b>") << (int)Qt::LinksAccessibleByMouse << (int)Qt::NoFocus << true;
    QTest::newRow("rich text2") << QString("<b>rich text</b>") << (int)Qt::TextSelectableByKeyboard << (int)Qt::ClickFocus << true;
    QTest::newRow("rich text3") << QString("<b>rich text</b>") << (int)Qt::TextSelectableByMouse << (int)Qt::ClickFocus << false;
    QTest::newRow("rich text4") << QString("<b>rich text</b>") << (int)Qt::NoTextInteraction << (int)Qt::NoFocus << true;
    QTest::newRow("rich text5") << QString("<b>rich text</b>") << (int)Qt::LinksAccessibleByKeyboard << (int)Qt::StrongFocus << true;

    if (!test_box)
        test_box = new Widget;
    if (!test_label)
        test_label = new QLabel(test_box);
}

void tst_QLabel::eventPropagation()
{
    QFETCH(QString, text);
    QFETCH(int, textInteractionFlags);
    QFETCH(int, focusPolicy);
    QFETCH(bool, propagation);

    // plain text (accepts mouse event _only_ when label selectable by mouse)
    test_label->setText(text);
    test_box->events.clear();
    test_label->setTextInteractionFlags(Qt::TextInteractionFlags(textInteractionFlags));
    QCOMPARE(int(test_label->focusPolicy()), focusPolicy);
    QTest::mousePress(test_label, Qt::LeftButton);
    QVERIFY(test_box->events.contains(QEvent::MouseButtonPress) == propagation); // should have propagated!
}

void tst_QLabel::focusPolicy()
{
    delete test_label;
    test_label = new QLabel;
    QCOMPARE(test_label->focusPolicy(), Qt::NoFocus); // default
    test_label->setFocusPolicy(Qt::StrongFocus);
    test_label->setText("Whatever"); // setting text should not change the focus policy
    QCOMPARE(test_label->focusPolicy(), Qt::StrongFocus);
    test_label->setTextInteractionFlags(Qt::TextSelectableByKeyboard); // this should
    QCOMPARE(test_label->focusPolicy(), Qt::ClickFocus);
    test_label->setFocusPolicy(Qt::StrongFocus);
    test_label->setText("Whatever"); // setting text should not change the focus policy
    QCOMPARE(test_label->focusPolicy(), Qt::StrongFocus);
    test_label->setTextInteractionFlags(Qt::NoTextInteraction);
    QCOMPARE(test_label->focusPolicy(), Qt::NoFocus);
    test_label->setFocusPolicy(Qt::StrongFocus);
    test_label->setTextInteractionFlags(Qt::NoTextInteraction);
    QCOMPARE(test_label->focusPolicy(), Qt::StrongFocus); // is not touched since value didn't change
    delete test_label;
}

void tst_QLabel::task190318_sizes()
{
    QLabel label(" ");
    QSize ms(500,600);
    label.setMinimumSize(ms);
    QCOMPARE(label.minimumSize(), ms);
    QCOMPARE(label.sizeHint(), ms);
    QCOMPARE(label.minimumSizeHint(), ms);
}

void tst_QLabel::sizeHint()
{
    QLabel label(QLatin1String("Test"));
    label.setIndent(0);
    label.setMargin(0);
    label.setContentsMargins(0, 0, 0, 0);
    label.setAlignment(Qt::AlignVCenter);
    int h = label.sizeHint().height();

    QLabel l1(QLatin1String("Test"));
    l1.setIndent(0);
    l1.setMargin(0);
    l1.setContentsMargins(0, 0, 0, 0);
    l1.setAlignment(Qt::AlignVCenter);
    l1.setTextInteractionFlags(Qt::TextSelectableByMouse);   // will now use qtextcontrol
    int h1 = l1.sizeHint().height();
    QCOMPARE(h1, h);

}

void tst_QLabel::task226479_movieResize()
{
    class Label : public QLabel {
        protected:
            void paintEvent(QPaintEvent *e)
            {
                paintedRegion += e->region();
                QLabel::paintEvent(e);
            }

        public:
            QRegion paintedRegion;
    };

    Label label;
    label.resize(350,350);
    label.show();
    QMovie *movie = new QMovie( &label );
    label.setMovie(movie);
    QVERIFY(QTest::qWaitForWindowExposed(&label));
    movie->setFileName(QFINDTESTDATA("red.png"));
    movie->start();
    QTest::qWait(50);
    movie->stop();
    label.paintedRegion = QRegion();
    movie->setFileName(QFINDTESTDATA("green.png"));
    movie->start();
    QTest::qWait(50);

    QTRY_COMPARE(label.paintedRegion , QRegion(label.rect()) );
}

void tst_QLabel::emptyPixmap()
{
    //task 197919
    QLabel label1, label2, label3, label4;
    label2.setPixmap(QPixmap("/tmp/idonotexist"));
    QMovie movie;
    label3.setMovie(&movie);
    label4.setPicture(QPicture());
    QCOMPARE(label1.sizeHint(), label2.sizeHint());
    QCOMPARE(label1.sizeHint(), label3.sizeHint());
    QCOMPARE(label1.sizeHint(), label4.sizeHint());
}

/**
    Test for QTBUG-4848 - unicode data corrupting QLabel display
*/
void tst_QLabel::unicodeText_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("languageName");

    /*
    The "glass" phrase in Thai was the initial report for bug QTBUG-4848, was
    originally found on http://www.columbia.edu/kermit/utf8.html.

    The phrase is from an internet tradition regarding a striking phrase
    that is translated into many different languages.  The utf8 strings
    below were generated by using http://translate.google.com.

    The glass phrase in Thai contains the ้ว character which manifests bug
    QTBUG-4848

    The last long phrase is an excerpt from Churchills "on the beaches"
    speech, also translated using http://translate.google.com.
    */

    QTest::newRow("english") << QString::fromUtf8("I can eat glass and it doesn't hurt me.") << QString("english");
    QTest::newRow("thai") << QString::fromUtf8("ฉันจะกินแก้วและไม่เจ็บฉัน") << QString("thai");
    QTest::newRow("chinese") << QString::fromUtf8("我可以吃玻璃，并没有伤害我。") << QString("chinese");
    QTest::newRow("arabic") << QString::fromUtf8("أستطيع أكل الزجاج ، وأنه لا يؤذيني.") << QString("arabic");
    QTest::newRow("russian") << QString::fromUtf8("Я могу есть стекло, и не больно.") << QString("russian");
    QTest::newRow("korean") << QString::fromUtf8("유리를 먹을 수있는, 그리고 그게 날 다치게하지 않습니다.") << QString("korean");
    QTest::newRow("greek") << QString::fromUtf8("Μπορώ να φάτε γυαλί και δεν μου κάνει κακό.") << QString("greek");
    QTest::newRow("german") << QString::fromUtf8("Ich kann Glas essen und es macht mich nicht heiß.") << QString("german");

    QTest::newRow("thai_long") << QString::fromUtf8("เราจะต่อสู้ในทะเลและมหาสมุทร. เราจะต่อสู้ด้วยความมั่นใจเติบโตและความเจริญเติบโตในอากาศเราจะปกป้องเกาะของเราค่าใช้จ่ายใดๆอาจ."
                                                    "เราจะต่อสู้บนชายหาดเราจะต่อสู้ในบริเวณเชื่อมโยงไปถึงเราจะต่อสู้ในช่องและในถนนที่เราจะต่อสู้ในภูเขานั้นเราจะไม่ยอม.")
            << QString("thai_long");
}

void tst_QLabel::unicodeText()
{
    const QString testDataPath("testdata/unicodeText");
    QFETCH(QString, text);
    QFETCH(QString, languageName);
    QFrame frame;
    QVBoxLayout *layout = new QVBoxLayout();
    QLabel *label = new QLabel(text, &frame);
    layout->addWidget(label);
    layout->setMargin(8);
    frame.setLayout(layout);
    frame.show();
    QVERIFY(QTest::qWaitForWindowExposed(&frame));
    QVERIFY(frame.isVisible());  // was successfully sized and shown
    testWidget->show();
}

void tst_QLabel::mnemonic_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("expectedDocText");
    QTest::addColumn<QString>("expectedShortcutCursor");

    QTest::newRow("1") << QString("Normal") << QString("Normal") << QString();
    QTest::newRow("2") << QString("&Simple") << QString("Simple") << QString("S");
    QTest::newRow("3") << QString("Also &simple") << QString("Also simple") << QString("s");
    QTest::newRow("4") << QString("&&With &Double &&amp;") << QString("&With Double &amp;") << QString("D");
    QTest::newRow("5") << QString("Hep&&Hop") << QString("Hep&Hop") << QString("");
    QTest::newRow("6") << QString("Hep&&&Hop") << QString("Hep&Hop") << QString("H");
}


void tst_QLabel::mnemonic()
{
    // this test that the mnemonics appears correctly when the label has a text control.

    QFETCH(QString, text);
    QFETCH(QString, expectedDocText);
    QFETCH(QString, expectedShortcutCursor);

    QWidget w;
    QHBoxLayout *hbox = new QHBoxLayout;
    QLabel *lab = new QLabel(text);
    //lab->setText("plop &plop");
    QLineEdit *lineedit = new QLineEdit;
    lab->setBuddy(lineedit);
    lab->setTextInteractionFlags(Qt::TextSelectableByMouse);

    hbox->addWidget(lab);
    hbox->addWidget(lineedit);
    hbox->addWidget(new QLineEdit);
    w.setLayout(hbox);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    QLabelPrivate *d = static_cast<QLabelPrivate *>(QObjectPrivate::get(lab));
    QVERIFY(d->control);
    QCOMPARE(d->control->document()->toPlainText(), expectedDocText);
    QCOMPARE(d->shortcutCursor.selectedText(), expectedShortcutCursor);
}

void tst_QLabel::selection()
{
    QLabel label;
    label.setText("Hello world");

    label.setTextInteractionFlags(Qt::TextSelectableByMouse);

    QVERIFY(!label.hasSelectedText());
    QCOMPARE(label.selectedText(), QString());
    QCOMPARE(label.selectionStart(), -1);

    label.setSelection(0, 4);
    QVERIFY(label.hasSelectedText());
    QCOMPARE(label.selectedText(), QString::fromLatin1("Hell"));
    QCOMPARE(label.selectionStart(), 0);

    label.setSelection(6, 5);
    QVERIFY(label.hasSelectedText());
    QCOMPARE(label.selectedText(), QString::fromLatin1("world"));
    QCOMPARE(label.selectionStart(), 6);
}

#ifndef QT_NO_CONTEXTMENU
void tst_QLabel::taskQTBUG_7902_contextMenuCrash()
{
    QLabel *w = new QLabel("Test or crash?");
    w->setTextInteractionFlags(Qt::TextSelectableByMouse);
    w->show();
    QVERIFY(QTest::qWaitForWindowExposed(w));

    QTimer ti;
    w->connect(&ti, SIGNAL(timeout()), w, SLOT(deleteLater()));
    ti.start(300);

    QContextMenuEvent *cme = new QContextMenuEvent(QContextMenuEvent::Mouse, w->rect().center());
    qApp->postEvent(w, cme);

    QTest::qWait(350);
    // No crash, it's allright.
}
#endif

void tst_QLabel::taskQTBUG_48157_dprPixmap()
{
    QLabel label;
    QPixmap pixmap;
    pixmap.load(QFINDTESTDATA(QStringLiteral("red@2x.png")));
    QCOMPARE(pixmap.devicePixelRatio(), 2.0);
    label.setPixmap(pixmap);
    QCOMPARE(label.sizeHint(), pixmap.rect().size() / pixmap.devicePixelRatio());
}

void tst_QLabel::taskQTBUG_48157_dprMovie()
{
    QLabel label;
    QMovie movie;
    movie.setFileName(QFINDTESTDATA(QStringLiteral("red@2x.png")));
    movie.start();
    QCOMPARE(movie.currentPixmap().devicePixelRatio(), 2.0);
    label.setMovie(&movie);
    QCOMPARE(label.sizeHint(), movie.currentPixmap().size() / movie.currentPixmap().devicePixelRatio());
}

QTEST_MAIN(tst_QLabel)
#include "tst_qlabel.moc"
