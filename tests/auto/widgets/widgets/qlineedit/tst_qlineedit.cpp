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
#include "qlineedit.h"
#include "qapplication.h"
#include "qstringlist.h"
#include "qstyle.h"
#include "qvalidator.h"
#include "qcompleter.h"
#include "qstandarditemmodel.h"
#include <qpa/qplatformtheme.h>
#include "qstylehints.h"
#include <private/qguiapplication_p.h>
#include "qclipboard.h"

#ifdef Q_OS_MAC
#include <cstdlib> // For the random function.
#endif

#include <qlineedit.h>
#include <private/qlineedit_p.h>
#include <private/qwidgetlinecontrol_p.h>
#include <qmenu.h>
#include <qlayout.h>
#include <qspinbox.h>
#include <qdebug.h>

#include "qcommonstyle.h"
#include "qstyleoption.h"

#include "qplatformdefs.h"

#include "../../../shared/platformclipboard.h"
#include "../../../shared/platforminputcontext.h"
#include <private/qinputmethod_p.h>

#include "../../../qtest-config.h"

QT_BEGIN_NAMESPACE
class QPainter;
QT_END_NAMESPACE

class StyleOptionTestStyle : public QCommonStyle
{
private:
    bool readOnly;

public:
    inline StyleOptionTestStyle() : QCommonStyle(), readOnly(false)
    {
    }

    inline void setReadOnly(bool readOnly)
    {
        this->readOnly = readOnly;
    }

    inline void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *,
                                 const QWidget *) const
    {
        switch (pe) {
            case PE_PanelLineEdit:
            if (readOnly)
                QVERIFY(opt->state & QStyle::State_ReadOnly);
            else
                QVERIFY(!(opt->state & QStyle::State_ReadOnly));
            break;

            default:
            break;
        }
    }
};

class tst_QLineEdit : public QObject
{
Q_OBJECT

public:
    enum EventStates { Press, Release, Click };

    tst_QLineEdit();
    virtual ~tst_QLineEdit();

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
private slots:
    void getSetCheck();
    void experimental();

    void upperAndLowercase();

    void setInputMask_data();
    void setInputMask();

    void inputMask_data();
    void inputMask();

    void clearInputMask();

    void keypress_inputMask_data();
    void keypress_inputMask();

    void inputMaskAndValidator_data();
    void inputMaskAndValidator();

    void hasAcceptableInputMask_data();
    void hasAcceptableInputMask();

    void hasAcceptableInputValidator();


    void redo_data();
    void redo();

    void undo_data();
    void undo();

    void undo_keypressevents_data();
    void undo_keypressevents();

#ifndef QT_NO_CLIPBOARD
    void QTBUG5786_undoPaste();
#endif

    void clear();

    void text_data();
    void text();
    void textMask_data();
    void textMask();
    void maskCharacter();
    void maskCharacter_data();
    void setText();

    void displayText_data();
    void displayText();
    void passwordEchoOnEdit();
    void passwordEchoDelay();

    void maxLength_mask_data();
    void maxLength_mask();

    void maxLength_data();
    void maxLength();

    void isReadOnly();

    void noCursorBlinkWhenReadOnly();

    void cursorPosition();

    void cursorPositionChanged_data();
    void cursorPositionChanged();

    void selectedText();
    void deleteSelectedText();

    void textChangedAndTextEdited();
    void returnPressed();
    void returnPressed_maskvalidator_data();
    void returnPressed_maskvalidator();

    void setValidator();
    void setValidator_QIntValidator_data();
    void setValidator_QIntValidator();

    void frame_data();
    void frame();

    void leftKeyOnSelectedText();

    void setAlignment_data();
    void setAlignment();

    void isModified();
    void edited();

    void insert();
    void setSelection_data();
    void setSelection();

#ifndef QT_NO_CLIPBOARD
    void cut();
#endif
    void maxLengthAndInputMask();
    void returnPressedKeyEvent();

    void keepSelectionOnTabFocusIn();

    void readOnlyStyleOption();

    void validateOnFocusOut();

    void editInvalidText();

    void charWithAltOrCtrlModifier();

    void inlineCompletion();

    void noTextEditedOnClear();

#ifndef QTEST_NO_CURSOR
    void cursor();
#endif

    void textMargin_data();
    void textMargin();

    // task-specific tests:
    void task180999_focus();
    void task174640_editingFinished();
#ifndef QT_NO_COMPLETER
    void task198789_currentCompletion();
    void task210502_caseInsensitiveInlineCompletion();
#endif
    void task229938_dontEmitChangedWhenTextIsNotChanged();
    void task233101_cursorPosAfterInputMethod_data();
    void task233101_cursorPosAfterInputMethod();
    void task241436_passwordEchoOnEditRestoreEchoMode();
    void task248948_redoRemovedSelection();
    void taskQTBUG_4401_enterKeyClearsPassword();
    void taskQTBUG_4679_moveToStartEndOfBlock();
    void taskQTBUG_4679_selectToStartEndOfBlock();
#ifndef QT_NO_CONTEXTMENU
    void taskQTBUG_7902_contextMenuCrash();
#endif
    void taskQTBUG_7395_readOnlyShortcut();
    void QTBUG697_paletteCurrentColorGroup();
    void QTBUG13520_textNotVisible();
    void QTBUG7174_inputMaskCursorBlink();
    void QTBUG16850_setSelection();

    void bidiVisualMovement_data();
    void bidiVisualMovement();

    void bidiLogicalMovement_data();
    void bidiLogicalMovement();

    void selectAndCursorPosition();
    void inputMethod();
    void inputMethodSelection();

    void inputMethodQueryImHints_data();
    void inputMethodQueryImHints();

    void undoRedoAndEchoModes_data();
    void undoRedoAndEchoModes();

protected slots:
    void editingFinished();

    void onTextChanged( const QString &newString );
    void onTextEdited( const QString &newString );
    void onReturnPressed();
    void onSelectionChanged();
    void onCursorPositionChanged(int oldpos, int newpos);

private:
    // keyClicks(..) is moved to QtTestCase
    void psKeyClick(QWidget *target, Qt::Key key, Qt::KeyboardModifiers pressState = 0);
    void psKeyClick(QTestEventList &keys, Qt::Key key, Qt::KeyboardModifiers pressState = 0);

    bool validInput;
    QString changed_string;
    int changed_count;
    int edited_count;
    int return_count;
    int selection_count;
    int lastCursorPos;
    int newCursorPos;
    QLineEdit *testWidget;
    int m_keyboardScheme;
    PlatformInputContext m_platformInputContext;
};

typedef QList<int> IntList;
Q_DECLARE_METATYPE(QLineEdit::EchoMode)

// Testing get/set functions
void tst_QLineEdit::getSetCheck()
{
    QLineEdit obj1;
    // const QValidator * QLineEdit::validator()
    // void QLineEdit::setValidator(const QValidator *)
    QIntValidator *var1 = new QIntValidator(0);
    obj1.setValidator(var1);
    QCOMPARE((const QValidator *)var1, obj1.validator());
    obj1.setValidator((QValidator *)0);
    QCOMPARE((const QValidator *)0, obj1.validator());
    delete var1;

    // bool QLineEdit::dragEnabled()
    // void QLineEdit::setDragEnabled(bool)
    obj1.setDragEnabled(false);
    QCOMPARE(false, obj1.dragEnabled());
    obj1.setDragEnabled(true);
    QCOMPARE(true, obj1.dragEnabled());
}

tst_QLineEdit::tst_QLineEdit() : validInput(false), m_keyboardScheme(0)
{
    if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme())
        m_keyboardScheme = theme->themeHint(QPlatformTheme::KeyboardScheme).toInt();
    // Generalize for X11
    if (m_keyboardScheme == QPlatformTheme::KdeKeyboardScheme
            || m_keyboardScheme == QPlatformTheme::GnomeKeyboardScheme
            || m_keyboardScheme == QPlatformTheme::CdeKeyboardScheme) {
        m_keyboardScheme = QPlatformTheme::X11KeyboardScheme;
    }
}

tst_QLineEdit::~tst_QLineEdit()
{
}

void tst_QLineEdit::initTestCase()
{
    testWidget = new QLineEdit(0);
    testWidget->setObjectName("testWidget");
    connect(testWidget, SIGNAL(cursorPositionChanged(int,int)), this, SLOT(onCursorPositionChanged(int,int)));
    connect(testWidget, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged(QString)));
    connect(testWidget, SIGNAL(textEdited(QString)), this, SLOT(onTextEdited(QString)));
    connect(testWidget, SIGNAL(returnPressed()), this, SLOT(onReturnPressed()));
    connect(testWidget, SIGNAL(selectionChanged()), this, SLOT(onSelectionChanged()));
    connect(testWidget, SIGNAL(editingFinished()), this, SLOT(editingFinished()));

    testWidget->resize(200,50);
    testWidget->show();
    QVERIFY(QTest::qWaitForWindowExposed(testWidget));
    QApplication::setActiveWindow(testWidget);
    QTRY_VERIFY(testWidget->hasFocus());

    changed_count = 0;
    edited_count = 0;
    selection_count = 0;

    QInputMethodPrivate *inputMethodPrivate = QInputMethodPrivate::get(qApp->inputMethod());
    inputMethodPrivate->testContext = &m_platformInputContext;
}

void tst_QLineEdit::cleanupTestCase()
{
    delete testWidget;
    QInputMethodPrivate *inputMethodPrivate = QInputMethodPrivate::get(qApp->inputMethod());
    inputMethodPrivate->testContext = 0;
}

void tst_QLineEdit::init()
{
    return_count = 0;
    testWidget->clear();
    testWidget->setEchoMode(QLineEdit::Normal);
    testWidget->setMaxLength(32767);
    testWidget->setReadOnly(false);
    testWidget->setText("");
    testWidget->setInputMask("");
    testWidget->setFrame(true);
    testWidget->setValidator(0);
    testWidget->setDragEnabled(true);
}

void tst_QLineEdit::cleanup()
{
}

void tst_QLineEdit::experimental()
{
    QIntValidator intValidator(3, 7, 0);
    testWidget->setValidator(&intValidator);
    testWidget->setText("");


    // test the order of setting these
    testWidget->setInputMask("");
    testWidget->setText("abc123");
    testWidget->setInputMask("000.000.000.000");
    QCOMPARE(testWidget->text(), QString("123..."));
    testWidget->setText("");


}

void tst_QLineEdit::upperAndLowercase()
{
    testWidget->setInputMask("");
    testWidget->setText("");

    QTest::keyClicks(testWidget, "aAzZ`1234567890-=~!@#$%^&*()_+[]{}\\|;:'\",.<>/?");
    qApp->processEvents();
    QCOMPARE(testWidget->text(), QString("aAzZ`1234567890-=~!@#$%^&*()_+[]{}\\|;:'\",.<>/?"));
}

void tst_QLineEdit::setInputMask_data()
{
    QTest::addColumn<QString>("mask");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expectedText");
    QTest::addColumn<QString>("expectedDisplay");
    QTest::addColumn<bool>("insert_text");

    // both keyboard and insert()
    for (int i=0; i<2; i++) {
        bool insert_text = i==0 ? false : true;
        QString insert_mode = "keys ";
        if (insert_text)
            insert_mode = "insert ";

        QTest::newRow(QString(insert_mode + "ip_localhost").toLatin1())
            << QString("000.000.000.000")
            << QString("127.0.0.1")
            << QString("127.0.0.1")
            << QString("127.0  .0  .1  ")
            << bool(insert_text);
        QTest::newRow(QString(insert_mode + "mac").toLatin1())
            << QString("HH:HH:HH:HH:HH:HH;#")
            << QString("00:E0:81:21:9E:8E")
            << QString("00:E0:81:21:9E:8E")
            << QString("00:E0:81:21:9E:8E")
            << bool(insert_text);
        QTest::newRow(QString(insert_mode + "mac2").toLatin1())
            << QString("<HH:>HH:!HH:HH:HH:HH;#")
            << QString("AAe081219E8E")
            << QString("aa:E0:81:21:9E:8E")
            << QString("aa:E0:81:21:9E:8E")
            << bool(insert_text);
        QTest::newRow(QString(insert_mode + "byte").toLatin1())
            << QString("BBBBBBBB;0")
            << QString("11011001")
            << QString("11111")
            << QString("11011001")
            << bool(insert_text);
        QTest::newRow(QString(insert_mode + "halfbytes").toLatin1())
            << QString("bbbb.bbbb;-")
            << QString("110. 0001")
            << QString("110.0001")
            << QString("110-.0001")
            << bool(insert_text);
        QTest::newRow(QString(insert_mode + "blank char same type as content").toLatin1())
            << QString("000.000.000.000;0")
            << QString("127.0.0.1")
            << QString("127...1")
            << QString("127.000.000.100")
            << bool(insert_text);
        QTest::newRow(QString(insert_mode + "parts of ip_localhost").toLatin1())
            << QString("000.000.000.000")
            << QString(".0.0.1")
            << QString(".0.0.1")
            << QString("   .0  .0  .1  ")
            << bool(insert_text);
        QTest::newRow(QString(insert_mode + "ip_null").toLatin1())
            << QString("000.000.000.000")
            << QString()
            << QString("...")
            << QString("   .   .   .   ")
            << bool(insert_text);
        QTest::newRow(QString(insert_mode + "ip_null_hash").toLatin1())
            << QString("000.000.000.000;#")
            << QString()
            << QString("...")
            << QString("###.###.###.###")
            << bool(insert_text);
        QTest::newRow(QString(insert_mode + "ip_overflow").toLatin1())
            << QString("000.000.000.000")
            << QString("1234123412341234")
            << QString("123.412.341.234")
            << QString("123.412.341.234")
            << bool(insert_text);
        QTest::newRow(QString(insert_mode + "uppercase").toLatin1())
            << QString(">AAAA")
            << QString("AbCd")
            << QString("ABCD")
            << QString("ABCD")
            << bool(insert_text);
        QTest::newRow(QString(insert_mode + "lowercase").toLatin1())
            << QString("<AAAA")
            << QString("AbCd")
            << QString("abcd")
            << QString("abcd")
            << bool(insert_text);

        QTest::newRow(QString(insert_mode + "nocase").toLatin1())
            << QString("!AAAA")
            << QString("AbCd")
            << QString("AbCd")
            << QString("AbCd")
            << bool(insert_text);
        QTest::newRow(QString(insert_mode + "nocase1").toLatin1())
            << QString("!A!A!A!A")
            << QString("AbCd")
            << QString("AbCd")
            << QString("AbCd")
            << bool(insert_text);
        QTest::newRow(QString(insert_mode + "nocase2").toLatin1())
            << QString("AAAA")
            << QString("AbCd")
            << QString("AbCd")
            << QString("AbCd")
            << bool(insert_text);

        QTest::newRow(QString(insert_mode + "reserved").toLatin1())
            << QString("{n}[0]")
            << QString("A9")
            << QString("A9")
            << QString("A9")
            << bool(insert_text);
        QTest::newRow(QString(insert_mode + "escape01").toLatin1())
            << QString("\\N\\n00")
            << QString("9")
            << QString("Nn9")
            << QString("Nn9 ")
            << bool(insert_text);
        QTest::newRow(QString(insert_mode + "escape02").toLatin1())
            << QString("\\\\00")
            << QString("0")
            << QString("\\0")
            << QString("\\0 ")
            << bool(insert_text);
        QTest::newRow(QString(insert_mode + "escape03").toLatin1())
            << QString("\\(00\\)")
            << QString("0")
            << QString("(0)")
            << QString("(0 )")
            << bool(insert_text);

        QTest::newRow(QString(insert_mode + "upper_lower_nocase1").toLatin1())
            << QString(">AAAA<AAAA!AAAA")
            << QString("AbCdEfGhIjKl")
            << QString("ABCDefghIjKl")
            << QString("ABCDefghIjKl")
            << bool(insert_text);
        QTest::newRow(QString(insert_mode + "upper_lower_nocase2").toLatin1())
            << QString(">aaaa<aaaa!aaaa")
            << QString("AbCdEfGhIjKl")
            << QString("ABCDefghIjKl")
            << QString("ABCDefghIjKl")
            << bool(insert_text);

        QTest::newRow(QString(insert_mode + "exact_case1").toLatin1())
            << QString(">A<A<A>A>A<A!A!A")
            << QString("AbCdEFGH")
            << QString("AbcDEfGH")
            << QString("AbcDEfGH")
            << bool(insert_text);
        QTest::newRow(QString(insert_mode + "exact_case2").toLatin1())
            << QString(">A<A<A>A>A<A!A!A")
            << QString("aBcDefgh")
            << QString("AbcDEfgh")
            << QString("AbcDEfgh")
            << bool(insert_text);
        QTest::newRow(QString(insert_mode + "exact_case3").toLatin1())
            << QString(">a<a<a>a>a<a!a!a")
            << QString("AbCdEFGH")
            << QString("AbcDEfGH")
            << QString("AbcDEfGH")
            << bool(insert_text);
        QTest::newRow(QString(insert_mode + "exact_case4").toLatin1())
            << QString(">a<a<a>a>a<a!a!a")
            << QString("aBcDefgh")
            << QString("AbcDEfgh")
            << QString("AbcDEfgh")
            << bool(insert_text);
        QTest::newRow(QString(insert_mode + "exact_case5").toLatin1())
            << QString(">H<H<H>H>H<H!H!H")
            << QString("aBcDef01")
            << QString("AbcDEf01")
            << QString("AbcDEf01")
            << bool(insert_text);
        QTest::newRow(QString(insert_mode + "exact_case6").toLatin1())
            << QString(">h<h<h>h>h<h!h!h")
            << QString("aBcDef92")
            << QString("AbcDEf92")
            << QString("AbcDEf92")
            << bool(insert_text);

        QTest::newRow(QString(insert_mode + "illegal_keys1").toLatin1())
            << QString("AAAAAAAA")
            << QString("A2#a;.0!")
            << QString("Aa")
            << QString("Aa      ")
            << bool(insert_text);
        QTest::newRow(QString(insert_mode + "illegal_keys2").toLatin1())
            << QString("AAAA")
            << QString("f4f4f4f4")
            << QString("ffff")
            << QString("ffff")
            << bool(insert_text);
        QTest::newRow(QString(insert_mode + "blank=input").toLatin1())
            << QString("9999;0")
            << QString("2004")
            << QString("2004")
            << QString("2004")
            << bool(insert_text);
    }
}

void tst_QLineEdit::setInputMask()
{
    QFETCH(QString, mask);
    QFETCH(QString, input);
    QFETCH(QString, expectedText);
    QFETCH(QString, expectedDisplay);
    QFETCH(bool, insert_text);

    QEXPECT_FAIL( "keys blank=input", "To eat blanks or not? Known issue. Task 43172", Abort);
    QEXPECT_FAIL( "insert blank=input", "To eat blanks or not? Known issue. Task 43172", Abort);

    // First set the input mask
    testWidget->setInputMask(mask);

    // then either insert using insert() or keyboard
    if (insert_text) {
        testWidget->insert(input);
    } else {
        psKeyClick(testWidget, Qt::Key_Home);
        for (int i=0; i<input.length(); i++)
            QTest::keyClick(testWidget, input.at(i).toLatin1());
    }

    QCOMPARE(testWidget->text(), expectedText);
    QCOMPARE(testWidget->displayText(), expectedDisplay);
}

void tst_QLineEdit::inputMask_data()
{
    QTest::addColumn<QString>("mask");
    QTest::addColumn<QString>("expectedMask");

    // if no mask is set a nul string should be returned
    QTest::newRow("nul 1") << QString("") << QString();
    QTest::newRow("nul 2") << QString() << QString();

    // try different masks
    QTest::newRow("mask 1") << QString("000.000.000.000") << QString("000.000.000.000");
    QTest::newRow("mask 2") << QString("000.000.000.000;#") << QString("000.000.000.000;#");
    QTest::newRow("mask 3") << QString("AAA.aa.999.###;") << QString("AAA.aa.999.###");
    QTest::newRow("mask 4") << QString(">abcdef<GHIJK") << QString(">abcdef<GHIJK");

    // set an invalid input mask...
    // the current behaviour is that this exact (faulty) string is returned.
    QTest::newRow("invalid") << QString("ABCDEFGHIKLMNOP;") << QString("ABCDEFGHIKLMNOP");

    // verify that we can unset the mask again
    QTest::newRow("unset") << QString("") << QString();
}

void tst_QLineEdit::inputMask()
{
    QFETCH(QString, mask);
    QFETCH(QString, expectedMask);

    testWidget->setInputMask(mask);
    QCOMPARE(testWidget->inputMask(), expectedMask);
}

void tst_QLineEdit::clearInputMask()
{
    testWidget->setInputMask("000.000.000.000");
    QVERIFY(testWidget->inputMask() != QString::null);
    testWidget->setInputMask(QString::null);
    QCOMPARE(testWidget->inputMask(), QString());
}

void tst_QLineEdit::keypress_inputMask_data()
{
    QTest::addColumn<QString>("mask");
    QTest::addColumn<QTestEventList>("keys");
    QTest::addColumn<QString>("expectedText");
    QTest::addColumn<QString>("expectedDisplayText");

    {
        QTestEventList keys;
        // inserting 'A1.2B'
        keys.addKeyClick(Qt::Key_Home);
        keys.addKeyClick(Qt::Key_A);
        keys.addKeyClick(Qt::Key_1);
        keys.addKeyClick(Qt::Key_Period);
        keys.addKeyClick(Qt::Key_2);
        keys.addKeyClick(Qt::Key_B);
        QTest::newRow("jumping on period(separator)") << QString("000.000;_") << keys << QString("1.2") << QString("1__.2__");
    }
    {
        QTestEventList keys;
        // inserting 'A1.2B'
        keys.addKeyClick(Qt::Key_Home);
        keys.addKeyClick(Qt::Key_0);
        keys.addKeyClick(Qt::Key_Exclam);
        keys.addKeyClick('P');
        keys.addKeyClick(Qt::Key_3);
        QTest::newRow("jumping on input") << QString("D0.AA.XX.AA.00;_") << keys << QString("0..!P..3") << QString("_0.__.!P.__.3_");
    }
    {
        QTestEventList keys;
        // pressing delete
        keys.addKeyClick(Qt::Key_Home);
        keys.addKeyClick(Qt::Key_Delete);
        QTest::newRow("delete") << QString("000.000;_") << keys << QString(".") << QString("___.___");
    }
    {
        QTestEventList keys;
        // selecting all and delete
        keys.addKeyClick(Qt::Key_Home);
        keys.addKeyClick(Qt::Key_End, Qt::ShiftModifier);
        keys.addKeyClick(Qt::Key_Delete);
        QTest::newRow("deleting all") << QString("000.000;_") << keys << QString(".") << QString("___.___");
    }
    {
        QTestEventList keys;
        // inserting '12.12' then two backspaces
        keys.addKeyClick(Qt::Key_Home);
        keys.addKeyClick(Qt::Key_1);
        keys.addKeyClick(Qt::Key_2);
        keys.addKeyClick(Qt::Key_Period);
        keys.addKeyClick(Qt::Key_1);
        keys.addKeyClick(Qt::Key_2);
        keys.addKeyClick(Qt::Key_Backspace);
        keys.addKeyClick(Qt::Key_Backspace);
        QTest::newRow("backspace") << QString("000.000;_") << keys << QString("12.") << QString("12_.___");
    }
    {
        QTestEventList keys;
        // inserting '12ab'
        keys.addKeyClick(Qt::Key_Home);
        keys.addKeyClick(Qt::Key_1);
        keys.addKeyClick(Qt::Key_2);
        keys.addKeyClick(Qt::Key_A);
        keys.addKeyClick(Qt::Key_B);
        QTest::newRow("uppercase") << QString("9999 >AA;_") << keys << QString("12 AB") << QString("12__ AB");
    }
}

void tst_QLineEdit::keypress_inputMask()
{
    QFETCH(QString, mask);
    QFETCH(QTestEventList, keys);
    QFETCH(QString, expectedText);
    QFETCH(QString, expectedDisplayText);

    testWidget->setInputMask(mask);
    keys.simulate(testWidget);

    QCOMPARE(testWidget->text(), expectedText);
    QCOMPARE(testWidget->displayText(), expectedDisplayText);
}


void tst_QLineEdit::hasAcceptableInputMask_data()
{
    QTest::addColumn<QString>("optionalMask");
    QTest::addColumn<QString>("requiredMask");
    QTest::addColumn<QString>("invalid");
    QTest::addColumn<QString>("valid");

    QTest::newRow("Alphabetic optional and required")
        << QString("aaaa") << QString("AAAA") << QString("ab") << QString("abcd");
    QTest::newRow("Alphanumeric optional and require")
        << QString("nnnn") << QString("NNNN") << QString("R2") << QString("R2D2");
    QTest::newRow("Any optional and required")
        << QString("xxxx") << QString("XXXX") << QString("+-") << QString("+-*/");
    QTest::newRow("Numeric (0-9) required")
        << QString("0000") << QString("9999") << QString("11") << QString("1138");
    QTest::newRow("Numeric (1-9) optional and required")
        << QString("dddd") << QString("DDDD") << QString("12") << QString("1234");
}

void tst_QLineEdit::hasAcceptableInputMask()
{
    QFocusEvent lostFocus(QEvent::FocusOut);
    QFETCH(QString, optionalMask);
    QFETCH(QString, requiredMask);
    QFETCH(QString, invalid);
    QFETCH(QString, valid);

    // test that invalid input (for required) work for optionalMask
    testWidget->setInputMask(optionalMask);
    validInput = false;
    testWidget->setText(invalid);
    qApp->sendEvent(testWidget, &lostFocus);
    QVERIFY(validInput);

    // at the moment we don't strip the blank character if it is valid input, this makes the test between x vs X useless
    QEXPECT_FAIL( "Any optional and required", "To eat blanks or not? Known issue. Task 43172", Abort);

    // test requiredMask
    testWidget->setInputMask(requiredMask);
    validInput = true;
    testWidget->setText(invalid);
    validInput = testWidget->hasAcceptableInput();
    QVERIFY(!validInput);

    validInput = false;
    testWidget->setText(valid);
    qApp->sendEvent(testWidget, &lostFocus);
    QVERIFY(validInput);
}

static const int chars = 8;
class ValidatorWithFixup : public QValidator
{
public:
    ValidatorWithFixup(QWidget *parent = 0)
        : QValidator(parent)
    {}

    QValidator::State validate(QString &str, int &) const
    {
        const int s = str.size();
        if (s < chars) {
            return Intermediate;
        } else if (s > chars) {
            return Invalid;
        }
        return Acceptable;
    }

    void fixup(QString &str) const
    {
        str = str.leftJustified(chars, 'X', true);
    }
};



void tst_QLineEdit::hasAcceptableInputValidator()
{
    QFocusEvent lostFocus(QEvent::FocusOut);
    ValidatorWithFixup val;
    testWidget->setValidator(&val);
    testWidget->setText("foobar");
    qApp->sendEvent(testWidget, &lostFocus);
    QVERIFY(testWidget->hasAcceptableInput());
}



void tst_QLineEdit::maskCharacter_data()
{
    QTest::addColumn<QString>("mask");
    QTest::addColumn<QString>("input");
    QTest::addColumn<bool>("expectedValid");

    QTest::newRow("Hex") << QString("H")
                         << QString("0123456789abcdefABCDEF") << true;
    QTest::newRow("hex") << QString("h")
                         << QString("0123456789abcdefABCDEF") << true;
    QTest::newRow("HexInvalid") << QString("H")
                                << QString("ghijklmnopqrstuvwxyzGHIJKLMNOPQRSTUVWXYZ")
                                << false;
    QTest::newRow("hexInvalid") << QString("h")
                                << QString("ghijklmnopqrstuvwxyzGHIJKLMNOPQRSTUVWXYZ")
                                << false;
    QTest::newRow("Bin") << QString("B")
                         << QString("01") << true;
    QTest::newRow("bin") << QString("b")
                         << QString("01") << true;
    QTest::newRow("BinInvalid") << QString("B")
                                << QString("23456789qwertyuiopasdfghjklzxcvbnm")
                                << false;
    QTest::newRow("binInvalid") << QString("b")
                                << QString("23456789qwertyuiopasdfghjklzxcvbnm")
                                << false;
}

void tst_QLineEdit::maskCharacter()
{
    QFETCH(QString, mask);
    QFETCH(QString, input);
    QFETCH(bool, expectedValid);

    QFocusEvent lostFocus(QEvent::FocusOut);

    testWidget->setInputMask(mask);
    for (int i = 0; i < input.size(); ++i) {
        QString in = QString(input.at(i));
        QString expected = expectedValid ? in : QString();
        testWidget->setText(QString(input.at(i)));
        qApp->sendEvent(testWidget, &lostFocus);
        QCOMPARE(testWidget->text(), expected);
    }
}

#define NORMAL 0
#define REPLACE_UNTIL_END 1

void tst_QLineEdit::undo_data()
{
    QTest::addColumn<QStringList>("insertString");
    QTest::addColumn<IntList>("insertIndex");
    QTest::addColumn<IntList>("insertMode");
    QTest::addColumn<QStringList>("expectedString");
    QTest::addColumn<bool>("use_keys");

    for (int i=0; i<2; i++) {
        QString keys_str = "keyboard";
        bool use_keys = true;
        if (i==0) {
            keys_str = "insert";
            use_keys = false;
        }

        {
            IntList insertIndex;
            IntList insertMode;
            QStringList insertString;
            QStringList expectedString;

            insertIndex << -1;
            insertMode << NORMAL;
            insertString << "1";

            insertIndex << -1;
            insertMode << NORMAL;
            insertString << "5";

            insertIndex << 1;
            insertMode << NORMAL;
            insertString << "3";

            insertIndex << 1;
            insertMode << NORMAL;
            insertString << "2";

            insertIndex << 3;
            insertMode << NORMAL;
            insertString << "4";

            expectedString << "12345";
            expectedString << "1235";
            expectedString << "135";
            expectedString << "15";
            expectedString << "";

            QTest::newRow(QString(keys_str + "_numbers").toLatin1()) <<
                insertString <<
                insertIndex <<
                insertMode <<
                expectedString <<
                bool(use_keys);
        }
        {
            IntList insertIndex;
            IntList insertMode;
            QStringList insertString;
            QStringList expectedString;

            insertIndex << -1;
            insertMode << NORMAL;
            insertString << "World"; // World

            insertIndex << 0;
            insertMode << NORMAL;
            insertString << "Hello"; // HelloWorld

            insertIndex << 0;
            insertMode << NORMAL;
            insertString << "Well"; // WellHelloWorld

            insertIndex << 9;
            insertMode << NORMAL;
            insertString << "There"; // WellHelloThereWorld;

            expectedString << "WellHelloThereWorld";
            expectedString << "WellHelloWorld";
            expectedString << "HelloWorld";
            expectedString << "World";
            expectedString << "";

            QTest::newRow(QString(keys_str + "_helloworld").toLatin1()) <<
                insertString <<
                insertIndex <<
                insertMode <<
                expectedString <<
                bool(use_keys);
        }
        {
            IntList insertIndex;
            IntList insertMode;
            QStringList insertString;
            QStringList expectedString;

            insertIndex << -1;
            insertMode << NORMAL;
            insertString << "Ensuring";

            insertIndex << -1;
            insertMode << NORMAL;
            insertString << " instan";

            insertIndex << 9;
            insertMode << NORMAL;
            insertString << "an ";

            insertIndex << 10;
            insertMode << REPLACE_UNTIL_END;
            insertString << " unique instance.";

            expectedString << "Ensuring a unique instance.";
            expectedString << "Ensuring an instan";
            expectedString << "Ensuring instan";
            expectedString << "";

            QTest::newRow(QString(keys_str + "_patterns").toLatin1()) <<
                insertString <<
                insertIndex <<
                insertMode <<
                expectedString <<
                bool(use_keys);
        }
    }
}

void tst_QLineEdit::undo()
{
    QFETCH(QStringList, insertString);
    QFETCH(IntList, insertIndex);
    QFETCH(IntList, insertMode);
    QFETCH(QStringList, expectedString);
    QFETCH(bool, use_keys);

    QVERIFY(!testWidget->isUndoAvailable());

    int i;

// STEP 1: First build up an undo history by inserting or typing some strings...
    for (i=0; i<insertString.size(); ++i) {
        if (insertIndex[i] > -1)
            testWidget->setCursorPosition(insertIndex[i]);

 // experimental stuff
        if (insertMode[i] == REPLACE_UNTIL_END) {
            testWidget->setSelection(insertIndex[i], 8);

            // This is what I actually want...
            // QTest::keyClick(testWidget, Qt::Key_End, Qt::ShiftModifier);
        }

        if (use_keys)
            QTest::keyClicks(testWidget, insertString[i]);
        else
            testWidget->insert(insertString[i]);
    }

// STEP 2: Next call undo several times and see if we can restore to the previous state
    for (i=0; i<expectedString.size()-1; ++i) {
        QCOMPARE(testWidget->text(), expectedString[i]);
        QVERIFY(testWidget->isUndoAvailable());
        testWidget->undo();
    }

// STEP 3: Verify that we have undone everything
    QVERIFY(!testWidget->isUndoAvailable());
    QVERIFY(testWidget->text().isEmpty());


    if (m_keyboardScheme == QPlatformTheme::WindowsKeyboardScheme) {
        // Repeat the test using shortcut instead of undo()
        for (i=0; i<insertString.size(); ++i) {
            if (insertIndex[i] > -1)
                testWidget->setCursorPosition(insertIndex[i]);
            if (insertMode[i] == REPLACE_UNTIL_END)
                testWidget->setSelection(insertIndex[i], 8);
            if (use_keys)
                QTest::keyClicks(testWidget, insertString[i]);
            else
                testWidget->insert(insertString[i]);
        }
        for (i=0; i<expectedString.size()-1; ++i) {
            QCOMPARE(testWidget->text(), expectedString[i]);
            QVERIFY(testWidget->isUndoAvailable());
            QTest::keyClick(testWidget, Qt::Key_Backspace, Qt::AltModifier);
        }
    }

}

void tst_QLineEdit::redo_data()
{
    QTest::addColumn<QStringList>("insertString");
    QTest::addColumn<IntList>("insertIndex");
    QTest::addColumn<QStringList>("expectedString");

    {
        IntList insertIndex;
        QStringList insertString;
        QStringList expectedString;

        insertIndex << -1;
        insertString << "World"; // World
        insertIndex << 0;
        insertString << "Hello"; // HelloWorld
        insertIndex << 0;
        insertString << "Well"; // WellHelloWorld
        insertIndex << 9;
        insertString << "There"; // WellHelloThereWorld;

        expectedString << "World";
        expectedString << "HelloWorld";
        expectedString << "WellHelloWorld";
        expectedString << "WellHelloThereWorld";

        QTest::newRow("Inserts and setting cursor") << insertString << insertIndex << expectedString;
    }
}

void tst_QLineEdit::redo()
{
    QFETCH(QStringList, insertString);
    QFETCH(IntList, insertIndex);
    QFETCH(QStringList, expectedString);

    QVERIFY(!testWidget->isUndoAvailable());
    QVERIFY(!testWidget->isRedoAvailable());

    int i;
    // inserts the diff strings at diff positions
    for (i=0; i<insertString.size(); ++i) {
        if (insertIndex[i] > -1)
            testWidget->setCursorPosition(insertIndex[i]);
        testWidget->insert(insertString[i]);
    }

    QVERIFY(!testWidget->isRedoAvailable());

    // undo everything
    while (!testWidget->text().isEmpty())
        testWidget->undo();

    for (i=0; i<expectedString.size(); ++i) {
        QVERIFY(testWidget->isRedoAvailable());
        testWidget->redo();
        QCOMPARE(testWidget->text() , expectedString[i]);
    }

    QVERIFY(!testWidget->isRedoAvailable());


    if (m_keyboardScheme == QPlatformTheme::WindowsKeyboardScheme) {
        // repeat test, this time using shortcuts instead of undo()/redo()

        while (!testWidget->text().isEmpty())
            QTest::keyClick(testWidget, Qt::Key_Backspace, Qt::AltModifier);

        for (i = 0; i < expectedString.size(); ++i) {
            QVERIFY(testWidget->isRedoAvailable());
            QTest::keyClick(testWidget, Qt::Key_Backspace,
                            Qt::ShiftModifier | Qt::AltModifier);
            QCOMPARE(testWidget->text() , expectedString[i]);
        }

        QVERIFY(!testWidget->isRedoAvailable());
    }
}

void tst_QLineEdit::undo_keypressevents_data()
{
    QTest::addColumn<QTestEventList>("keys");
    QTest::addColumn<QStringList>("expectedString");

    {
        QTestEventList keys;
        QStringList expectedString;

        keys.addKeyClick('A');
        keys.addKeyClick('F');
        keys.addKeyClick('R');
        keys.addKeyClick('A');
        keys.addKeyClick('I');
        keys.addKeyClick('D');
        psKeyClick(keys, Qt::Key_Home);

        keys.addKeyClick('V');
        keys.addKeyClick('E');
        keys.addKeyClick('R');
        keys.addKeyClick('Y');

        keys.addKeyClick(Qt::Key_Left);
        keys.addKeyClick(Qt::Key_Left);
        keys.addKeyClick(Qt::Key_Left);
        keys.addKeyClick(Qt::Key_Left);

        keys.addKeyClick('B');
        keys.addKeyClick('E');
        psKeyClick(keys, Qt::Key_End);

        keys.addKeyClick(Qt::Key_Exclam);

        expectedString << "BEVERYAFRAID!";
        expectedString << "BEVERYAFRAID";
        expectedString << "VERYAFRAID";
        expectedString << "AFRAID";

        QTest::newRow("Inserts and moving cursor") << keys << expectedString;
    }

    {
        QTestEventList keys;
        QStringList expectedString;

        // inserting '1234'
        keys.addKeyClick(Qt::Key_1);
        keys.addKeyClick(Qt::Key_2);
        keys.addKeyClick(Qt::Key_3);
        keys.addKeyClick(Qt::Key_4);
        psKeyClick(keys, Qt::Key_Home);

        // skipping '12'
        keys.addKeyClick(Qt::Key_Right);
        keys.addKeyClick(Qt::Key_Right);

        // selecting '34'
        keys.addKeyClick(Qt::Key_Right, Qt::ShiftModifier);
        keys.addKeyClick(Qt::Key_Right, Qt::ShiftModifier);

        // deleting '34'
        keys.addKeyClick(Qt::Key_Delete);

        expectedString << "12";
        expectedString << "1234";

        QTest::newRow("Inserts,moving,selection and delete") << keys << expectedString;
    }

    {
        QTestEventList keys;
        QStringList expectedString;

        // inserting 'AB12'
        keys.addKeyClick('A');
        keys.addKeyClick('B');

        keys.addKeyClick(Qt::Key_1);
        keys.addKeyClick(Qt::Key_2);

        psKeyClick(keys, Qt::Key_Home);

        // selecting 'AB'
        keys.addKeyClick(Qt::Key_Right, Qt::ShiftModifier);
        keys.addKeyClick(Qt::Key_Right, Qt::ShiftModifier);

        // deleting 'AB'
        keys.addKeyClick(Qt::Key_Delete);

        // undoing deletion of 'AB'
        keys.addKeyClick(Qt::Key_Z, Qt::ControlModifier);

        // unselect any current selection
        keys.addKeyClick(Qt::Key_Right);
#ifdef Q_OS_WIN //Mac has a specialcase to handle jumping to the end of a selection
        keys.addKeyClick(Qt::Key_Left);
#endif

        // selecting '12'
        keys.addKeyClick(Qt::Key_Right, Qt::ShiftModifier);
        keys.addKeyClick(Qt::Key_Right, Qt::ShiftModifier);

        // deleting '12'
        keys.addKeyClick(Qt::Key_Delete);

        expectedString << "AB";
        expectedString << "AB12";

        QTest::newRow("Inserts,moving,selection, delete and undo") << keys << expectedString;
    }

    {
        QTestEventList keys;
        QStringList expectedString;

        // inserting 'ABCD'
        keys.addKeyClick(Qt::Key_A);
        keys.addKeyClick(Qt::Key_B);
        keys.addKeyClick(Qt::Key_C);
        keys.addKeyClick(Qt::Key_D);

        //move left two
        keys.addKeyClick(Qt::Key_Left);
        keys.addKeyClick(Qt::Key_Left);

        // inserting '1234'
        keys.addKeyClick(Qt::Key_1);
        keys.addKeyClick(Qt::Key_2);
        keys.addKeyClick(Qt::Key_3);
        keys.addKeyClick(Qt::Key_4);

        // selecting '1234'
        keys.addKeyClick(Qt::Key_Left, Qt::ShiftModifier);
        keys.addKeyClick(Qt::Key_Left, Qt::ShiftModifier);
        keys.addKeyClick(Qt::Key_Left, Qt::ShiftModifier);
        keys.addKeyClick(Qt::Key_Left, Qt::ShiftModifier);

        // overwriting '1234' with '5'
        keys.addKeyClick(Qt::Key_5);

        // undoing deletion of 'AB'
        keys.addKeyClick(Qt::Key_Z, Qt::ControlModifier);

        // overwriting '1234' with '6'
        keys.addKeyClick(Qt::Key_6);

        expectedString << "ab6cd";
        // for versions previous to 3.2 we overwrite needed two undo operations
        expectedString << "ab1234cd";
        expectedString << "abcd";

        QTest::newRow("Inserts,moving,selection and undo, removing selection") << keys << expectedString;
    }

    {
        QTestEventList keys;
        QStringList expectedString;

        // inserting 'ABC'
        keys.addKeyClick('A');
        keys.addKeyClick('B');
        keys.addKeyClick('C');

        // removes 'C'
        keys.addKeyClick(Qt::Key_Backspace);

        expectedString << "AB";
        expectedString << "ABC";

        QTest::newRow("Inserts,backspace") << keys << expectedString;
    }

    {
        QTestEventList keys;
        QStringList expectedString;

        // inserting 'ABC'
        keys.addKeyClick('A');
        keys.addKeyClick('B');
        keys.addKeyClick('C');

        // removes 'C'
        keys.addKeyClick(Qt::Key_Backspace);

        // inserting 'Z'
        keys.addKeyClick('Z');

        expectedString << "ABZ";
        expectedString << "AB";
        expectedString << "ABC";

        QTest::newRow("Inserts,backspace,inserts") << keys << expectedString;
    }


    {
        QTestEventList keys;
        QStringList expectedString;

        // inserting '123'
        keys.addKeyClick(Qt::Key_1);
        keys.addKeyClick(Qt::Key_2);
        keys.addKeyClick(Qt::Key_3);
        psKeyClick(keys, Qt::Key_Home);

        // selecting '123'
        psKeyClick(keys, Qt::Key_End, Qt::ShiftModifier);

        // overwriting '123' with 'ABC'
        keys.addKeyClick('A');
        keys.addKeyClick('B');
        keys.addKeyClick('C');

        expectedString << "ABC";
        // for versions previous to 3.2 we overwrite needed two undo operations
        expectedString << "123";

        QTest::newRow("Inserts,moving,selection and overwriting") << keys << expectedString;
    }
}

void tst_QLineEdit::undo_keypressevents()
{
    QFETCH(QTestEventList, keys);
    QFETCH(QStringList, expectedString);

    keys.simulate(testWidget);

    for (int i=0; i<expectedString.size(); ++i) {
        QCOMPARE(testWidget->text() , expectedString[i]);
        testWidget->undo();
    }
    QVERIFY(testWidget->text().isEmpty());
}

#ifndef QT_NO_CLIPBOARD
void tst_QLineEdit::QTBUG5786_undoPaste()
{
    if (!PlatformClipboard::isAvailable())
	   QSKIP("this machine doesn't support the clipboard");
    QString initial("initial");
    QString string("test");
    QString additional("add");
    QApplication::clipboard()->setText(string);
    QLineEdit edit(initial);
    QCOMPARE(edit.text(), initial);
    edit.paste();
    QCOMPARE(edit.text(), initial + string);
    edit.paste();
    QCOMPARE(edit.text(), initial + string + string);
    edit.insert(additional);
    QCOMPARE(edit.text(), initial + string + string + additional);
    edit.undo();
    QCOMPARE(edit.text(), initial + string + string);
    edit.undo();
    QCOMPARE(edit.text(), initial + string);
    edit.undo();
    QCOMPARE(edit.text(), initial);
    edit.selectAll();
    QApplication::clipboard()->setText(QString());
    edit.paste();
    QVERIFY(edit.text().isEmpty());

}
#endif


void tst_QLineEdit::clear()
{
    // checking that clear of empty/nullstring doesn't add to undo history
    int max = 5000;
    while (max > 0 && testWidget->isUndoAvailable()) {
        max--;
        testWidget->undo();
    }

    testWidget->clear();
//    QVERIFY(!testWidget->isUndoAvailable());

    // checks that clear actually clears
    testWidget->insert("I am Legend");
    testWidget->clear();
    QVERIFY(testWidget->text().isEmpty());

    // checks that clears can be undone
    testWidget->undo();
    QCOMPARE(testWidget->text(), QString("I am Legend"));
}

void tst_QLineEdit::editingFinished()
{
    if (testWidget->hasAcceptableInput())
        validInput = true;
    else
        validInput = false;
}

void tst_QLineEdit::text_data()
{
    QTest::addColumn<QString>("insertString");

    QTest::newRow("Plain text0") << QString("Hello World");
    QTest::newRow("Plain text1") << QString("");
    QTest::newRow("Plain text2") << QString("A");
    QTest::newRow("Plain text3") << QString("ryyryryryryryryryryryryryryryryryryryryryryryryryryryrryryryryryryryryryryryryryryryryryryryryryryryryryryryryryryryryryrryryryryryryryryryryryryry");
    QTest::newRow("Plain text4") << QString("abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890`~!@#$%^&*()_-+={[}]|\\:;'?/>.<,\"");
    QTest::newRow("Newlines") << QString("A\nB\nC\n");
    QTest::newRow("Text with nbsp") << QString("Hello") + QChar(0xa0) + "World";
}

void tst_QLineEdit::text()
{
    QFETCH(QString, insertString);
    testWidget->setText(insertString);
    QCOMPARE(testWidget->text(), insertString);
}

void tst_QLineEdit::textMask_data()
{
    QTest::addColumn<QString>("insertString");

    QTest::newRow( "Plain text1" ) << QString( "" );
}

void tst_QLineEdit::textMask()
{
    QFETCH( QString, insertString );
    testWidget->setInputMask( "#" );
    testWidget->setText( insertString );
    QCOMPARE( testWidget->text(), insertString );
}

void tst_QLineEdit::setText()
{
    QSignalSpy editedSpy(testWidget, SIGNAL(textEdited(QString)));
    QSignalSpy changedSpy(testWidget, SIGNAL(textChanged(QString)));
    testWidget->setText("hello");
    QCOMPARE(editedSpy.count(), 0);
    QCOMPARE(changedSpy.value(0).value(0).toString(), QString("hello"));
}

void tst_QLineEdit::displayText_data()
{
    QTest::addColumn<QString>("insertString");
    QTest::addColumn<QString>("expectedString");
    QTest::addColumn<QLineEdit::EchoMode>("mode");
    QTest::addColumn<bool>("use_setText");

    QString s;
    QLineEdit::EchoMode m;

    for (int i=0; i<2; i++) {
        QString key_mode_str;
        bool use_setText;
        if (i==0) {
            key_mode_str = "setText_";
            use_setText = true;
        } else {
            key_mode_str = "useKeys_";
            use_setText = false;
        }
        s = key_mode_str + "Normal";
        m = QLineEdit::Normal;
        QTest::newRow(QString(s + " text0").toLatin1()) << QString("Hello World") <<
                      QString("Hello World") <<
                      m << bool(use_setText);
        QTest::newRow(QString(s + " text1").toLatin1()) << QString("") <<
                      QString("") <<
                      m << bool(use_setText);
        QTest::newRow(QString(s + " text2").toLatin1()) << QString("A") <<
                      QString("A") <<
                      m << bool(use_setText);
        QTest::newRow(QString(s + " text3").toLatin1()) << QString("ryyryryryryryryryryryryryryryryryryryryryryryryryryryrryryryryryryryryryryryryryryryryryryryryryryryryryryryryryryryryryrryryryryryryryryryryryryry") <<
                      QString("ryyryryryryryryryryryryryryryryryryryryryryryryryryryrryryryryryryryryryryryryryryryryryryryryryryryryryryryryryryryryryrryryryryryryryryryryryryry") <<
                      m << bool(use_setText);
        QTest::newRow(QString(s + " text4").toLatin1()) << QString("abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890`~!@#$%^&*()_-+={[}]|\\:;'?/>.<,\"") <<
                      QString("abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890`~!@#$%^&*()_-+={[}]|\\:;'?/>.<,\"") <<
                      m << bool(use_setText);
        QTest::newRow(QString(s + " text with nbsp").toLatin1()) << QString("Hello") + QChar(0xa0) + "World" <<
                      QString("Hello") + QChar(0xa0) + "World" <<
                      m << bool(use_setText);
        s = key_mode_str + "NoEcho";
        m = QLineEdit::NoEcho;
        QTest::newRow(QString(s + " text0").toLatin1()) << QString("Hello World") <<
                      QString("") <<
                      m << bool(use_setText);
        QTest::newRow(QString(s + " text1").toLatin1()) << QString("") <<
                      QString("") <<
                      m << bool(use_setText);
        QTest::newRow(QString(s + " text2").toLatin1()) << QString("A") <<
                      QString("") <<
                      m << bool(use_setText);
        QTest::newRow(QString(s + " text3").toLatin1()) << QString("ryyryryryryryryryryryryryryryryryryryryryryryryryryryrryryryryryryryryryryryryryryryryryryryryryryryryryryryryryryryryryrryryryryryryryryryryryryry") <<
                      QString("") <<
                      m << bool(use_setText);
        QTest::newRow(QString(s + " text4").toLatin1()) << QString("abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890`~!@#$%^&*()_-+={[}]|\\:;'?/>.<,\"") <<
                      QString("") <<
                      m << bool(use_setText);
        QTest::newRow(QString(s + " text with nbsp").toLatin1()) << QString("Hello") + QChar(0xa0) + "World" <<
                      QString("") <<
                      m << bool(use_setText);
        s = key_mode_str + "Password";
        m = QLineEdit::Password;
        QChar passChar = qApp->style()->styleHint(QStyle::SH_LineEdit_PasswordCharacter, 0, testWidget);
        QString input;
        QString pass;
        input = "Hello World";
        pass.resize(input.length());
        pass.fill(passChar);
        QTest::newRow(QString(s + " text0").toLatin1()) << input << pass << m << bool(use_setText);
        QTest::newRow(QString(s + " text1").toLatin1()) << QString("") <<
                      QString("") <<
                      m << bool(use_setText);
        QTest::newRow(QString(s + " text2").toLatin1()) << QString("A") << QString(passChar) << m << bool(use_setText);
        input = QString("ryyryryryryryryryryryryryryryryryryryryryryryryryryryrryryryryryryryryryryryryryryryryryryryryryryryryryryryryryryryryryrryryryryryryryryryryryryry");
        pass.resize(input.length());
        pass.fill(passChar);
        QTest::newRow(QString(s + " text3").toLatin1()) << input << pass << m << bool(use_setText);
        input = QString("abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890`~!@#$%^&*()_-+={[}]|\\:;'?/>.<,\"");
        pass.fill(passChar, input.length());
        QTest::newRow(QString(s + " text4").toLatin1()) << input << pass << m << bool(use_setText);
        input = QString("Hello") + QChar(0xa0) + "World";
        pass.resize(input.length());
        pass.fill(passChar);
        QTest::newRow(QString(s + " text with nbsp").toLatin1()) << input << pass << m << bool(use_setText);
    }
}

void tst_QLineEdit::displayText()
{
    QFETCH(QString, insertString);
    QFETCH(QString, expectedString);
    QFETCH(QLineEdit::EchoMode, mode);
    //QFETCH(bool, use_setText);  Currently unused.

    testWidget->setEchoMode(mode);
    testWidget->setText(insertString);
    QCOMPARE(testWidget->displayText(), expectedString);
    QVERIFY(testWidget->echoMode() == mode);
}

void tst_QLineEdit::passwordEchoOnEdit()
{
    QStyleOptionFrameV2 opt;
    QChar fillChar = testWidget->style()->styleHint(QStyle::SH_LineEdit_PasswordCharacter, &opt, testWidget);

    testWidget->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    testWidget->setFocus();
    testWidget->raise();
    QTRY_VERIFY(testWidget->hasFocus());

    QTest::keyPress(testWidget, '0');
    QTest::keyPress(testWidget, '1');
    QTest::keyPress(testWidget, '2');
    QTest::keyPress(testWidget, '3');
    QTest::keyPress(testWidget, '4');
    QCOMPARE(testWidget->displayText(), QString("01234"));
    testWidget->clearFocus();
    QVERIFY(!testWidget->hasFocus());
    QCOMPARE(testWidget->displayText(), QString(5, fillChar));
    testWidget->setFocus();
    QTRY_VERIFY(testWidget->hasFocus());

    QCOMPARE(testWidget->displayText(), QString(5, fillChar));
    QTest::keyPress(testWidget, '0');
    QCOMPARE(testWidget->displayText(), QString("0"));

    // restore clean state
    testWidget->setEchoMode(QLineEdit::Normal);
}

void tst_QLineEdit::passwordEchoDelay()
{
    int delay = qGuiApp->styleHints()->passwordMaskDelay();
#if defined QT_BUILD_INTERNAL
    QLineEditPrivate *priv = QLineEditPrivate::get(testWidget);
    QWidgetLineControl *control = priv->control;
    control->m_passwordMaskDelayOverride = 200;
    delay = 200;
#endif
    if (delay <= 0)
        QSKIP("Platform not defining echo delay and overriding only possible in internal build");

    QStyleOptionFrameV2 opt;
    QChar fillChar = testWidget->style()->styleHint(QStyle::SH_LineEdit_PasswordCharacter, &opt, testWidget);

    testWidget->setEchoMode(QLineEdit::Password);
    testWidget->setFocus();
    testWidget->raise();
    QTRY_VERIFY(testWidget->hasFocus());

    QTest::keyPress(testWidget, '0');
    QTest::keyPress(testWidget, '1');
    QTest::keyPress(testWidget, '2');
    QCOMPARE(testWidget->displayText(), QString(2, fillChar) + QLatin1Char('2'));
    QTest::keyPress(testWidget, '3');
    QTest::keyPress(testWidget, '4');
    QCOMPARE(testWidget->displayText(), QString(4, fillChar) + QLatin1Char('4'));
    QTest::keyPress(testWidget, Qt::Key_Backspace);
    QCOMPARE(testWidget->displayText(), QString(4, fillChar));
    QTest::keyPress(testWidget, '4');
    QCOMPARE(testWidget->displayText(), QString(4, fillChar) + QLatin1Char('4'));
    QTest::qWait(delay);
    QTRY_COMPARE(testWidget->displayText(), QString(5, fillChar));
    QTest::keyPress(testWidget, '5');
    QCOMPARE(testWidget->displayText(), QString(5, fillChar) + QLatin1Char('5'));
    testWidget->clearFocus();
    QVERIFY(!testWidget->hasFocus());
    QCOMPARE(testWidget->displayText(), QString(6, fillChar));
    testWidget->setFocus();
    QTRY_VERIFY(testWidget->hasFocus());
    QCOMPARE(testWidget->displayText(), QString(6, fillChar));
    QTest::keyPress(testWidget, '6');
    QCOMPARE(testWidget->displayText(), QString(6, fillChar) + QLatin1Char('6'));

    QInputMethodEvent ev;
    ev.setCommitString(QLatin1String("7"));
    QApplication::sendEvent(testWidget, &ev);
    QCOMPARE(testWidget->displayText(), QString(7, fillChar) + QLatin1Char('7'));

    testWidget->setCursorPosition(3);
    QCOMPARE(testWidget->displayText(), QString(7, fillChar) + QLatin1Char('7'));
    QTest::keyPress(testWidget, 'a');
    QCOMPARE(testWidget->displayText(), QString(3, fillChar) + QLatin1Char('a') + QString(5, fillChar));
    QTest::keyPress(testWidget, Qt::Key_Backspace);
    QCOMPARE(testWidget->displayText(), QString(8, fillChar));

    // restore clean state
    testWidget->setEchoMode(QLineEdit::Normal);
}

void tst_QLineEdit::maxLength_mask_data()
{
    QTest::addColumn<QString>("mask");
    QTest::addColumn<int>("expectedLength");

    QTest::newRow("mask_case") << QString(">000<>00<000") << 8;
    QTest::newRow("mask_nocase") << QString("00000000") << 8;
    QTest::newRow("mask_null") << QString() << 32767;
    QTest::newRow("mask_escape") << QString("\\A\\aAA") << 4;
}

void tst_QLineEdit::maxLength_mask()
{
    QFETCH(QString, mask);
    QFETCH(int, expectedLength);

    testWidget->setInputMask(mask);

    QCOMPARE(testWidget->maxLength(), expectedLength);
}

void tst_QLineEdit::maxLength_data()
{
    QTest::addColumn<QString>("insertString");
    QTest::addColumn<QString>("expectedString");
    QTest::addColumn<int>("length");
    QTest::addColumn<bool>("insertBeforeSettingMaxLength");
    QTest::addColumn<bool>("use_setText");

    QTest::newRow("keyclick before0") << QString("this is a test.") << QString("this is a test.") << 20 << bool(true) << bool(false);
    QTest::newRow("keyclick before1") << QString("this is a test.") << QString("this is a ") << 10 << bool(true) << bool(false);
    QTest::newRow("keyclick after0") << QString("this is a test.") << QString("this is a test.") << 20 << bool(false) << bool(false);
    QTest::newRow("keyclick after1") << QString("this is a test.") << QString("this is a ") << 10 << bool(false) << bool(false);
    QTest::newRow("settext before0") << QString("this is a test.") << QString("this is a test.") << 20 << bool(true) << bool(true);
    QTest::newRow("settext before1") << QString("this is a test.") << QString("this is a ") << 10 << bool(true) << bool(true);
    QTest::newRow("settext after0") << QString("this is a test.") << QString("this is a test.") << 20 << bool(false) << bool(true);
    QTest::newRow("settext after1") << QString("this is a test.") << QString("this is a ") << 10 << bool(false) << bool(true);
}

void tst_QLineEdit::maxLength()
{
    QFETCH(QString, insertString);
    QFETCH(QString, expectedString);
    QFETCH(int, length);
    QFETCH(bool, insertBeforeSettingMaxLength);
    QFETCH(bool, use_setText);

    // in some cases we set the maxLength _before_ entering the text.
    if (!insertBeforeSettingMaxLength)
        testWidget->setMaxLength(length);

    // I expect MaxLength to work BOTH with entering live characters AND with setting the text.
    if (use_setText) {
        // Enter insertString using setText.
        testWidget->setText(insertString);
    } else {
        // Enter insertString as a sequence of keyClicks
        QTest::keyClicks(testWidget, insertString);
    }

    // in all other cases we set the maxLength _after_ entering the text.
    if (insertBeforeSettingMaxLength) {
        changed_count = 0;
        testWidget->setMaxLength(length);

        // Make sure that the textChanged is not emitted unless the text is actually changed
        if (insertString == expectedString) {
            QVERIFY(changed_count == 0);
        } else {
            QVERIFY(changed_count == 1);
        }
    }

    // and check if we get the expected string back
    QCOMPARE(testWidget->text(), expectedString);
}

void tst_QLineEdit::isReadOnly()
{
    QVERIFY(!testWidget->isReadOnly());

    // start with a basic text
    QTest::keyClicks(testWidget, "the quick brown fox");
    QCOMPARE(testWidget->text(), QString("the quick brown fox"));

    // do a quick check to verify that we can indeed edit the text
    testWidget->home(false);
    testWidget->cursorForward(false, 10);
    QTest::keyClicks(testWidget, "dark ");
    QCOMPARE(testWidget->text(), QString("the quick dark brown fox"));

    testWidget->setReadOnly(true);
    QVERIFY(testWidget->isReadOnly());

    // verify that we cannot edit the text anymore
    testWidget->home(false);
    testWidget->cursorForward(false, 10);
    QTest::keyClick(testWidget, Qt::Key_Delete);
    QCOMPARE(testWidget->text(), QString("the quick dark brown fox"));
    testWidget->cursorForward(false, 10);
    QTest::keyClicks(testWidget, "this should not have any effect!! ");
    QCOMPARE(testWidget->text(), QString("the quick dark brown fox"));
}

class BlinkTestLineEdit : public QLineEdit
{
public:
    void paintEvent(QPaintEvent *e)
    {
        ++updates;
        QLineEdit::paintEvent(e);
    }

    int updates;
};

void tst_QLineEdit::noCursorBlinkWhenReadOnly()
{
    int cursorFlashTime = QApplication::cursorFlashTime();
    if (cursorFlashTime == 0)
        return;
    BlinkTestLineEdit le;
    le.show();
    le.setFocus();
    QTest::qWaitForWindowActive(&le);
    le.updates = 0;
    QTest::qWait(cursorFlashTime);
    QVERIFY(le.updates > 0);
    le.setReadOnly(true);
    QTest::qWait(10);
    le.updates = 0;
    QTest::qWait(cursorFlashTime);
    QCOMPARE(le.updates, 0);
    le.setReadOnly(false);
    QTest::qWait(10);
    le.updates = 0;
    QTest::qWait(cursorFlashTime);
    QVERIFY(le.updates > 0);
}

static void figureOutProperKey(Qt::Key &key, Qt::KeyboardModifiers &pressState)
{
#ifdef Q_OS_MAC
    static bool tst_lineedit_randomized = false;
    // Mac has 3 different ways of accomplishing this (same for moving to the back)
    // So I guess we should just randomly do this for now. Which may get people mad, but if
    // we fail at one point, it's just a matter of setting roll to the correct value
    // instead of random.

    if (!tst_lineedit_randomized) {
        tst_lineedit_randomized = true;
        ::srandom(ulong(time(0)));
    }
    long roll = ::random() % 3;
    switch (roll) {
    case 0:
        key = key == Qt::Key_Home ? Qt::Key_Up : Qt::Key_Down;
        break;
    case 1:
    case 2:
        key = key == Qt::Key_Home ? Qt::Key_Left : Qt::Key_Right;
        pressState |= (roll == 1) ? Qt::ControlModifier : Qt::MetaModifier;
        break;
    }
#else
    // Naively kill the warning.
    key = key;
    pressState = pressState;
#endif
}

// Platform specific move. Home and End do nothing on the Mac,
// so do something a bit smarter than tons of #ifdefs
void tst_QLineEdit::psKeyClick(QWidget *target, Qt::Key key, Qt::KeyboardModifiers pressState)
{
    figureOutProperKey(key, pressState);
    QTest::keyClick(target, key, pressState);
}

void tst_QLineEdit::psKeyClick(QTestEventList &keys, Qt::Key key, Qt::KeyboardModifiers pressState)
{
    figureOutProperKey(key, pressState);
    keys.addKeyClick(key, pressState);
}

void tst_QLineEdit::cursorPosition()
{
    QVERIFY(testWidget->cursorPosition() == 0);

    // start with a basic text
    QTest::keyClicks(testWidget, "The");
    QCOMPARE(testWidget->cursorPosition(), 3);
    QTest::keyClicks(testWidget, " quick");
    QCOMPARE(testWidget->cursorPosition(), 9);
    QTest::keyClicks(testWidget, " brown fox jumps over the lazy dog");
    QCOMPARE(testWidget->cursorPosition(), 43);

    // The text we have now is:
    //           1         2         3         4         5
    // 012345678901234567890123456789012345678901234567890
    // The quick brown fox jumps over the lazy dog

    // next we will check some of the cursor movement functions
    testWidget->end(false);
    QCOMPARE(testWidget->cursorPosition() , 43);
    testWidget->cursorForward(false, -1);
    QCOMPARE(testWidget->cursorPosition() , 42);
    testWidget->cursorBackward(false, 1);
    QCOMPARE(testWidget->cursorPosition() , 41);
    testWidget->home(false);
    QCOMPARE(testWidget->cursorPosition() , 0);
    testWidget->cursorForward(false, 1);
    QCOMPARE(testWidget->cursorPosition() , 1);
    testWidget->cursorForward(false, 9);
    QCOMPARE(testWidget->cursorPosition() , 10);
    testWidget->cursorWordForward(false); // 'fox'
    QCOMPARE(testWidget->cursorPosition(), 16);
    testWidget->cursorWordForward(false); // 'jumps'
    QCOMPARE(testWidget->cursorPosition(), 20);
    testWidget->cursorWordBackward(false); // 'fox'
    QCOMPARE(testWidget->cursorPosition(), 16);
    testWidget->cursorWordBackward(false); // 'brown'
    testWidget->cursorWordBackward(false); // 'quick'
    testWidget->cursorWordBackward(false); // 'The'
    QCOMPARE(testWidget->cursorPosition(), 0);
    testWidget->cursorWordBackward(false); // 'The'
    QCOMPARE(testWidget->cursorPosition(), 0);

    // Cursor position should be 0 here!
    int i;
    for (i=0; i<5; i++)
        QTest::keyClick(testWidget, Qt::Key_Right);
    QCOMPARE(testWidget->cursorPosition(), 5);
    psKeyClick(testWidget, Qt::Key_End);
    QCOMPARE(testWidget->cursorPosition(), 43);
    QTest::keyClick(testWidget, Qt::Key_Left);
    QCOMPARE(testWidget->cursorPosition(), 42);
    QTest::keyClick(testWidget, Qt::Key_Left);
    QCOMPARE(testWidget->cursorPosition(), 41);
    psKeyClick(testWidget, Qt::Key_Home);
    QCOMPARE(testWidget->cursorPosition(), 0);

    // cursorposition when maxlength is set
    int maxLength = 9;
    testWidget->clear();
    testWidget->setMaxLength(maxLength);
    QCOMPARE(testWidget->cursorPosition() , 0);
    QTest::keyClicks(testWidget, "123ABC123");
    QCOMPARE(testWidget->cursorPosition() , maxLength);
    psKeyClick(testWidget, Qt::Key_Home);
    QCOMPARE(testWidget->cursorPosition() , 0);
    psKeyClick(testWidget, Qt::Key_End);
    QCOMPARE(testWidget->cursorPosition() , maxLength);
}

/* // tested in cursorPosition
void tst_QLineEdit::cursorLeft()
void tst_QLineEdit::cursorRight()
void tst_QLineEdit::cursorForward()
void tst_QLineEdit::cursorBackward()
void tst_QLineEdit::cursorWordForward()
void tst_QLineEdit::cursorWordBackward()
void tst_QLineEdit::home()
void tst_QLineEdit::end()
*/

void tst_QLineEdit::cursorPositionChanged_data()
{
    QTest::addColumn<QTestEventList>("input");
    QTest::addColumn<int>("lastPos");
    QTest::addColumn<int>("newPos");

    QTestEventList keys;
    keys.addKeyClick(Qt::Key_A);
    QTest::newRow("a") << keys << 0 << 1;
    keys.clear();

    keys.addKeyClick(Qt::Key_A);
    keys.addKeyClick(Qt::Key_B);
    keys.addKeyClick(Qt::Key_C);
    psKeyClick(keys, Qt::Key_Home);
    QTest::newRow("abc<home>") << keys << 3 << 0;
    keys.clear();

    keys.addKeyClick(Qt::Key_A);
    keys.addKeyClick(Qt::Key_B);
    keys.addKeyClick(Qt::Key_C);
    keys.addKeyClick(Qt::Key_Left);
    QTest::newRow("abc<left>") << keys << 3 << 2;
    keys.clear();

    keys.addKeyClick(Qt::Key_A);
    keys.addKeyClick(Qt::Key_B);
    keys.addKeyClick(Qt::Key_C);
    keys.addKeyClick(Qt::Key_Right);
    QTest::newRow("abc<right>") << keys << 2 << 3;
    keys.clear();

    keys.addKeyClick(Qt::Key_A);
    keys.addKeyClick(Qt::Key_B);
    keys.addKeyClick(Qt::Key_C);
    psKeyClick(keys, Qt::Key_Home);
    keys.addKeyClick(Qt::Key_Right);
    QTest::newRow("abc<home><right>") << keys << 0 << 1;
    keys.clear();

    keys.addKeyClick(Qt::Key_A);
    keys.addKeyClick(Qt::Key_B);
    keys.addKeyClick(Qt::Key_C);
    keys.addKeyClick(Qt::Key_Backspace);
    QTest::newRow("abc<backspace>") << keys << 3 << 2;
    keys.clear();

    keys.addKeyClick(Qt::Key_A);
    keys.addKeyClick(Qt::Key_B);
    keys.addKeyClick(Qt::Key_C);
    keys.addKeyClick(Qt::Key_Delete);
    QTest::newRow("abc<delete>") << keys << 2 << 3;
    keys.clear();

    keys.addKeyClick(Qt::Key_A);
    keys.addKeyClick(Qt::Key_B);
    keys.addKeyClick(Qt::Key_C);
    keys.addKeyClick(Qt::Key_Left);
    keys.addKeyClick(Qt::Key_Delete);
    QTest::newRow("abc<left><delete>") << keys << 3 << 2;
    keys.clear();

    keys.addKeyClick(Qt::Key_A);
    keys.addKeyClick(Qt::Key_B);
    keys.addKeyClick(Qt::Key_C);
    psKeyClick(keys, Qt::Key_Home);
    psKeyClick(keys, Qt::Key_End);
    QTest::newRow("abc<home><end>") << keys << 0 << 3;
    keys.clear();

    keys.addKeyClick(Qt::Key_A);
    keys.addKeyClick(Qt::Key_B);
    keys.addKeyClick(Qt::Key_C);
    keys.addKeyClick(Qt::Key_Space);
    keys.addKeyClick(Qt::Key_D);
    keys.addKeyClick(Qt::Key_E);
    keys.addKeyClick(Qt::Key_F);
    keys.addKeyClick(Qt::Key_Home);
    keys.addKeyClick(Qt::Key_Right, Qt::ControlModifier);
    QTest::newRow("abc efg<home><ctrl-right>") << keys
#ifndef Q_OS_MAC
        << 0 << 4;
#else
        << 6 << 7;
#endif
    keys.clear();

#ifdef Q_OS_MAC
    keys.addKeyClick(Qt::Key_A);
    keys.addKeyClick(Qt::Key_B);
    keys.addKeyClick(Qt::Key_C);
    keys.addKeyClick(Qt::Key_Space);
    keys.addKeyClick(Qt::Key_D);
    keys.addKeyClick(Qt::Key_E);
    keys.addKeyClick(Qt::Key_F);
    keys.addKeyClick(Qt::Key_Up);
    keys.addKeyClick(Qt::Key_Right, Qt::AltModifier);
    QTest::newRow("mac equivalent abc efg<up><option-right>") << keys << 0 << 4;
    keys.clear();
#endif

    keys.addKeyClick(Qt::Key_A);
    keys.addKeyClick(Qt::Key_B);
    keys.addKeyClick(Qt::Key_C);
    keys.addKeyClick(Qt::Key_Space);
    keys.addKeyClick(Qt::Key_D);
    keys.addKeyClick(Qt::Key_E);
    keys.addKeyClick(Qt::Key_F);
    keys.addKeyClick(Qt::Key_Left, Qt::ControlModifier);
    QTest::newRow("abc efg<ctrl-left>") << keys << 7
#ifndef Q_OS_MAC
        << 4;
#else
        << 0;
#endif
    keys.clear();
#ifdef Q_OS_MAC
    keys.addKeyClick(Qt::Key_A);
    keys.addKeyClick(Qt::Key_B);
    keys.addKeyClick(Qt::Key_C);
    keys.addKeyClick(Qt::Key_Space);
    keys.addKeyClick(Qt::Key_D);
    keys.addKeyClick(Qt::Key_E);
    keys.addKeyClick(Qt::Key_F);
    keys.addKeyClick(Qt::Key_Left, Qt::AltModifier);
    QTest::newRow("mac equivalent abc efg<option-left>") << keys << 7 << 4;
    keys.clear();
#endif
}


void tst_QLineEdit::cursorPositionChanged()
{
    QFETCH(QTestEventList, input);
    QFETCH(int, lastPos);
    QFETCH(int, newPos);

    lastCursorPos = 0;
    newCursorPos = 0;
    input.simulate(testWidget);
    QCOMPARE(lastCursorPos, lastPos);
    QCOMPARE(newCursorPos, newPos);
}

void tst_QLineEdit::selectedText()
{
    QString testString = "Abc defg hijklmno, p 'qrst' uvw xyz";

    // start with a basic text
    testWidget->setText(testString);
    selection_count = 0;

    // The text we have now is:
    //           1         2         3         4         5
    // 012345678901234567890123456789012345678901234567890
    // Abc defg hijklmno, p 'qrst' uvw xyz

    testWidget->home(false);
    QVERIFY(!testWidget->hasSelectedText());
    QCOMPARE(testWidget->selectedText(), QString());

    // play a bit with the cursorForward, cursorBackward(), etc
    testWidget->cursorForward(true, 9);
    QVERIFY(testWidget->hasSelectedText());
    QCOMPARE(testWidget->selectedText(), QString("Abc defg "));
    QVERIFY(selection_count == 1);

    // reset selection
    testWidget->home(false);
    QVERIFY(!testWidget->hasSelectedText());
    QCOMPARE(testWidget->selectedText(), QString());
    selection_count = 0;
}

/* // tested in selectedText
void tst_QLineEdit::backspace()
void tst_QLineEdit::del()
void tst_QLineEdit::selectionChanged()
void tst_QLineEdit::selectAll()
void tst_QLineEdit::deselect()
*/

void tst_QLineEdit::onSelectionChanged()
{
    selection_count++;
}

void tst_QLineEdit::deleteSelectedText()
{
    const QString text = QString::fromLatin1("bar");
    QLineEdit edit( text );
    QCOMPARE(edit.text(), text);

    edit.selectAll();

    QTest::keyClick(&edit, Qt::Key_Delete, 0);
    QVERIFY(edit.text().isEmpty());

    edit.setText(text);
    edit.selectAll();

    QMenu *menu = edit.createStandardContextMenu();
    for (int i = 0; i < menu->actions().count(); ++i) {
        QAction *current = menu->actions().at(i);
        if (current->text() == QLineEdit::tr("Delete")) {
            current->trigger(); //this will delete the whole text selected
            QVERIFY(edit.text().isEmpty());
        }
    }

}


void tst_QLineEdit::textChangedAndTextEdited()
{
    changed_count = 0;
    edited_count = 0;

    QTest::keyClick(testWidget, Qt::Key_A);
    QCOMPARE(changed_count, 1);
    QVERIFY(edited_count == changed_count);
    QTest::keyClick(testWidget, 'b');
    QCOMPARE(changed_count, 2);
    QVERIFY(edited_count == changed_count);
    QTest::keyClick(testWidget, 'c');
    QCOMPARE(changed_count, 3);
    QVERIFY(edited_count == changed_count);
    QTest::keyClick(testWidget, ' ');
    QCOMPARE(changed_count, 4);
    QVERIFY(edited_count == changed_count);
    QTest::keyClick(testWidget, 'd');
    QCOMPARE(changed_count, 5);
    QVERIFY(edited_count == changed_count);

    changed_count = 0;
    edited_count = 0;
    changed_string = QString::null;

    testWidget->setText("foo");
    QCOMPARE(changed_count, 1);
    QCOMPARE(edited_count, 0);
    QCOMPARE(changed_string, QString("foo"));

    changed_count = 0;
    edited_count = 0;
    changed_string = QString::null;

    testWidget->setText("");
    QCOMPARE(changed_count, 1);
    QCOMPARE(edited_count, 0);
    QVERIFY(changed_string.isEmpty());
    QVERIFY(!changed_string.isNull());
}

void tst_QLineEdit::onTextChanged(const QString &text)
{
    changed_count++;
    changed_string = text;
}

void tst_QLineEdit::onTextEdited(const QString &/*text*/)
{
    edited_count++;
}


void tst_QLineEdit::onCursorPositionChanged(int oldPos, int newPos)
{
    lastCursorPos = oldPos;
    newCursorPos = newPos;
}

void tst_QLineEdit::returnPressed()
{
    return_count = 0;

    QTest::keyClick(testWidget, Qt::Key_Return);
    QVERIFY(return_count == 1);
    return_count = 0;

    QTest::keyClick(testWidget, 'A');
    QVERIFY(return_count == 0);
    QTest::keyClick(testWidget, 'b');
    QVERIFY(return_count == 0);
    QTest::keyClick(testWidget, 'c');
    QVERIFY(return_count == 0);
    QTest::keyClick(testWidget, ' ');
    QVERIFY(return_count == 0);
    QTest::keyClick(testWidget, 'd');
    QVERIFY(return_count == 0);
    psKeyClick(testWidget, Qt::Key_Home);
    QVERIFY(return_count == 0);
    psKeyClick(testWidget, Qt::Key_End);
    QVERIFY(return_count == 0);
    QTest::keyClick(testWidget, Qt::Key_Escape);
    QVERIFY(return_count == 0);
    QTest::keyClick(testWidget, Qt::Key_Return);
    QVERIFY(return_count == 1);
}

// int validator that fixes all !isNumber to '0'
class QIntFixValidator : public QIntValidator {
public:
    QIntFixValidator(int min, int max, QObject *parent) : QIntValidator(min, max, parent) {}
    void fixup (QString &input) const {
        for (int i=0; i<input.length(); ++i)
            if (!input.at(i).isNumber()) {
                input[(int)i] = QChar('0');
            }
    }
};

void tst_QLineEdit::returnPressed_maskvalidator_data() {
    QTest::addColumn<QString>("inputMask");
    QTest::addColumn<bool>("hasValidator");
    QTest::addColumn<QTestEventList>("input");
    QTest::addColumn<QString>("expectedText");
    QTest::addColumn<bool>("returnPressed");

    {
        QTestEventList keys;
        keys.addKeyClick(Qt::Key_Home);
        keys.addKeyClick(Qt::Key_1);
        keys.addKeyClick(Qt::Key_2);
        keys.addKeyClick(Qt::Key_3);
        keys.addKeyClick(Qt::Key_Return);
        QTest::newRow("no mask, no validator, input '123<cr>'")
            << QString()
            << false
            << keys
            << QString("123")
            << true;
    }
    {
        QTestEventList keys;
        keys.addKeyClick(Qt::Key_Home);
        keys.addKeyClick(Qt::Key_1);
        keys.addKeyClick(Qt::Key_2);
        keys.addKeyClick(Qt::Key_Return);
        QTest::newRow("mask '999', no validator, input '12<cr>'")
            << QString("999")
            << false
            << keys
            << QString("12")
            << false;
    }
    {
        QTestEventList keys;
        keys.addKeyClick(Qt::Key_Home);
        keys.addKeyClick(Qt::Key_1);
        keys.addKeyClick(Qt::Key_2);
        keys.addKeyClick(Qt::Key_3);
        keys.addKeyClick(Qt::Key_Return);
        QTest::newRow("mask '999', no validator, input '123<cr>'")
            << QString("999")
            << false
            << keys
            << QString("123")
            << true;
    }
    {
        QTestEventList keys;
        keys.addKeyClick(Qt::Key_Home);
        keys.addKeyClick(Qt::Key_1);
        keys.addKeyClick(Qt::Key_2);
        keys.addKeyClick(Qt::Key_3);
        keys.addKeyClick(Qt::Key_Return);
        QTest::newRow("no mask, intfix validator(0,999), input '123<cr>'")
            << QString()
            << true
            << keys
            << QString("123")
            << true;
    }
    {
        QTestEventList keys;
        keys.addKeyClick(Qt::Key_Home);
        keys.addKeyClick(Qt::Key_7);
        keys.addKeyClick(Qt::Key_7);
        keys.addKeyClick(Qt::Key_7);
        keys.addKeyClick(Qt::Key_7);
        keys.addKeyClick(Qt::Key_Return);
        QTest::newRow("no mask, intfix validator(0,999), input '7777<cr>'")
            << QString()
            << true
            << keys
            << QString("777")
            << true;
    }
    {
        QTestEventList keys;
        keys.addKeyClick(Qt::Key_Home);
        keys.addKeyClick(Qt::Key_1);
        keys.addKeyClick(Qt::Key_2);
        keys.addKeyClick(Qt::Key_Return);
        QTest::newRow("mask '999', intfix validator(0,999), input '12<cr>'")
            << QString("999")
            << true
            << keys
            << QString("12")
            << false;
    }
    {
        QTestEventList keys;
        keys.addKeyClick(Qt::Key_Home);
        keys.addKeyClick(Qt::Key_Return);
        QTest::newRow("mask '999', intfix validator(0,999), input '<cr>'")
            << QString("999")
            << true
            << keys
            << QString("000")
            << true;
    }
}

void tst_QLineEdit::returnPressed_maskvalidator()
{
    QFETCH(QString, inputMask);
    QFETCH(bool, hasValidator);
    QFETCH(QTestEventList, input);
    QFETCH(QString, expectedText);
    QFETCH(bool, returnPressed);

    QEXPECT_FAIL("mask '999', intfix validator(0,999), input '12<cr>'", "QIntValidator has changed behaviour. Does not accept spaces. Task 43082.", Abort);

    testWidget->setInputMask(inputMask);
    if (hasValidator)
        testWidget->setValidator(new QIntFixValidator(0, 999, testWidget));

    return_count = 0;
    input.simulate(testWidget);

    QCOMPARE(testWidget->text(), expectedText);
    QCOMPARE(return_count , returnPressed ? 1 : 0);
}

void tst_QLineEdit::onReturnPressed()
{
    return_count++;
}

void tst_QLineEdit::setValidator()
{
    // Verify that we can set and re-set a validator.
    QVERIFY(!testWidget->validator());

    QIntValidator iv1(0);
    testWidget->setValidator(&iv1);
    QCOMPARE(testWidget->validator(), static_cast<const QValidator*>(&iv1));

    testWidget->setValidator(0);
    QVERIFY(testWidget->validator() == 0);

    QIntValidator iv2(0, 99, 0);
    testWidget->setValidator(&iv2);
    QCOMPARE(testWidget->validator(), static_cast<const QValidator *>(&iv2));

    testWidget->setValidator(0);
    QVERIFY(testWidget->validator() == 0);
}

void tst_QLineEdit::setValidator_QIntValidator_data()
{
    QTest::addColumn<int>("mini");
    QTest::addColumn<int>("maxi");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expectedText");
    QTest::addColumn<bool>("useKeys");
    QTest::addColumn<bool>("is_valid");

    for (int i=0; i<2; i++) {
        bool useKeys = false;
        QString inputMode = "insert ";
        if (i!=0) {
            inputMode = "useKeys ";
            useKeys = true;
        }

        // valid data
        QTest::newRow(QString(inputMode + "range [0,9] valid '1'").toLatin1())
            << 0
            << 9
            << QString("1")
            << QString("1")
            << bool(useKeys)
            << bool(true);

        QTest::newRow(QString(inputMode + "range [3,7] valid '3'").toLatin1())
            << 3
            << 7
            << QString("3")
            << QString("3")
            << bool(useKeys)
            << bool(true);

        QTest::newRow(QString(inputMode + "range [3,7] valid '7'").toLatin1())
            << 3
            << 7
            << QString("7")
            << QString("7")
            << bool(useKeys)
            << bool(true);

        QTest::newRow(QString(inputMode + "range [0,100] valid '9'").toLatin1())
            << 0
            << 100
            << QString("9")
            << QString("9")
            << bool(useKeys)
            << bool(true);

        QTest::newRow(QString(inputMode + "range [0,100] valid '12'").toLatin1())
            << 0
            << 100
            << QString("12")
            << QString("12")
            << bool(useKeys)
            << bool(true);

        QTest::newRow(QString(inputMode + "range [-100,100] valid '-12'").toLatin1())
            << -100
            << 100
            << QString("-12")
            << QString("-12")
            << bool(useKeys)
            << bool(true);

        // invalid data
        // characters not allowed in QIntValidator
        QTest::newRow(QString(inputMode + "range [0,9] inv 'a-a'").toLatin1())
            << 0
            << 9
            << QString("a")
            << QString("")
            << bool(useKeys)
            << bool(false);

        QTest::newRow(QString(inputMode + "range [0,9] inv 'A'").toLatin1())
            << 0
            << 9
            << QString("A")
            << QString("")
            << bool(useKeys)
            << bool(false);
        // minus sign only allowed with a range on the negative side
        QTest::newRow(QString(inputMode + "range [0,100] inv '-'").toLatin1())
            << 0
            << 100
            << QString("-")
            << QString("")
            << bool(useKeys)
            << bool(false);
        QTest::newRow(QString(inputMode + "range [0,100] int '153'").toLatin1())
            << 0
            << 100
            << QString("153")
            << QString(useKeys ? "15" : "")
            << bool(useKeys)
            << bool(useKeys ? true : false);
        QTest::newRow(QString(inputMode + "range [-100,100] int '-153'").toLatin1())
            << -100
            << 100
            << QString("-153")
            << QString(useKeys ? "-15" : "")
            << bool(useKeys)
            << bool(useKeys ? true : false);
        QTest::newRow(QString(inputMode + "range [3,7] int '2'").toLatin1())
            << 3
            << 7
            << QString("2")
            << QString("2")
            << bool(useKeys)
            << bool(false);

        QTest::newRow(QString(inputMode + "range [3,7] int '8'").toLatin1())
            << 3
            << 7
            << QString("8")
            << QString("")
            << bool(useKeys)
            << bool(false);
    }
}

void tst_QLineEdit::setValidator_QIntValidator()
{
    QFETCH(int, mini);
    QFETCH(int, maxi);
    QFETCH(QString, input);
    QFETCH(QString, expectedText);
    QFETCH(bool, useKeys);
    QFETCH(bool, is_valid);

    QIntValidator intValidator(mini, maxi, 0);
    testWidget->setValidator(&intValidator);
    QVERIFY(testWidget->text().isEmpty());
//qDebug("1 input: '" + input + "' Exp: '" + expectedText + "'");

    // tests valid input
    if (!useKeys) {
        testWidget->insert(input);
    } else {
        QTest::keyClicks(testWidget, input);
        return_count = 0;
        QTest::keyClick(testWidget, Qt::Key_Return);
        QCOMPARE(return_count, int(is_valid)); // assuming that is_valid = true equals 1
    }
//qDebug("2 input: '" + input + "' Exp: '" + expectedText + "'");
//    QCOMPARE(testWidget->displayText(), expectedText);
    QCOMPARE(testWidget->text(), expectedText);
}

#define NO_PIXMAP_TESTS

void tst_QLineEdit::frame_data()
{
#ifndef NO_PIXMAP_TESTS
#if defined Q_OS_WIN
    QTest::addColumn<QPixmap>("noFrame");
    QTest::addColumn<QPixmap>("useFrame");

    QTest::newRow("win");
//#else
//    QTest::newRow("x11");
#endif
#endif
}

void tst_QLineEdit::frame()
{
    testWidget->setFrame(false);
    // verify that the editor is shown without a frame
#ifndef NO_PIXMAP_TESTS
#if defined Q_OS_WIN
    QTEST(testWidget, "noFrame");
#endif
#endif
    QVERIFY(!testWidget->hasFrame());

    testWidget->setFrame(true);
    // verify that the editor is shown with a frame
#ifndef NO_PIXMAP_TESTS
#if defined Q_OS_WIN
    QTEST(testWidget, "useFrame");
#endif
#endif
    QVERIFY(testWidget->hasFrame());
}

void tst_QLineEdit::setAlignment_data()
{
#ifndef NO_PIXMAP_TESTS
#if defined Q_OS_WIN
    QTest::addColumn<QPixmap>("left");
    QTest::addColumn<QPixmap>("right");
    QTest::addColumn<QPixmap>("hcenter");
    QTest::addColumn<QPixmap>("auto");

    QTest::newRow("win");
//#else
//    QTest::newRow("x11");
#endif
#endif
}

void tst_QLineEdit::setAlignment()
{
    testWidget->setText("left");
    testWidget->setAlignment(Qt::AlignLeft);
#ifndef NO_PIXMAP_TESTS
#if defined Q_OS_WIN
    QTEST(testWidget, "left");
#endif
#endif
    QVERIFY(testWidget->alignment() == Qt::AlignLeft);

    testWidget->setText("hcenter");
    testWidget->setAlignment(Qt::AlignHCenter);
#ifndef NO_PIXMAP_TESTS
#if defined Q_OS_WIN
    QTEST(testWidget, "hcenter");
#endif
#endif
    QVERIFY(testWidget->alignment() == Qt::AlignHCenter);

    testWidget->setText("right");
    testWidget->setAlignment(Qt::AlignRight);
#ifndef NO_PIXMAP_TESTS
#if defined Q_OS_WIN
    QTEST(testWidget, "right");
#endif
#endif
    QVERIFY(testWidget->alignment() == Qt::AlignRight);

    testWidget->setAlignment(Qt::AlignTop);
    QVERIFY(testWidget->alignment() == Qt::AlignTop);

    testWidget->setAlignment(Qt::AlignBottom);
    QVERIFY(testWidget->alignment() == Qt::AlignBottom);

    testWidget->setAlignment(Qt::AlignCenter);
    QVERIFY(testWidget->alignment() == Qt::AlignCenter);
}

void tst_QLineEdit::isModified()
{
    QVERIFY(!testWidget->isModified());
    testWidget->setText("bla");
    QVERIFY(!testWidget->isModified());

    psKeyClick(testWidget, Qt::Key_Home);
    QVERIFY(!testWidget->isModified());
    QTest::keyClick(testWidget, Qt::Key_Right);
    QVERIFY(!testWidget->isModified());
    QTest::keyClick(testWidget, Qt::Key_Right);
    QVERIFY(!testWidget->isModified());
    QTest::keyClick(testWidget, Qt::Key_Right);
    QVERIFY(!testWidget->isModified());
    QTest::keyClick(testWidget, Qt::Key_Left);
    QVERIFY(!testWidget->isModified());
    psKeyClick(testWidget, Qt::Key_End);
    QVERIFY(!testWidget->isModified());

    QTest::keyClicks(testWidget, "T");
    QVERIFY(testWidget->isModified());
    QTest::keyClicks(testWidget, "his is a string");
    QVERIFY(testWidget->isModified());

    testWidget->setText("");
    QVERIFY(!testWidget->isModified());
    testWidget->setText("foo");
    QVERIFY(!testWidget->isModified());
}

/*
    Obsolete function but as long as we provide it, it needs to work.
*/

void tst_QLineEdit::edited()
{
    QVERIFY(!testWidget->isModified());
    testWidget->setText("bla");
    QVERIFY(!testWidget->isModified());

    psKeyClick(testWidget, Qt::Key_Home);
    QVERIFY(!testWidget->isModified());
    QTest::keyClick(testWidget, Qt::Key_Right);
    QVERIFY(!testWidget->isModified());
    QTest::keyClick(testWidget, Qt::Key_Right);
    QVERIFY(!testWidget->isModified());
    QTest::keyClick(testWidget, Qt::Key_Right);
    QVERIFY(!testWidget->isModified());
    QTest::keyClick(testWidget, Qt::Key_Left);
    QVERIFY(!testWidget->isModified());
    psKeyClick(testWidget, Qt::Key_End);
    QVERIFY(!testWidget->isModified());

    QTest::keyClicks(testWidget, "T");
    QVERIFY(testWidget->isModified());
    QTest::keyClicks(testWidget, "his is a string");
    QVERIFY(testWidget->isModified());

    testWidget->setModified(false);
    QVERIFY(!testWidget->isModified());

    testWidget->setModified(true);
    QVERIFY(testWidget->isModified());
}

void tst_QLineEdit::insert()
{
    testWidget->insert("This");
    testWidget->insert(" is");
    testWidget->insert(" a");
    testWidget->insert(" test");

    QCOMPARE(testWidget->text(), QString("This is a test"));

    testWidget->cursorWordBackward(false);
    testWidget->cursorBackward(false, 1);
    testWidget->insert(" nice");
    QCOMPARE(testWidget->text(), QString("This is a nice test"));

    testWidget->setCursorPosition(-1);
    testWidget->insert("No Crash! ");
    QCOMPARE(testWidget->text(), QString("No Crash! This is a nice test"));
}

void tst_QLineEdit::setSelection_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("start");
    QTest::addColumn<int>("length");
    QTest::addColumn<int>("expectedCursor");
    QTest::addColumn<QString>("expectedText");
    QTest::addColumn<bool>("expectedHasSelectedText");

    QString text = "Abc defg hijklmno, p 'qrst' uvw xyz";
    int start, length, pos;

    start = 0; length = 1; pos = 1;
    QTest::newRow(QString("selection start: %1 length: %2").arg(start).arg(length).toLatin1())
        << text << start << length << pos << QString("A") << true;

    start = 0; length = 2; pos = 2;
    QTest::newRow(QString("selection start: %1 length: %2").arg(start).arg(length).toLatin1())
        << text << start << length << pos << QString("Ab") << true;

    start = 0; length = 4; pos = 4;
    QTest::newRow(QString("selection start: %1 length: %2").arg(start).arg(length).toLatin1())
        << text << start << length << pos << QString("Abc ") << true;

    start = -1; length = 0; pos = text.length();
    QTest::newRow(QString("selection start: %1 length: %2").arg(start).arg(length).toLatin1())
        << text << start << length << pos << QString() << false;

    start = 34; length = 1; pos = 35;
    QTest::newRow(QString("selection start: %1 length: %2").arg(start).arg(length).toLatin1())
        << text << start << length << pos << QString("z") << true;

    start = 34; length = 2; pos = 35;
    QTest::newRow(QString("selection start: %1 length: %2").arg(start).arg(length).toLatin1())
        << text << start << length << pos << QString("z") << true;

    start = 34; length = -1; pos = 33;
    QTest::newRow(QString("selection start: %1 length: %2").arg(start).arg(length).toLatin1())
        << text << start << length << pos << QString("y") << true;

    start = 1; length = -2; pos = 0;
    QTest::newRow(QString("selection start: %1 length: %2").arg(start).arg(length).toLatin1())
        << text << start << length << pos << QString("A") << true;

    start = -1; length = -1; pos = text.length();
    QTest::newRow(QString("selection start: %1 length: %2").arg(start).arg(length).toLatin1())
        << text << start << length << pos << QString() << false;
}


void tst_QLineEdit::setSelection()
{
    QFETCH(QString, text);
    QFETCH(int, start);
    QFETCH(int, length);
    QFETCH(int, expectedCursor);
    QFETCH(QString, expectedText);
    QFETCH(bool, expectedHasSelectedText);

    testWidget->setText(text);
    testWidget->setSelection(start, length);
    QCOMPARE(testWidget->hasSelectedText(), expectedHasSelectedText);
    QCOMPARE(testWidget->selectedText(), expectedText);
    if (expectedCursor >= 0)
        QCOMPARE(testWidget->cursorPosition(), expectedCursor);
}

#ifndef QT_NO_CLIPBOARD
void tst_QLineEdit::cut()
{
    if (!PlatformClipboard::isAvailable())
        QSKIP("Autotests run from cron and pasteboard don't get along quite ATM");

    // test newlines in cut'n'paste
    testWidget->setText("A\nB\nC\n");
    testWidget->setSelection(0, 6);
    testWidget->cut();
    psKeyClick(testWidget, Qt::Key_Home);
    testWidget->paste();
    QCOMPARE(testWidget->text(), QString("A\nB\nC\n"));
    //                              1         2         3         4
    //                    01234567890123456789012345678901234567890
    testWidget->setText("Abc defg hijklmno");

    testWidget->setSelection(0, 3);
    testWidget->cut();
    QCOMPARE(testWidget->text(), QString(" defg hijklmno"));

    psKeyClick(testWidget, Qt::Key_End);
    testWidget->paste();
    QCOMPARE(testWidget->text(), QString(" defg hijklmnoAbc"));

    psKeyClick(testWidget, Qt::Key_Home);
    testWidget->del();
    QCOMPARE(testWidget->text(), QString("defg hijklmnoAbc"));

    testWidget->setSelection(0, 4);
    testWidget->copy();
    psKeyClick(testWidget, Qt::Key_End);
    testWidget->paste();
    QCOMPARE(testWidget->text(), QString("defg hijklmnoAbcdefg"));

    QTest::keyClick(testWidget, Qt::Key_Left);
    QTest::keyClick(testWidget, Qt::Key_Left);
    QTest::keyClick(testWidget, Qt::Key_Left);
    QTest::keyClick(testWidget, Qt::Key_Left);
    QTest::keyClick(testWidget, Qt::Key_Left);
    QTest::keyClick(testWidget, Qt::Key_Left);
    QTest::keyClick(testWidget, Qt::Key_Left);
    QTest::keyClick(testWidget, ' ');
    QCOMPARE(testWidget->text(), QString("defg hijklmno Abcdefg"));

    testWidget->setSelection(0, 5);
    testWidget->del();
    QCOMPARE(testWidget->text(), QString("hijklmno Abcdefg"));

    testWidget->end(false);
    QTest::keyClick(testWidget, ' ');
    testWidget->paste();
    QCOMPARE(testWidget->text(), QString("hijklmno Abcdefg defg"));

    testWidget->home(false);
    testWidget->cursorWordForward(true);
    testWidget->cut();
    testWidget->end(false);
    QTest::keyClick(testWidget, ' ');
    testWidget->paste();
    testWidget->cursorBackward(true, 1);
    testWidget->cut();
    QCOMPARE(testWidget->text(), QString("Abcdefg defg hijklmno"));
}
#endif

class InputMaskValidator : public QValidator
{
public:
    InputMaskValidator(QObject *parent, const char *name = 0) : QValidator(parent) { setObjectName(name); }
    State validate(QString &text, int &pos) const
    {
        InputMaskValidator *that = (InputMaskValidator *)this;
        that->validateText = text;
        that->validatePos = pos;
        return Acceptable;
    }
    QString validateText;
    int validatePos;
};

void tst_QLineEdit::inputMaskAndValidator_data()
{
    QTest::addColumn<QString>("inputMask");
    QTest::addColumn<QTestEventList>("keys");
    QTest::addColumn<QString>("validateText");
    QTest::addColumn<int>("validatePos");

    QTestEventList inputKeys;
    inputKeys.addKeyClick(Qt::Key_1);
    inputKeys.addKeyClick(Qt::Key_2);

    QTest::newRow("task28291") << "000;_" << inputKeys << "12_" << 2;
}

void tst_QLineEdit::inputMaskAndValidator()
{
    QFETCH(QString, inputMask);
    QFETCH(QTestEventList, keys);
    QFETCH(QString, validateText);
    QFETCH(int, validatePos);

    InputMaskValidator imv(testWidget);
    testWidget->setValidator(&imv);

    testWidget->setInputMask(inputMask);
    keys.simulate(testWidget);

    QCOMPARE(imv.validateText, validateText);
    QCOMPARE(imv.validatePos, validatePos);
}

void tst_QLineEdit::maxLengthAndInputMask()
{
    // Really a test for #30447
    QVERIFY(testWidget->inputMask().isNull());
    testWidget->setMaxLength(10);
    QVERIFY(testWidget->maxLength() == 10);
    testWidget->setInputMask(QString::null);
    QVERIFY(testWidget->inputMask().isNull());
    QVERIFY(testWidget->maxLength() == 10);
}


class LineEdit : public QLineEdit
{
public:
    LineEdit() { state = Other; }

    void keyPressEvent(QKeyEvent *e)
    {
        QLineEdit::keyPressEvent(e);
        if (e->key() == Qt::Key_Enter) {
            state = e->isAccepted() ? Accepted : Ignored;
        } else {
            state = Other;
        }

    }
    enum State {
        Accepted,
        Ignored,
        Other
    };

    State state;

    friend class tst_QLineEdit;
};

Q_DECLARE_METATYPE(LineEdit::State);
void tst_QLineEdit::returnPressedKeyEvent()
{
    LineEdit lineedit;
    lineedit.show();
    QCOMPARE((int)lineedit.state, (int)LineEdit::Other);
    QTest::keyClick(&lineedit, Qt::Key_Enter);
    QCOMPARE((int)lineedit.state, (int)LineEdit::Ignored);
    connect(&lineedit, SIGNAL(returnPressed()), this, SLOT(onReturnPressed()));
    QTest::keyClick(&lineedit, Qt::Key_Enter);
    QCOMPARE((int)lineedit.state, (int)LineEdit::Ignored);
    disconnect(&lineedit, SIGNAL(returnPressed()), this, SLOT(onReturnPressed()));
    QTest::keyClick(&lineedit, Qt::Key_Enter);
    QCOMPARE((int)lineedit.state, (int)LineEdit::Ignored);
    QTest::keyClick(&lineedit, Qt::Key_1);
    QCOMPARE((int)lineedit.state, (int)LineEdit::Other);
}

void tst_QLineEdit::keepSelectionOnTabFocusIn()
{
    testWidget->setText("hello world");
    {
        QFocusEvent e(QEvent::FocusIn, Qt::TabFocusReason);
        QApplication::sendEvent(testWidget, &e);
    }
    QCOMPARE(testWidget->selectedText(), QString("hello world"));
    testWidget->setSelection(0, 5);
    QCOMPARE(testWidget->selectedText(), QString("hello"));
    {
        QFocusEvent e(QEvent::FocusIn, Qt::TabFocusReason);
        QApplication::sendEvent(testWidget, &e);
    }
    QCOMPARE(testWidget->selectedText(), QString("hello"));
}

void tst_QLineEdit::readOnlyStyleOption()
{
    bool wasReadOnly = testWidget->isReadOnly();
    QStyle *oldStyle = testWidget->style();

    StyleOptionTestStyle myStyle;
    testWidget->setStyle(&myStyle);

    myStyle.setReadOnly(true);
    testWidget->setReadOnly(true);
    testWidget->repaint();
    qApp->processEvents();

    testWidget->setReadOnly(false);
    myStyle.setReadOnly(false);
    testWidget->repaint();
    qApp->processEvents();

    testWidget->setReadOnly(wasReadOnly);
    testWidget->setStyle(oldStyle);
}

void tst_QLineEdit::validateOnFocusOut()
{
    QSignalSpy editingFinishedSpy(testWidget, SIGNAL(editingFinished()));
    testWidget->setValidator(new QIntValidator(100, 999, 0));
    QTest::keyPress(testWidget, '1');
    QTest::keyPress(testWidget, '0');
    QCOMPARE(testWidget->text(), QString("10"));
    testWidget->clearFocus();
    QCOMPARE(editingFinishedSpy.count(), 0);

    testWidget->setFocus();
    testWidget->activateWindow();
    QTRY_VERIFY(testWidget->hasFocus());

    QTest::keyPress(testWidget, '0');
    QTRY_COMPARE(testWidget->text(), QString("100"));

    testWidget->clearFocus();
    QCOMPARE(editingFinishedSpy.count(), 1);
}

void tst_QLineEdit::editInvalidText()
{
    testWidget->clear();
    testWidget->setValidator(new QIntValidator(0, 120, 0));
    testWidget->setText("1234");

    QVERIFY(!testWidget->hasAcceptableInput());
    QTest::keyPress(testWidget, Qt::Key_Backspace);
    QTest::keyPress(testWidget, Qt::Key_Backspace);
    QTest::keyPress(testWidget, Qt::Key_A);
    QTest::keyPress(testWidget, Qt::Key_B);
    QTest::keyPress(testWidget, Qt::Key_C);
    QTest::keyPress(testWidget, Qt::Key_1);
    QVERIFY(testWidget->hasAcceptableInput());
    QCOMPARE(testWidget->text(), QString("12"));
    testWidget->cursorBackward(false);
    testWidget->cursorBackward(true, 2);
    QTest::keyPress(testWidget, Qt::Key_Delete);
    QVERIFY(testWidget->hasAcceptableInput());
    QCOMPARE(testWidget->text(), QString("2"));
    QTest::keyPress(testWidget, Qt::Key_1);
    QVERIFY(testWidget->hasAcceptableInput());
    QCOMPARE(testWidget->text(), QString("12"));

    testWidget->setValidator(0);
}

void tst_QLineEdit::charWithAltOrCtrlModifier()
{
    testWidget->clear();
    QCOMPARE(testWidget->text(), QString(""));
    QTest::keyPress(testWidget, Qt::Key_Plus);
    QCOMPARE(testWidget->text(), QString("+"));
    QTest::keyPress(testWidget, Qt::Key_Plus, Qt::ControlModifier);
    QCOMPARE(testWidget->text(), QString("++"));
    QTest::keyPress(testWidget, Qt::Key_Plus, Qt::AltModifier);
    QCOMPARE(testWidget->text(), QString("+++"));
    QTest::keyPress(testWidget, Qt::Key_Plus, Qt::AltModifier | Qt::ControlModifier);
    QCOMPARE(testWidget->text(), QString("++++"));
}

void tst_QLineEdit::leftKeyOnSelectedText()
{
    testWidget->clear();
    testWidget->setText("0123");
    testWidget->setCursorPosition(4);
    QTest::keyClick(testWidget, Qt::Key_Left, Qt::ShiftModifier);
    QCOMPARE(testWidget->cursorPosition(), 3);
    QCOMPARE(testWidget->selectedText(), QString("3"));
    QTest::keyClick(testWidget, Qt::Key_Left, Qt::ShiftModifier);
    QCOMPARE(testWidget->cursorPosition(), 2);
    QCOMPARE(testWidget->selectedText(), QString("23"));
    QTest::keyClick(testWidget, Qt::Key_Left);
#ifdef Q_OS_WIN
    QCOMPARE(testWidget->cursorPosition(), 1);
#else
    // Selection is cleared ands cursor remains at position 2.
    // X11 used to behave like window prior to 4.2. Changes caused by QKeySequence
    // resulted in an inadvertant change in behavior
    QCOMPARE(testWidget->cursorPosition(), 2);
#endif
}

void tst_QLineEdit::inlineCompletion()
{
    testWidget->clear();
    QStandardItemModel *model = new QStandardItemModel;
    QStandardItem *root = model->invisibleRootItem();
    QStandardItem *items[5];
    for (int i = 0; i < 5; i++) {
        items[i] = new QStandardItem(QString("item%1").arg(i));
        if ((i+2)%2 == 0) { // disable 0,2,4
            items[i]->setFlags(items[i]->flags() & ~Qt::ItemIsEnabled);
        }
        root->appendRow(items[i]);
    }
    QCompleter *completer = new QCompleter(model);
    completer->setCompletionMode(QCompleter::InlineCompletion);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    testWidget->setFocus();
    QTRY_COMPARE(qApp->activeWindow(), (QWidget*)testWidget);
    testWidget->setCompleter(completer);

    // sanity
    QTest::keyClick(testWidget, Qt::Key_X);
    QCOMPARE(testWidget->selectedText(), QString());
    QCOMPARE(testWidget->text(), QString("x"));
    QTest::keyClick(testWidget, Qt::Key_Down, Qt::ControlModifier);
    QCOMPARE(testWidget->selectedText(), QString());
    QCOMPARE(testWidget->text(), QString("x"));
    QTest::keyClick(testWidget, Qt::Key_Up, Qt::ControlModifier);
    QCOMPARE(testWidget->selectedText(), QString());
    QCOMPARE(testWidget->text(), QString("x"));

    testWidget->clear();
    QTest::keyClick(testWidget, Qt::Key_I);
    QCOMPARE(testWidget->selectedText(), QString("tem1"));

    Qt::KeyboardModifiers keyboardModifiers = Qt::ControlModifier;
#ifdef Q_OS_MAC
    keyboardModifiers |= Qt::AltModifier;
#endif
    QTest::keyClick(testWidget, Qt::Key_Down, keyboardModifiers);
    QCOMPARE(testWidget->selectedText(), QString("tem3"));

    // wraps around (Default)
    QTest::keyClick(testWidget, Qt::Key_Down, keyboardModifiers);
    QCOMPARE(testWidget->selectedText(), QString("tem1"));

    QTest::keyClick(testWidget, Qt::Key_Up, keyboardModifiers);
    QCOMPARE(testWidget->selectedText(), QString("tem3"));

    // should not wrap
    completer->setWrapAround(false);
    QTest::keyClick(testWidget, Qt::Key_Down, keyboardModifiers);
    QCOMPARE(testWidget->selectedText(), QString("tem3"));
    QTest::keyClick(testWidget, Qt::Key_Up, keyboardModifiers); // item1
    QTest::keyClick(testWidget, Qt::Key_Up, keyboardModifiers); // item1
    QCOMPARE(testWidget->selectedText(), QString("tem1"));

    // trivia :)
    root->appendRow(new QStandardItem("item11"));
    root->appendRow(new QStandardItem("item12"));
    testWidget->clear();
    QTest::keyClick(testWidget, Qt::Key_I);
    QCOMPARE(testWidget->selectedText(), QString("tem1"));
    QTest::keyClick(testWidget, Qt::Key_Delete);
    QCOMPARE(testWidget->selectedText(), QString());
    QTest::keyClick(testWidget, Qt::Key_Down, keyboardModifiers);
    QCOMPARE(testWidget->selectedText(), QString("tem1")); // neato
    testWidget->setText("item1");
    testWidget->setSelection(1, 2);
    QTest::keyClick(testWidget, Qt::Key_Down, keyboardModifiers);
    testWidget->end(false);
    QCOMPARE(testWidget->text(), QString("item1")); // no effect for selection in "middle"
    QTest::keyClick(testWidget, Qt::Key_Down, keyboardModifiers); // item1
    QTest::keyClick(testWidget, Qt::Key_Down, keyboardModifiers); // item11
    QCOMPARE(testWidget->text(), QString("item11"));

    delete model;
    delete completer;
}

void tst_QLineEdit::noTextEditedOnClear()
{
    testWidget->setText("Test");
    QSignalSpy textEditedSpy(testWidget, SIGNAL(textEdited(QString)));
    testWidget->clear();
    QCOMPARE(textEditedSpy.count(), 0);
}

void tst_QLineEdit::textMargin_data()
{
    QTest::addColumn<int>("left");
    QTest::addColumn<int>("top");
    QTest::addColumn<int>("right");
    QTest::addColumn<int>("bottom");

    QTest::addColumn<QPoint>("mousePressPos");
    QTest::addColumn<int>("cursorPosition");

    QLineEdit testWidget;
    QFontMetrics metrics(testWidget.font());
    const QString s = QLatin1String("MMM MMM MMM");

    // Different styles generate different offsets, so
    // calculate the width rather than hardcode it.
    const int pixelWidthOfM = metrics.width(s, 1);
    const int pixelWidthOfMMM_MM = metrics.width(s, 6);

    QTest::newRow("default-0") << 0 << 0 << 0 << 0 << QPoint(pixelWidthOfMMM_MM, 0) << 6;
    QTest::newRow("default-1") << 0 << 0 << 0 << 0 << QPoint(1, 1) << 0;
    QTest::newRow("default-2") << -1 << 0 << -1 << 0 << QPoint(pixelWidthOfMMM_MM, 0) << 6;
    QTest::newRow("default-3") << 0 << 0 << 0 << 0 << QPoint(pixelWidthOfM, 1) << 1;

    QTest::newRow("hor-0") << 10 << 0 << 10 << 0 << QPoint(1, 1) << 0;
    QTest::newRow("hor-1") << 10 << 0 << 10 << 0 << QPoint(10, 1) << 0;
    QTest::newRow("hor-2") << 20 << 0 << 10 << 0 << QPoint(20, 1) << 0;

    if (!qApp->style()->inherits("QMacStyle")) { //MacStyle doesn't support verticals margins.
        QTest::newRow("default-2-ver") << -1 << -1 << -1 << -1 << QPoint(pixelWidthOfMMM_MM, 0) << 6;
        QTest::newRow("ver") << 0 << 10 << 0 << 10 << QPoint(1, 1) << 0;
    }
}

void tst_QLineEdit::textMargin()
{
    QFETCH(int, left);
    QFETCH(int, top);
    QFETCH(int, right);
    QFETCH(int, bottom);
    QFETCH(QPoint, mousePressPos);
    QFETCH(int, cursorPosition);

    // Put the line edit into a toplevel window to avoid
    // resizing by the window system.
    QWidget tlw;
    QLineEdit testWidget(&tlw);
    testWidget.setGeometry(100, 100, 100, 30);
    testWidget.setText("MMM MMM MMM");
    testWidget.setCursorPosition(6);

    QSize sizeHint = testWidget.sizeHint();
    testWidget.setTextMargins(left, top, right, bottom);
    sizeHint.setWidth(sizeHint.width() + left + right);
    sizeHint.setHeight(sizeHint.height() + top +bottom);
    QCOMPARE(testWidget.sizeHint(), sizeHint);
    testWidget.setFrame(false);
    tlw.show();

    int l;
    int t;
    int r;
    int b;
    testWidget.getTextMargins(&l, &t, &r, &b);
    QCOMPARE(left, l);
    QCOMPARE(top, t);
    QCOMPARE(right, r);
    QCOMPARE(bottom, b);

    QTest::mouseClick(&testWidget, Qt::LeftButton, 0, mousePressPos);
    QTRY_COMPARE(testWidget.cursorPosition(), cursorPosition);
}

#ifndef QTEST_NO_CURSOR
void tst_QLineEdit::cursor()
{
    testWidget->setReadOnly(false);
    QCOMPARE(testWidget->cursor().shape(), Qt::IBeamCursor);
    testWidget->setReadOnly(true);
    QCOMPARE(testWidget->cursor().shape(), Qt::ArrowCursor);
    testWidget->setReadOnly(false);
    QCOMPARE(testWidget->cursor().shape(), Qt::IBeamCursor);
}
#endif

class task180999_Widget : public QWidget
{
public:
    task180999_Widget(QWidget *parent = 0) : QWidget(parent)
    {
        QHBoxLayout *layout  = new QHBoxLayout(this);
        lineEdit1.setText("some text 1 ...");
        lineEdit2.setText("some text 2 ...");
        layout->addWidget(&lineEdit1);
        layout->addWidget(&lineEdit2);
    }

    QLineEdit lineEdit1;
    QLineEdit lineEdit2;
};

void tst_QLineEdit::task180999_focus()
{
    task180999_Widget widget;

    widget.lineEdit1.setFocus();
    widget.show();

    widget.lineEdit2.setFocus();
    widget.lineEdit2.selectAll();
    widget.hide();

    widget.lineEdit1.setFocus();
    widget.show();
    QTest::qWait(200);
    widget.activateWindow();

    QTRY_VERIFY(!widget.lineEdit2.hasSelectedText());
}

void tst_QLineEdit::task174640_editingFinished()
{
    QWidget mw;
    QVBoxLayout *layout = new QVBoxLayout(&mw);
    QLineEdit *le1 = new QLineEdit(&mw);
    QLineEdit *le2 = new QLineEdit(&mw);
    layout->addWidget(le1);
    layout->addWidget(le2);

    mw.show();
    QApplication::setActiveWindow(&mw);
    mw.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&mw));
    QCOMPARE(&mw, QApplication::activeWindow());

    QSignalSpy editingFinishedSpy(le1, SIGNAL(editingFinished()));

    le1->setFocus();
    QTest::qWait(20);
    QTRY_VERIFY(le1->hasFocus());
    QCOMPARE(editingFinishedSpy.count(), 0);

    le2->setFocus();
    QTest::qWait(20);
    QTRY_VERIFY(le2->hasFocus());
    QCOMPARE(editingFinishedSpy.count(), 1);
    editingFinishedSpy.clear();

    le1->setFocus();
    QTest::qWait(20);
    QTRY_VERIFY(le1->hasFocus());

    QMenu *testMenu1 = new QMenu(le1);
    testMenu1->addAction("foo");
    testMenu1->addAction("bar");
    testMenu1->show();
    QVERIFY(QTest::qWaitForWindowExposed(testMenu1));
    QTest::qWait(20);
    mw.activateWindow();

    delete testMenu1;
    QCOMPARE(editingFinishedSpy.count(), 0);
    QTRY_VERIFY(le1->hasFocus());

    QMenu *testMenu2 = new QMenu(le2);
    testMenu2->addAction("foo2");
    testMenu2->addAction("bar2");
    testMenu2->show();
    QVERIFY(QTest::qWaitForWindowExposed(testMenu2));
    QTest::qWait(20);
    mw.activateWindow();
    delete testMenu2;
    QCOMPARE(editingFinishedSpy.count(), 1);
}

#ifndef QT_NO_COMPLETER
class task198789_Widget : public QWidget
{
    Q_OBJECT
public:
    task198789_Widget(QWidget *parent = 0) : QWidget(parent)
    {
        QStringList wordList;
        wordList << "alpha" << "omega" << "omicron" << "zeta";

        lineEdit = new QLineEdit(this);
        completer = new QCompleter(wordList, this);
        lineEdit->setCompleter(completer);

        connect(lineEdit, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));
    }

    QLineEdit *lineEdit;
    QCompleter *completer;
    QString currentCompletion;

private slots:
    void textChanged(const QString &)
    {
        currentCompletion = completer->currentCompletion();
    }
};

void tst_QLineEdit::task198789_currentCompletion()
{
    task198789_Widget widget;
    widget.show();
    qApp->processEvents();
    QTest::keyPress(widget.lineEdit, 'o');
    QTest::keyPress(widget.lineEdit, 'm');
    QTest::keyPress(widget.lineEdit, 'i');
    QCOMPARE(widget.currentCompletion, QLatin1String("omicron"));
}

void tst_QLineEdit::task210502_caseInsensitiveInlineCompletion()
{
    QString completion("ABCD");
    QStringList completions;
    completions << completion;
    QLineEdit lineEdit;
    QCompleter completer(completions);
    completer.setCaseSensitivity(Qt::CaseInsensitive);
    completer.setCompletionMode(QCompleter::InlineCompletion);
    lineEdit.setCompleter(&completer);
    lineEdit.show();
    QApplication::setActiveWindow(&lineEdit);
    QVERIFY(QTest::qWaitForWindowActive(&lineEdit));
    lineEdit.setFocus();
    QTRY_VERIFY(lineEdit.hasFocus());
    QTest::keyPress(&lineEdit, 'a');
    QTest::keyPress(&lineEdit, Qt::Key_Return);
    QCOMPARE(lineEdit.text(), completion);
}

#endif // QT_NO_COMPLETER


void tst_QLineEdit::task229938_dontEmitChangedWhenTextIsNotChanged()
{
    QLineEdit lineEdit;
    lineEdit.setMaxLength(5);
    lineEdit.show();
    QTest::qWaitForWindowExposed(&lineEdit); // to be safe and avoid failing setFocus with window managers
    lineEdit.setFocus();
    QSignalSpy changedSpy(&lineEdit, SIGNAL(textChanged(QString)));
    QTest::qWait(200);
    QTest::keyPress(&lineEdit, 'a');
    QTest::keyPress(&lineEdit, 'b');
    QTest::keyPress(&lineEdit, 'c');
    QTest::keyPress(&lineEdit, 'd');
    QTest::keyPress(&lineEdit, 'e');
    QTest::keyPress(&lineEdit, 'f');
    QCOMPARE(changedSpy.count(), 5);
}

void tst_QLineEdit::task233101_cursorPosAfterInputMethod_data()
{
    QTest::addColumn<int>("maxLength");
    QTest::addColumn<int>("cursorPos");
    QTest::addColumn<int>("replacementStart");
    QTest::addColumn<int>("replacementLength");
    QTest::addColumn<QString>("commitString");

    QTest::newRow("data1")  << 4 << 4 << 0 << 0 << QString("");
    QTest::newRow("data2")  << 4 << 4 << 0 << 0 << QString("x");
    QTest::newRow("data3")  << 4 << 4 << 0 << 0 << QString("xxxxxxxxxxxxxxxx");
    QTest::newRow("data4")  << 4 << 3 << 0 << 0 << QString("");
    QTest::newRow("data5")  << 4 << 3 << 0 << 0 << QString("x");
    QTest::newRow("data6")  << 4 << 3 << 0 << 0 << QString("xxxxxxxxxxxxxxxx");
    QTest::newRow("data7")  << 4 << 0 << 0 << 0 << QString("");
    QTest::newRow("data8")  << 4 << 0 << 0 << 0 << QString("x");
    QTest::newRow("data9")  << 4 << 0 << 0 << 0 << QString("xxxxxxxxxxxxxxxx");

    QTest::newRow("data10") << 4 << 4 << -4 << 4 << QString("");
    QTest::newRow("data11") << 4 << 4 << -4 << 4 << QString("x");
    QTest::newRow("data12") << 4 << 4 << -4 << 4 << QString("xxxxxxxxxxxxxxxx");
    QTest::newRow("data13") << 4 << 3 << -3 << 4 << QString("");
    QTest::newRow("data14") << 4 << 3 << -3 << 4 << QString("x");
    QTest::newRow("data15") << 4 << 3 << -3 << 4 << QString("xxxxxxxxxxxxxxxx");
    QTest::newRow("data16") << 4 << 0 << 0 << 4 << QString("");
    QTest::newRow("data17") << 4 << 0 << 0 << 4 << QString("x");
    QTest::newRow("data18") << 4 << 0 << 0 << 4 << QString("xxxxxxxxxxxxxxxx");

    QTest::newRow("data19") << 4 << 4 << -4 << 0 << QString("");
    QTest::newRow("data20") << 4 << 4 << -4 << 0 << QString("x");
    QTest::newRow("data21") << 4 << 4 << -4 << 0 << QString("xxxxxxxxxxxxxxxx");
    QTest::newRow("data22") << 4 << 3 << -3 << 0 << QString("");
    QTest::newRow("data23") << 4 << 3 << -3 << 0 << QString("x");
    QTest::newRow("data24") << 4 << 3 << -3 << 0 << QString("xxxxxxxxxxxxxxxx");
}

void tst_QLineEdit::task233101_cursorPosAfterInputMethod()
{
    QFETCH(int, maxLength);
    QFETCH(int, cursorPos);
    QFETCH(int, replacementStart);
    QFETCH(int, replacementLength);
    QFETCH(QString, commitString);

    QLineEdit lineEdit;
    lineEdit.setMaxLength(maxLength);
    lineEdit.insert(QString().fill(QLatin1Char('a'), cursorPos));
    QCOMPARE(lineEdit.cursorPosition(), cursorPos);

    QInputMethodEvent event;
    event.setCommitString(QLatin1String("x"), replacementStart, replacementLength);
    qApp->sendEvent(&lineEdit, &event);
    QVERIFY(lineEdit.cursorPosition() >= 0);
    QVERIFY(lineEdit.cursorPosition() <= lineEdit.text().size());
    QVERIFY(lineEdit.text().size() <= lineEdit.maxLength());
}

void tst_QLineEdit::task241436_passwordEchoOnEditRestoreEchoMode()
{
    QStyleOptionFrameV2 opt;
    QChar fillChar = testWidget->style()->styleHint(QStyle::SH_LineEdit_PasswordCharacter, &opt, testWidget);

    testWidget->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    testWidget->setFocus();
    QApplication::setActiveWindow(testWidget);
    QTRY_VERIFY(testWidget->hasFocus());

    QTest::keyPress(testWidget, '0');
    QCOMPARE(testWidget->displayText(), QString("0"));
    testWidget->setEchoMode(QLineEdit::Normal);
    testWidget->clearFocus();
    QCOMPARE(testWidget->displayText(), QString("0"));

    testWidget->activateWindow();
    testWidget->setFocus();
    testWidget->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    QTest::keyPress(testWidget, '0');
    QCOMPARE(testWidget->displayText(), QString("0"));
    testWidget->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    QCOMPARE(testWidget->displayText(), QString("0"));
    testWidget->clearFocus();
    QCOMPARE(testWidget->displayText(), QString(fillChar));

    // restore clean state
    testWidget->setEchoMode(QLineEdit::Normal);
}

void tst_QLineEdit::task248948_redoRemovedSelection()
{
    testWidget->setText("a");
    testWidget->selectAll();
    QTest::keyPress(testWidget, Qt::Key_Delete);
    testWidget->undo();
    testWidget->redo();
    QTest::keyPress(testWidget, 'a');
    QTest::keyPress(testWidget, 'b');
    QCOMPARE(testWidget->text(), QLatin1String("ab"));
}

void tst_QLineEdit::taskQTBUG_4401_enterKeyClearsPassword()
{
    QString password("Wanna guess?");

    testWidget->setText(password);
    testWidget->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    testWidget->setFocus();
    testWidget->selectAll();
    QApplication::setActiveWindow(testWidget);
    QTRY_VERIFY(testWidget->hasFocus());

    QTest::keyPress(testWidget, Qt::Key_Enter);
    QTRY_COMPARE(testWidget->text(), password);
}

void tst_QLineEdit::taskQTBUG_4679_moveToStartEndOfBlock()
{
#ifdef Q_OS_MAC
    const QString text("there are no blocks for lineEdit");
    testWidget->setText(text);
    testWidget->setCursorPosition(5);
    QCOMPARE(testWidget->cursorPosition(), 5);
    testWidget->setFocus();
    QTest::keyPress(testWidget, Qt::Key_A, Qt::MetaModifier);
    QCOMPARE(testWidget->cursorPosition(), 0);
    QTest::keyPress(testWidget, Qt::Key_E, Qt::MetaModifier);
    QCOMPARE(testWidget->cursorPosition(), text.size());
#endif // Q_OS_MAC
}

void tst_QLineEdit::taskQTBUG_4679_selectToStartEndOfBlock()
{
#ifdef Q_OS_MAC
    const QString text("there are no blocks for lineEdit, select all");
    testWidget->setText(text);
    testWidget->setCursorPosition(5);
    QCOMPARE(testWidget->cursorPosition(), 5);
    testWidget->setFocus();
    QTest::keyPress(testWidget, Qt::Key_A, Qt::MetaModifier | Qt::ShiftModifier);
    QCOMPARE(testWidget->cursorPosition(), 0);
    QVERIFY(testWidget->hasSelectedText());
    QCOMPARE(testWidget->selectedText(), text.mid(0, 5));

    QTest::keyPress(testWidget, Qt::Key_E, Qt::MetaModifier | Qt::ShiftModifier);
    QCOMPARE(testWidget->cursorPosition(), text.size());
    QVERIFY(testWidget->hasSelectedText());
    QCOMPARE(testWidget->selectedText(), text.mid(5));
#endif // Q_OS_MAC
}

#ifndef QT_NO_CONTEXTMENU
void tst_QLineEdit::taskQTBUG_7902_contextMenuCrash()
{
    // Would pass before the associated commit, but left as a guard.
    QLineEdit *w = new QLineEdit;
    w->show();
    QVERIFY(QTest::qWaitForWindowExposed(w));

    QTimer ti;
    w->connect(&ti, SIGNAL(timeout()), w, SLOT(deleteLater()));
    ti.start(200);

    QContextMenuEvent *cme = new QContextMenuEvent(QContextMenuEvent::Mouse, w->rect().center());
    qApp->postEvent(w, cme);

    QTest::qWait(300);
    // No crash, it's allright.
}
#endif

void tst_QLineEdit::taskQTBUG_7395_readOnlyShortcut()
{
    //ReadOnly QLineEdit should not intercept shortcut.
    QLineEdit le;
    le.setReadOnly(true);

    QAction action(QString::fromLatin1("hello"), &le);
    action.setShortcut(QString::fromLatin1("p"));
    QSignalSpy spy(&action, SIGNAL(triggered()));
    le.addAction(&action);

    le.show();
    QVERIFY(QTest::qWaitForWindowExposed(&le));
    QApplication::setActiveWindow(&le);
    QVERIFY(QTest::qWaitForWindowActive(&le));
    le.setFocus();
    QTRY_VERIFY(le.hasFocus());

    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_P);
    QCOMPARE(spy.count(), 1);
}

void tst_QLineEdit::QTBUG697_paletteCurrentColorGroup()
{
    QLineEdit le;
    le.setText("               ");
    QPalette p = le.palette();
    p.setBrush(QPalette::Active, QPalette::Highlight, Qt::green);
    p.setBrush(QPalette::Inactive, QPalette::Highlight, Qt::red);
    le.setPalette(p);

    le.show();
    QApplication::setActiveWindow(&le);
    QVERIFY(QTest::qWaitForWindowActive(&le));
    le.setFocus();
    QTRY_VERIFY(le.hasFocus());
    le.selectAll();

    QImage img(le.size(),QImage::Format_ARGB32 );
    le.render(&img);
    QCOMPARE(img.pixel(10, le.height()/2), QColor(Qt::green).rgb());

    QWindow window;
    window.resize(100, 50);
    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    le.render(&img);
    QCOMPARE(img.pixel(10, le.height()/2), QColor(Qt::red).rgb());
}

void tst_QLineEdit::QTBUG13520_textNotVisible()
{
    LineEdit le;
    le.setAlignment( Qt::AlignRight | Qt::AlignVCenter);
    le.show();
    QVERIFY(QTest::qWaitForWindowExposed(&le));
    le.setText("01-ST16-01SIL-MPL001wfgsdfgsdgsdfgsdfgsdfgsdfgsdfg");
    le.setCursorPosition(0);
    QTest::qWait(100); //just make sure we get he lineedit correcly painted

    QVERIFY(le.cursorRect().center().x() < le.width() / 2);


}

class UpdateRegionLineEdit : public QLineEdit
{
public:
    QRegion updateRegion;
protected:
    void paintEvent(QPaintEvent *event)
    {
        updateRegion = event->region();
    }
};

void tst_QLineEdit::QTBUG7174_inputMaskCursorBlink()
{
    UpdateRegionLineEdit edit;
    edit.setInputMask(QLatin1String("AAAA"));
    edit.setFocus();
    edit.setText(QLatin1String("AAAA"));
    edit.show();
    QRect cursorRect = edit.inputMethodQuery(Qt::ImMicroFocus).toRect();
    QVERIFY(QTest::qWaitForWindowExposed(&edit));
    edit.updateRegion = QRegion();
    QTest::qWait(QApplication::cursorFlashTime());
    QVERIFY(edit.updateRegion.contains(cursorRect));
}

void tst_QLineEdit::QTBUG16850_setSelection()
{
    QLineEdit le;
    le.setInputMask("00:0");
    le.setText("  1");
    le.setSelection(3, 1);
    QCOMPARE(le.selectionStart(), 3);
    QCOMPARE(le.selectedText(), QString("1"));
}

void tst_QLineEdit::bidiVisualMovement_data()
{
    QTest::addColumn<QString>("logical");
    QTest::addColumn<int>("basicDir");
    QTest::addColumn<IntList>("positionList");

    QTest::newRow("Latin text")
        << QString::fromUtf8("abc")
        << (int) QChar::DirL
        << (IntList() << 0 << 1 << 2 << 3);
    QTest::newRow("Hebrew text, one item")
        << QString::fromUtf8("\327\220\327\221\327\222")
        << (int) QChar::DirR
        << (QList<int>() << 0 << 1 << 2 << 3);
    QTest::newRow("Hebrew text after Latin text")
        << QString::fromUtf8("abc\327\220\327\221\327\222")
        << (int) QChar::DirL
        << (QList<int>() << 0 << 1 << 2 << 6 << 5 << 4 << 3);
    QTest::newRow("Latin text after Hebrew text")
        << QString::fromUtf8("\327\220\327\221\327\222abc")
        << (int) QChar::DirR
        << (QList<int>() << 0 << 1 << 2 << 6 << 5 << 4 << 3);
    QTest::newRow("LTR, 3 items")
        << QString::fromUtf8("abc\327\220\327\221\327\222abc")
        << (int) QChar::DirL
        << (QList<int>() << 0 << 1 << 2 << 5 << 4 << 3 << 6 << 7 << 8 << 9);
    QTest::newRow("RTL, 3 items")
        << QString::fromUtf8("\327\220\327\221\327\222abc\327\220\327\221\327\222")
        << (int) QChar::DirR
        << (QList<int>() << 0 << 1 << 2 << 5 << 4 << 3 << 6 << 7 << 8 << 9);
    QTest::newRow("LTR, 4 items")
        << QString::fromUtf8("abc\327\220\327\221\327\222abc\327\220\327\221\327\222")
        << (int) QChar::DirL
        << (QList<int>() << 0 << 1 << 2 << 5 << 4 << 3 << 6 << 7 << 8 << 12 << 11 << 10 << 9);
    QTest::newRow("RTL, 4 items")
        << QString::fromUtf8("\327\220\327\221\327\222abc\327\220\327\221\327\222abc")
        << (int) QChar::DirR
        << (QList<int>() << 0 << 1 << 2 << 5 << 4 << 3 << 6 << 7 << 8 << 12 << 11 << 10 << 9);
}

void tst_QLineEdit::bidiVisualMovement()
{
    QFETCH(QString, logical);
    QFETCH(int,     basicDir);
    QFETCH(IntList, positionList);

    QLineEdit le;
    le.setText(logical);

    le.setCursorMoveStyle(Qt::VisualMoveStyle);
    le.setCursorPosition(0);

    bool moved;
    int i = 0, oldPos, newPos = 0;

    do {
        oldPos = newPos;
        QVERIFY(oldPos == positionList[i]);
        if (basicDir == QChar::DirL) {
            QTest::keyClick(&le, Qt::Key_Right);
        } else
            QTest::keyClick(&le, Qt::Key_Left);
        newPos = le.cursorPosition();
        moved = (oldPos != newPos);
        i++;
    } while (moved);

    QVERIFY(i == positionList.size());

    do {
        i--;
        oldPos = newPos;
        QVERIFY(oldPos == positionList[i]);
        if (basicDir == QChar::DirL) {
            QTest::keyClick(&le, Qt::Key_Left);
        } else
        {
            QTest::keyClick(&le, Qt::Key_Right);
        }
        newPos = le.cursorPosition();
        moved = (oldPos != newPos);
    } while (moved && i >= 0);
}

void tst_QLineEdit::bidiLogicalMovement_data()
{
    bidiVisualMovement_data();
}

void tst_QLineEdit::bidiLogicalMovement()
{
    QFETCH(QString, logical);
    QFETCH(int,     basicDir);

    QLineEdit le;
    le.setText(logical);

    le.setCursorMoveStyle(Qt::LogicalMoveStyle);
    le.setCursorPosition(0);

    bool moved;
    int i = 0, oldPos, newPos = 0;

    do {
        oldPos = newPos;
        QVERIFY(oldPos == i);
        if (basicDir == QChar::DirL) {
            QTest::keyClick(&le, Qt::Key_Right);
        } else
            QTest::keyClick(&le, Qt::Key_Left);
        newPos = le.cursorPosition();
        moved = (oldPos != newPos);
        i++;
    } while (moved);

    do {
        i--;
        oldPos = newPos;
        QVERIFY(oldPos == i);
        if (basicDir == QChar::DirL) {
            QTest::keyClick(&le, Qt::Key_Left);
        } else
        {
            QTest::keyClick(&le, Qt::Key_Right);
        }
        newPos = le.cursorPosition();
        moved = (oldPos != newPos);
    } while (moved && i >= 0);
}

void tst_QLineEdit::selectAndCursorPosition()
{
    testWidget->setText("This is a long piece of text");

    testWidget->setSelection(0, 5);
    QCOMPARE(testWidget->cursorPosition(), 5);
    testWidget->setSelection(5, -5);
    QCOMPARE(testWidget->cursorPosition(), 0);
}

void tst_QLineEdit::inputMethod()
{
    // widget accepts input
    QInputMethodQueryEvent queryEvent(Qt::ImEnabled);
    QApplication::sendEvent(testWidget, &queryEvent);
    QCOMPARE(queryEvent.value(Qt::ImEnabled).toBool(), true);

    testWidget->setEnabled(false);
    QApplication::sendEvent(testWidget, &queryEvent);
    QCOMPARE(queryEvent.value(Qt::ImEnabled).toBool(), false);
    testWidget->setEnabled(true);

    // removing focus allows input method to commit preedit
    testWidget->setText("");
    testWidget->activateWindow();
    // TODO setFocus should not be necessary here, because activateWindow
    // should focus it, and the window is the QLineEdit. But the test can fail
    // on Windows if we don't do this. If each test had a unique QLineEdit
    // instance, maybe such problems would go away.
    testWidget->setFocus();
    QTRY_VERIFY(testWidget->hasFocus());
    QTRY_COMPARE(qApp->focusObject(), testWidget);

    m_platformInputContext.setCommitString("text");
    m_platformInputContext.m_commitCallCount = 0;
    QList<QInputMethodEvent::Attribute> attributes;
    QInputMethodEvent preeditEvent("preedit text", attributes);
    QApplication::sendEvent(testWidget, &preeditEvent);

    testWidget->clearFocus();
    QCOMPARE(m_platformInputContext.m_commitCallCount, 1);
    QCOMPARE(testWidget->text(), QString("text"));
}

void tst_QLineEdit::inputMethodSelection()
{
    testWidget->setText("Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
    testWidget->setSelection(0,0);
    QSignalSpy selectionSpy(testWidget, SIGNAL(selectionChanged()));

    QCOMPARE(selectionSpy.count(), 0);
    QCOMPARE(testWidget->selectionStart(), -1);

    testWidget->setSelection(0,5);

    QCOMPARE(selectionSpy.count(), 1);
    QCOMPARE(testWidget->selectionStart(), 0);

    // selection gained
    {
        QList<QInputMethodEvent::Attribute> attributes;
        attributes << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, 12, 5, QVariant());
        QInputMethodEvent event("", attributes);
        QApplication::sendEvent(testWidget, &event);
    }

    QCOMPARE(selectionSpy.count(), 2);
    QCOMPARE(testWidget->selectionStart(), 12);

    // selection removed
    {
        QList<QInputMethodEvent::Attribute> attributes;
        attributes << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, 0, 0, QVariant());
        QInputMethodEvent event("", attributes);
        QApplication::sendEvent(testWidget, &event);
    }

    QCOMPARE(selectionSpy.count(), 3);
}

Q_DECLARE_METATYPE(Qt::InputMethodHints)
void tst_QLineEdit::inputMethodQueryImHints_data()
{
    QTest::addColumn<Qt::InputMethodHints>("hints");

    QTest::newRow("None") << static_cast<Qt::InputMethodHints>(Qt::ImhNone);
    QTest::newRow("Password") << static_cast<Qt::InputMethodHints>(Qt::ImhHiddenText);
    QTest::newRow("Normal") << static_cast<Qt::InputMethodHints>(Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText | Qt::ImhSensitiveData);
}

void tst_QLineEdit::inputMethodQueryImHints()
{
    QFETCH(Qt::InputMethodHints, hints);
    testWidget->setInputMethodHints(hints);

    QVariant value = testWidget->inputMethodQuery(Qt::ImHints);
    QCOMPARE(static_cast<Qt::InputMethodHints>(value.toInt()), hints);
}

void tst_QLineEdit::undoRedoAndEchoModes_data()
{
    QTest::addColumn<int>("echoMode");
    QTest::addColumn<QStringList>("input");
    QTest::addColumn<QStringList>("expected");

    QStringList input(QList<QString>() << "aaa" << "bbb" << "ccc");

    QTest::newRow("Normal")
        << (int) QLineEdit::Normal
        << input
        << QStringList(QList<QString>() << "aaa" << "ccc" << "");

    QTest::newRow("NoEcho")
        << (int) QLineEdit::NoEcho
        << input
        << QStringList(QList<QString>() << "" << "" << "");

    QTest::newRow("Password")
        << (int) QLineEdit::Password
        << input
        << QStringList(QList<QString>() << "" << "" << "");

    QTest::newRow("PasswordEchoOnEdit")
        << (int) QLineEdit::PasswordEchoOnEdit
        << input
        << QStringList(QList<QString>() << "" << "" << "");
}

void tst_QLineEdit::undoRedoAndEchoModes()
{
    QFETCH(int, echoMode);
    QFETCH(QStringList, input);
    QFETCH(QStringList, expected);

    // create some history for the QLineEdit
    testWidget->clear();
    testWidget->setEchoMode(QLineEdit::EchoMode(echoMode));
    testWidget->insert(input.at(0));
    testWidget->selectAll();
    testWidget->backspace();
    testWidget->insert(input.at(1));

    // test undo
    QVERIFY(testWidget->isUndoAvailable());
    testWidget->undo();
    QCOMPARE(testWidget->text(), expected.at(0));
    testWidget->insert(input.at(2));
    testWidget->selectAll();
    testWidget->backspace();
    QCOMPARE(testWidget->isUndoAvailable(), echoMode == QLineEdit::Normal);
    testWidget->undo();
    QCOMPARE(testWidget->text(), expected.at(1));

    // test redo
    QCOMPARE(testWidget->isRedoAvailable(), echoMode == QLineEdit::Normal);
    testWidget->redo();
    QCOMPARE(testWidget->text(), expected.at(2));
    QVERIFY(!testWidget->isRedoAvailable());
    testWidget->redo();
    QCOMPARE(testWidget->text(), expected.at(2));
}

QTEST_MAIN(tst_QLineEdit)
#include "tst_qlineedit.moc"
