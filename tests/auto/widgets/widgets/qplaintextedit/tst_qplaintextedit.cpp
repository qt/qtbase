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


#include <qtextedit.h>
#include <qtextcursor.h>
#include <qtextlist.h>
#include <qdebug.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qtextbrowser.h>
#include <private/qwidgettextcontrol_p.h>
#include <qscrollbar.h>
#include <qtextobject.h>

#include <qabstracttextdocumentlayout.h>
#include <qtextdocumentfragment.h>

#include "qplaintextedit.h"
#include "../../../shared/platformclipboard.h"

#include "../../../qtest-config.h"

//Used in copyAvailable
typedef QPair<Qt::Key, Qt::KeyboardModifier> keyPairType;
typedef QList<keyPairType> pairListType;
Q_DECLARE_METATYPE(keyPairType);

QT_FORWARD_DECLARE_CLASS(QPlainTextEdit)

class tst_QPlainTextEdit : public QObject
{
    Q_OBJECT
public:
    tst_QPlainTextEdit();

public slots:
    void initTestCase();
    void init();
    void cleanup();
private slots:
    void getSetCheck();
#ifndef QT_NO_CLIPBOARD
    void clearMustNotChangeClipboard();
#endif
    void clearMustNotResetRootFrameMarginToDefault();
    void paragSeparatorOnPlaintextAppend();
#ifndef QT_NO_CLIPBOARD
    void selectAllSetsNotSelection();
#endif
    void asciiTab();
    void setDocument();
    void emptyAppend();
    void appendOnEmptyDocumentShouldReuseInitialParagraph();
    void cursorPositionChanged();
    void setTextCursor();
#ifndef QT_NO_CLIPBOARD
    void undoAvailableAfterPaste();
#endif
    void undoRedoAvailableRepetition();
    void appendShouldNotTouchTheSelection();
    void backspace();
    void shiftBackspace();
    void undoRedo();
    void preserveCharFormatInAppend();
#ifndef QT_NO_CLIPBOARD
    void copyAndSelectAllInReadonly();
#endif
    void ctrlAltInput();
    void noPropertiesOnDefaultTextEditCharFormat();
    void setPlainTextShouldEmitTextChangedOnce();
    void overwriteMode();
    void shiftDownInLineLastShouldSelectToEnd_data();
    void shiftDownInLineLastShouldSelectToEnd();
    void undoRedoShouldRepositionTextEditCursor();
    void lineWrapModes();
#ifndef QTEST_NO_CURSOR
    void mouseCursorShape();
#endif
    void implicitClear();
    void undoRedoAfterSetContent();
    void numPadKeyNavigation();
    void moveCursor();
#ifndef QT_NO_CLIPBOARD
    void mimeDataReimplementations();
#endif
    void shiftEnterShouldInsertLineSeparator();
    void selectWordsFromStringsContainingSeparators_data();
    void selectWordsFromStringsContainingSeparators();
#ifndef QT_NO_CLIPBOARD
    void canPaste();
    void copyAvailable_data();
    void copyAvailable();
#endif
    void ensureCursorVisibleOnInitialShow();
    void setTextInsideResizeEvent();
    void colorfulAppend();
    void ensureVisibleWithRtl();
    void preserveCharFormatAfterSetPlainText();
    void extraSelections();
    void adjustScrollbars();
    void textObscuredByScrollbars();
    void setTextPreservesUndoRedoEnabled();
    void wordWrapProperty();
    void lineWrapProperty();
    void selectionChanged();
    void blockCountChanged();
    void insertAndScrollToBottom();
    void inputMethodQueryImHints_data();
    void inputMethodQueryImHints();

private:
    void createSelection();
    int blockCount() const;
    int lineCount() const;

    QPlainTextEdit *ed;
    qreal rootFrameMargin;
};

// Testing get/set functions
void tst_QPlainTextEdit::getSetCheck()
{
    QPlainTextEdit obj1;
    // QTextDocument * QPlainTextEdit::document()
    // void QPlainTextEdit::setDocument(QTextDocument *)
    QTextDocument *var1 = new QTextDocument;
    var1->setDocumentLayout(new QPlainTextDocumentLayout(var1));
    obj1.setDocument(var1);
    QCOMPARE(var1, obj1.document());
    obj1.setDocument((QTextDocument *)0);
    QVERIFY(var1 != obj1.document()); // QPlainTextEdit creates a new document when setting 0
    QVERIFY((QTextDocument *)0 != obj1.document());
    delete var1;


    // bool QPlainTextEdit::tabChangesFocus()
    // void QPlainTextEdit::setTabChangesFocus(bool)
    obj1.setTabChangesFocus(false);
    QCOMPARE(false, obj1.tabChangesFocus());
    obj1.setTabChangesFocus(true);
    QCOMPARE(true, obj1.tabChangesFocus());

    // LineWrapMode QPlainTextEdit::lineWrapMode()
    // void QPlainTextEdit::setLineWrapMode(LineWrapMode)
    obj1.setLineWrapMode(QPlainTextEdit::LineWrapMode(QPlainTextEdit::NoWrap));
    QCOMPARE(QPlainTextEdit::LineWrapMode(QPlainTextEdit::NoWrap), obj1.lineWrapMode());
    obj1.setLineWrapMode(QPlainTextEdit::LineWrapMode(QPlainTextEdit::WidgetWidth));
    QCOMPARE(QPlainTextEdit::LineWrapMode(QPlainTextEdit::WidgetWidth), obj1.lineWrapMode());
//     obj1.setLineWrapMode(QPlainTextEdit::LineWrapMode(QPlainTextEdit::FixedPixelWidth));
//     QCOMPARE(QPlainTextEdit::LineWrapMode(QPlainTextEdit::FixedPixelWidth), obj1.lineWrapMode());
//     obj1.setLineWrapMode(QPlainTextEdit::LineWrapMode(QPlainTextEdit::FixedColumnWidth));
//     QCOMPARE(QPlainTextEdit::LineWrapMode(QPlainTextEdit::FixedColumnWidth), obj1.lineWrapMode());


    // bool QPlainTextEdit::overwriteMode()
    // void QPlainTextEdit::setOverwriteMode(bool)
    obj1.setOverwriteMode(false);
    QCOMPARE(false, obj1.overwriteMode());
    obj1.setOverwriteMode(true);
    QCOMPARE(true, obj1.overwriteMode());

    // int QPlainTextEdit::tabStopWidth()
    // void QPlainTextEdit::setTabStopWidth(int)
    obj1.setTabStopWidth(0);
    QCOMPARE(0, obj1.tabStopWidth());
    obj1.setTabStopWidth(INT_MIN);
    QCOMPARE(0, obj1.tabStopWidth()); // Makes no sense to set a negative tabstop value
#if defined(Q_OS_WINCE)
    // due to rounding error in qRound when qreal==float
    // we cannot use INT_MAX for this check
    obj1.setTabStopWidth(SHRT_MAX*2);
    QCOMPARE(SHRT_MAX*2, obj1.tabStopWidth());
#else
    obj1.setTabStopWidth(INT_MAX);
    QCOMPARE(INT_MAX, obj1.tabStopWidth());
#endif

}

class QtTestDocumentLayout : public QAbstractTextDocumentLayout
{
    Q_OBJECT
public:
    inline QtTestDocumentLayout(QPlainTextEdit *edit, QTextDocument *doc, int &itCount)
        : QAbstractTextDocumentLayout(doc), useBiggerSize(false), ed(edit), iterationCounter(itCount) {}

    virtual void draw(QPainter *, const QAbstractTextDocumentLayout::PaintContext &)  {}

    virtual int hitTest(const QPointF &, Qt::HitTestAccuracy ) const { return 0; }

    virtual void documentChanged(int, int, int) {}

    virtual int pageCount() const { return 1; }

    virtual QSizeF documentSize() const { return usedSize; }

    virtual QRectF frameBoundingRect(QTextFrame *) const { return QRectF(); }
    virtual QRectF blockBoundingRect(const QTextBlock &) const { return QRectF(); }

    bool useBiggerSize;
    QSize usedSize;

    QPlainTextEdit *ed;

    int &iterationCounter;
};

tst_QPlainTextEdit::tst_QPlainTextEdit()
{}

void tst_QPlainTextEdit::initTestCase()
{
#ifdef Q_OS_WINCE //disable magic for WindowsCE
    qApp->setAutoMaximizeThreshold(-1);
#endif
}

void tst_QPlainTextEdit::init()
{
    ed = new QPlainTextEdit(0);
    rootFrameMargin = ed->document()->documentMargin();
}

void tst_QPlainTextEdit::cleanup()
{
    delete ed;
    ed = 0;
}


void tst_QPlainTextEdit::createSelection()
{
    QTest::keyClicks(ed, "Hello World");
    /* go to start */
#ifndef Q_OS_MAC
    QTest::keyClick(ed, Qt::Key_Home, Qt::ControlModifier);
#else
    QTest::keyClick(ed, Qt::Key_Home);
#endif
    QCOMPARE(ed->textCursor().position(), 0);
    /* select until end of text */
#ifndef Q_OS_MAC
    QTest::keyClick(ed, Qt::Key_End, Qt::ControlModifier | Qt::ShiftModifier);
#else
    QTest::keyClick(ed, Qt::Key_End, Qt::ShiftModifier);
#endif
    QCOMPARE(ed->textCursor().position(), 11);
}
#ifndef QT_NO_CLIPBOARD
void tst_QPlainTextEdit::clearMustNotChangeClipboard()
{
    if (!PlatformClipboard::isAvailable())
        QSKIP("Clipboard not working with cron-started unit tests");
    ed->textCursor().insertText("Hello World");
    QString txt("This is different text");
    QApplication::clipboard()->setText(txt);
    ed->clear();
    QCOMPARE(QApplication::clipboard()->text(), txt);
}
#endif

void tst_QPlainTextEdit::clearMustNotResetRootFrameMarginToDefault()
{
    QCOMPARE(ed->document()->rootFrame()->frameFormat().margin(), rootFrameMargin);
    ed->clear();
    QCOMPARE(ed->document()->rootFrame()->frameFormat().margin(), rootFrameMargin);
}


void tst_QPlainTextEdit::paragSeparatorOnPlaintextAppend()
{
    ed->appendPlainText("Hello\nWorld");
    int cnt = 0;
    QTextBlock blk = ed->document()->begin();
    while (blk.isValid()) {
        ++cnt;
        blk = blk.next();
    }
    QCOMPARE(cnt, 2);
}

#ifndef QT_NO_CLIPBOARD
void tst_QPlainTextEdit::selectAllSetsNotSelection()
{
    if (!QApplication::clipboard()->supportsSelection())
        QSKIP("Test only relevant for systems with selection");

    QApplication::clipboard()->setText(QString("foobar"), QClipboard::Selection);
    QVERIFY(QApplication::clipboard()->text(QClipboard::Selection) == QString("foobar"));

    ed->insertPlainText("Hello World");
    ed->selectAll();

    QCOMPARE(QApplication::clipboard()->text(QClipboard::Selection), QString::fromLatin1("foobar"));
}
#endif

void tst_QPlainTextEdit::asciiTab()
{
    QPlainTextEdit edit;
    edit.setPlainText("\t");
    edit.show();
    qApp->processEvents();
    QCOMPARE(edit.toPlainText().at(0), QChar('\t'));
}

void tst_QPlainTextEdit::setDocument()
{
    QTextDocument *document = new QTextDocument(ed);
    document->setDocumentLayout(new QPlainTextDocumentLayout(document));
    QTextCursor(document).insertText("Test");
    ed->setDocument(document);
    QCOMPARE(ed->toPlainText(), QString("Test"));
}


int tst_QPlainTextEdit::blockCount() const
{
    int blocks = 0;
    for (QTextBlock block = ed->document()->begin(); block.isValid(); block = block.next())
        ++blocks;
    return blocks;
}

int tst_QPlainTextEdit::lineCount() const
{
    int lines = 0;
    for (QTextBlock block = ed->document()->begin(); block.isValid(); block = block.next()) {
        ed->document()->documentLayout()->blockBoundingRect(block);
        lines += block.layout()->lineCount();
    }
    return lines;
}

// Supporter issue #56783
void tst_QPlainTextEdit::emptyAppend()
{
    ed->appendPlainText("Blah");
    QCOMPARE(blockCount(), 1);
    ed->appendPlainText(QString::null);
    QCOMPARE(blockCount(), 2);
    ed->appendPlainText(QString("   "));
    QCOMPARE(blockCount(), 3);
}

void tst_QPlainTextEdit::appendOnEmptyDocumentShouldReuseInitialParagraph()
{
    QCOMPARE(blockCount(), 1);
    ed->appendPlainText("Blah");
    QCOMPARE(blockCount(), 1);
}


class CursorPositionChangedRecorder : public QObject
{
    Q_OBJECT
public:
    inline CursorPositionChangedRecorder(QPlainTextEdit *ed)
        : editor(ed)
    {
        connect(editor, SIGNAL(cursorPositionChanged()), this, SLOT(recordCursorPos()));
    }

    QList<int> cursorPositions;

private slots:
    void recordCursorPos()
    {
        cursorPositions.append(editor->textCursor().position());
    }

private:
    QPlainTextEdit *editor;
};

void tst_QPlainTextEdit::cursorPositionChanged()
{
    QSignalSpy spy(ed, SIGNAL(cursorPositionChanged()));

    spy.clear();
    QTest::keyClick(ed, Qt::Key_A);
    QCOMPARE(spy.count(), 1);

    QTextCursor cursor = ed->textCursor();
    cursor.movePosition(QTextCursor::Start);
    ed->setTextCursor(cursor);
    cursor.movePosition(QTextCursor::End);
    spy.clear();
    cursor.insertText("Test");
    QCOMPARE(spy.count(), 0);

    cursor.movePosition(QTextCursor::End);
    ed->setTextCursor(cursor);
    cursor.movePosition(QTextCursor::Start);
    spy.clear();
    cursor.insertText("Test");
    QCOMPARE(spy.count(), 1);

    spy.clear();
    QTest::keyClick(ed, Qt::Key_Left);
    QCOMPARE(spy.count(), 1);

    CursorPositionChangedRecorder spy2(ed);
    QVERIFY(ed->textCursor().position() > 0);
    ed->setPlainText("Hello World");
    QCOMPARE(spy2.cursorPositions.count(), 1);
    QCOMPARE(spy2.cursorPositions.at(0), 0);
    QCOMPARE(ed->textCursor().position(), 0);
}

void tst_QPlainTextEdit::setTextCursor()
{
    QSignalSpy spy(ed, SIGNAL(cursorPositionChanged()));

    ed->setPlainText("Test");
    QTextCursor cursor = ed->textCursor();
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter);

    spy.clear();

    ed->setTextCursor(cursor);
    QCOMPARE(spy.count(), 1);
}

#ifndef QT_NO_CLIPBOARD
void tst_QPlainTextEdit::undoAvailableAfterPaste()
{
    if (!PlatformClipboard::isAvailable())
        QSKIP("Clipboard not working with cron-started unit tests");

    QSignalSpy spy(ed->document(), SIGNAL(undoAvailable(bool)));

    const QString txt("Test");
    QApplication::clipboard()->setText(txt);
    ed->paste();
    QVERIFY(spy.count() >= 1);
    QCOMPARE(ed->toPlainText(), txt);
}
#endif

class UndoRedoRecorder : public QObject
{
    Q_OBJECT
public:
    UndoRedoRecorder(QTextDocument *doc)
        : undoRepetitions(false)
        , redoRepetitions(false)
        , undoCount(0)
        , redoCount(0)
    {
        connect(doc, SIGNAL(undoAvailable(bool)), this, SLOT(undoAvailable(bool)));
        connect(doc, SIGNAL(redoAvailable(bool)), this, SLOT(redoAvailable(bool)));
    }

    bool undoRepetitions;
    bool redoRepetitions;

private slots:
    void undoAvailable(bool enabled) {
        if (undoCount > 0 && enabled == lastUndoEnabled)
            undoRepetitions = true;

        ++undoCount;
        lastUndoEnabled = enabled;
    }

    void redoAvailable(bool enabled) {
        if (redoCount > 0 && enabled == lastRedoEnabled)
            redoRepetitions = true;

        ++redoCount;
        lastRedoEnabled = enabled;
    }

private:
    bool lastUndoEnabled;
    bool lastRedoEnabled;

    int undoCount;
    int redoCount;
};

void tst_QPlainTextEdit::undoRedoAvailableRepetition()
{
    UndoRedoRecorder spy(ed->document());

    ed->textCursor().insertText("ABC\n\nDEF\n\nGHI\n");
    ed->textCursor().insertText("foo\n");
    ed->textCursor().insertText("bar\n");
    ed->undo(); ed->undo(); ed->undo();
    ed->redo(); ed->redo(); ed->redo();

    QVERIFY(!spy.undoRepetitions);
    QVERIFY(!spy.redoRepetitions);
}

void tst_QPlainTextEdit::appendShouldNotTouchTheSelection()
{
    QTextCursor cursor(ed->document());
    QTextCharFormat fmt;
    fmt.setForeground(Qt::blue);
    cursor.insertText("H", fmt);
    fmt.setForeground(Qt::red);
    cursor.insertText("ey", fmt);

    cursor.insertText("some random text inbetween");

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    QCOMPARE(cursor.charFormat().foreground().color(), QColor(Qt::blue));
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    QCOMPARE(cursor.charFormat().foreground().color(), QColor(Qt::red));
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    QCOMPARE(cursor.charFormat().foreground().color(), QColor(Qt::red));
    QCOMPARE(cursor.selectedText(), QString("Hey"));

    ed->setTextCursor(cursor);
    QVERIFY(ed->textCursor().hasSelection());

    ed->appendHtml("<b>Some Bold Text</b>");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter);
    QCOMPARE(cursor.charFormat().foreground().color(), QColor(Qt::blue));
}

void tst_QPlainTextEdit::backspace()
{
    QTextCursor cursor = ed->textCursor();

    QTextListFormat listFmt;
    listFmt.setStyle(QTextListFormat::ListDisc);
    listFmt.setIndent(1);
    cursor.insertList(listFmt);
    cursor.insertText("A");

    ed->setTextCursor(cursor);

    // delete 'A'
    QTest::keyClick(ed, Qt::Key_Backspace);
    QVERIFY(ed->textCursor().currentList());
    // delete list
    QTest::keyClick(ed, Qt::Key_Backspace);
    QVERIFY(!ed->textCursor().currentList());
    QCOMPARE(ed->textCursor().blockFormat().indent(), 1);
    // outdent paragraph
    QTest::keyClick(ed, Qt::Key_Backspace);
    QCOMPARE(ed->textCursor().blockFormat().indent(), 0);
}

void tst_QPlainTextEdit::shiftBackspace()
{
    QTextCursor cursor = ed->textCursor();

    QTextListFormat listFmt;
    listFmt.setStyle(QTextListFormat::ListDisc);
    listFmt.setIndent(1);
    cursor.insertList(listFmt);
    cursor.insertText("A");

    ed->setTextCursor(cursor);

    // delete 'A'
    QTest::keyClick(ed, Qt::Key_Backspace, Qt::ShiftModifier);
    QVERIFY(ed->textCursor().currentList());
    // delete list
    QTest::keyClick(ed, Qt::Key_Backspace, Qt::ShiftModifier);
    QVERIFY(!ed->textCursor().currentList());
    QCOMPARE(ed->textCursor().blockFormat().indent(), 1);
    // outdent paragraph
    QTest::keyClick(ed, Qt::Key_Backspace, Qt::ShiftModifier);
    QCOMPARE(ed->textCursor().blockFormat().indent(), 0);
}

void tst_QPlainTextEdit::undoRedo()
{
    ed->clear();
    QTest::keyClicks(ed, "abc d");
    QCOMPARE(ed->toPlainText(), QString("abc d"));
    ed->undo();
    QCOMPARE(ed->toPlainText(), QString());
    ed->redo();
    QCOMPARE(ed->toPlainText(), QString("abc d"));
#ifdef Q_OS_WIN
    // shortcut for undo
    QTest::keyClick(ed, Qt::Key_Backspace, Qt::AltModifier);
    QCOMPARE(ed->toPlainText(), QString());
    // shortcut for redo
    QTest::keyClick(ed, Qt::Key_Backspace, Qt::ShiftModifier|Qt::AltModifier);
    QCOMPARE(ed->toPlainText(), QString("abc d"));
#endif
}

// Task #70465
void tst_QPlainTextEdit::preserveCharFormatInAppend()
{
    ed->appendHtml("First para");
    ed->appendHtml("<b>Second para</b>");
    ed->appendHtml("third para");

    QTextCursor cursor(ed->textCursor());

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter);
    QCOMPARE(cursor.charFormat().fontWeight(), (int)QFont::Normal);
    QCOMPARE(cursor.block().text(), QString("First para"));

    cursor.movePosition(QTextCursor::NextBlock);
    cursor.movePosition(QTextCursor::NextCharacter);
    QCOMPARE(cursor.charFormat().fontWeight(), (int)QFont::Bold);
    QCOMPARE(cursor.block().text(), QString("Second para"));

    cursor.movePosition(QTextCursor::NextBlock);
    cursor.movePosition(QTextCursor::NextCharacter);
    QCOMPARE(cursor.charFormat().fontWeight(), (int)QFont::Normal);
    QCOMPARE(cursor.block().text(), QString("third para"));
}

#ifndef QT_NO_CLIPBOARD
void tst_QPlainTextEdit::copyAndSelectAllInReadonly()
{
    if (!PlatformClipboard::isAvailable())
        QSKIP("Clipboard not working with cron-started unit tests");

    ed->setReadOnly(true);
    ed->setPlainText("Hello World");

    QTextCursor cursor = ed->textCursor();
    cursor.clearSelection();
    ed->setTextCursor(cursor);
    QVERIFY(!ed->textCursor().hasSelection());

    QCOMPARE(ed->toPlainText(), QString("Hello World"));

    // shouldn't do anything
    QTest::keyClick(ed, Qt::Key_A);

    QCOMPARE(ed->toPlainText(), QString("Hello World"));

    QTest::keyClick(ed, Qt::Key_A, Qt::ControlModifier);

    QVERIFY(ed->textCursor().hasSelection());

    QApplication::clipboard()->setText(QString());
    QVERIFY(QApplication::clipboard()->text().isEmpty());

    QTest::keyClick(ed, Qt::Key_C, Qt::ControlModifier);
    QCOMPARE(QApplication::clipboard()->text(), QString("Hello World"));
}
#endif

void tst_QPlainTextEdit::ctrlAltInput()
{
    QTest::keyClick(ed, Qt::Key_At, Qt::ControlModifier | Qt::AltModifier);
    QCOMPARE(ed->toPlainText(), QString("@"));
}

void tst_QPlainTextEdit::noPropertiesOnDefaultTextEditCharFormat()
{
    // there should be no properties set on the default/initial char format
    // on a text edit. Font properties instead should be taken from the
    // widget's font (in sync with defaultFont property in document) and the
    // foreground color should be taken from the palette.
    QCOMPARE(ed->textCursor().charFormat().properties().count(), 0);
}

void tst_QPlainTextEdit::setPlainTextShouldEmitTextChangedOnce()
{
    QSignalSpy spy(ed, SIGNAL(textChanged()));
    ed->setPlainText("Yankee Doodle");
    QCOMPARE(spy.count(), 1);
    ed->setPlainText("");
    QCOMPARE(spy.count(), 2);
}

void tst_QPlainTextEdit::overwriteMode()
{
    QVERIFY(!ed->overwriteMode());
    QTest::keyClicks(ed, "Some first text");

    QCOMPARE(ed->toPlainText(), QString("Some first text"));

    ed->setOverwriteMode(true);

    QTextCursor cursor = ed->textCursor();
    cursor.setPosition(5);
    ed->setTextCursor(cursor);

    QTest::keyClicks(ed, "shiny");
    QCOMPARE(ed->toPlainText(), QString("Some shiny text"));

    cursor.movePosition(QTextCursor::End);
    ed->setTextCursor(cursor);

    QTest::keyClick(ed, Qt::Key_Enter);

    ed->setOverwriteMode(false);
    QTest::keyClicks(ed, "Second paragraph");

    QCOMPARE(blockCount(), 2);

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::EndOfBlock);

    QCOMPARE(cursor.position(), 15);
    ed->setTextCursor(cursor);

    ed->setOverwriteMode(true);

    QTest::keyClicks(ed, " blah");

    QCOMPARE(blockCount(), 2);

    QTextBlock block = ed->document()->begin();
    QCOMPARE(block.text(), QString("Some shiny text blah"));
    block = block.next();
    QCOMPARE(block.text(), QString("Second paragraph"));
}

void tst_QPlainTextEdit::shiftDownInLineLastShouldSelectToEnd_data()
{
    // shift cursor-down in the last line should select to the end of the document

    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("totalLineCount");

    QTest::newRow("1") << QString("Foo\nBar") << 2;
    QTest::newRow("2") << QString("Foo\nBar") + QChar(QChar::LineSeparator) + QString("Baz") << 3;
}

void tst_QPlainTextEdit::shiftDownInLineLastShouldSelectToEnd()
{
    QFETCH(QString, input);
    QFETCH(int, totalLineCount);

    ed->setPlainText(input);
    ed->show();

    // ensure we're layouted
    for (QTextBlock block = ed->document()->begin(); block.isValid(); block = block.next())
        ed->document()->documentLayout()->blockBoundingRect(block);

    QCOMPARE(blockCount(), 2);

    int lineCount = 0;
    for (QTextBlock block = ed->document()->begin(); block.isValid(); block = block.next())
        lineCount += block.layout()->lineCount();
    QCOMPARE(lineCount, totalLineCount);

    QTextCursor cursor = ed->textCursor();
    cursor.movePosition(QTextCursor::Start);
    ed->setTextCursor(cursor);

    for (int i = 0; i < lineCount; ++i) {
        QTest::keyClick(ed, Qt::Key_Down, Qt::ShiftModifier);
    }

    input.replace(QLatin1Char('\n'), QChar(QChar::ParagraphSeparator));
    QCOMPARE(ed->textCursor().selectedText(), input);
    QVERIFY(ed->textCursor().atEnd());

    // also test that without shift modifier the cursor does not move to the end
    // for Key_Down in the last line
    cursor.movePosition(QTextCursor::Start);
    ed->setTextCursor(cursor);
    for (int i = 0; i < lineCount; ++i) {
        QTest::keyClick(ed, Qt::Key_Down);
    }
    QVERIFY(!ed->textCursor().atEnd());
}

void tst_QPlainTextEdit::undoRedoShouldRepositionTextEditCursor()
{
    ed->setPlainText("five\nlines\nin\nthis\ntextedit");
    QTextCursor cursor = ed->textCursor();
    cursor.movePosition(QTextCursor::Start);

    ed->setUndoRedoEnabled(false);
    ed->setUndoRedoEnabled(true);

    QVERIFY(!ed->document()->isUndoAvailable());
    QVERIFY(!ed->document()->isRedoAvailable());

    cursor.insertText("Blah");

    QVERIFY(ed->document()->isUndoAvailable());
    QVERIFY(!ed->document()->isRedoAvailable());

    cursor.movePosition(QTextCursor::End);
    ed->setTextCursor(cursor);

    QVERIFY(QMetaObject::invokeMethod(ed, "undo"));

    QVERIFY(!ed->document()->isUndoAvailable());
    QVERIFY(ed->document()->isRedoAvailable());

    QCOMPARE(ed->textCursor().position(), 0);

    cursor.movePosition(QTextCursor::End);
    ed->setTextCursor(cursor);

    QVERIFY(QMetaObject::invokeMethod(ed, "redo"));

    QVERIFY(ed->document()->isUndoAvailable());
    QVERIFY(!ed->document()->isRedoAvailable());

    QCOMPARE(ed->textCursor().position(), 4);
}

void tst_QPlainTextEdit::lineWrapModes()
{
    QWidget *window = new QWidget;
    ed->setParent(window);
    window->show();
    ed->show();
    ed->setPlainText("a b c d e f g h i j k l m n o p q r s t u v w x y z");
    ed->setLineWrapMode(QPlainTextEdit::NoWrap);
    QCOMPARE(lineCount(), 1);
    ed->setLineWrapMode(QPlainTextEdit::WidgetWidth);

    // QPlainTextEdit does lazy line layout on resize, only for the visible blocks.
    // We thus need to make it wide enough to show something visible.
    int minimumWidth = 2 * ed->document()->documentMargin();
    minimumWidth += ed->fontMetrics().width(QLatin1Char('a'));
    minimumWidth += ed->frameWidth();
    ed->resize(minimumWidth, 1000);
    QCOMPARE(lineCount(), 26);
    ed->setParent(0);
    delete window;
}

#ifndef QTEST_NO_CURSOR
void tst_QPlainTextEdit::mouseCursorShape()
{
    // always show an IBeamCursor, see change 170146
    QVERIFY(!ed->isReadOnly());
    QVERIFY(ed->viewport()->cursor().shape() == Qt::IBeamCursor);

    ed->setReadOnly(true);
    QVERIFY(ed->viewport()->cursor().shape() == Qt::IBeamCursor);

    ed->setPlainText("Foo");
    QVERIFY(ed->viewport()->cursor().shape() == Qt::IBeamCursor);
}
#endif

void tst_QPlainTextEdit::implicitClear()
{
    // test that QPlainTextEdit::setHtml, etc. avoid calling clear() but instead call
    // QTextDocument::setHtml/etc. instead, which also clear the contents and
    // cached resource but preserve manually added resources. setHtml on a textedit
    // should behave the same as on a document with respect to that.
    // see also clearResources() autotest in qtextdocument

    // regular resource for QTextDocument
    QUrl testUrl(":/foobar");
    QVariant testResource("hello world");

    ed->document()->addResource(QTextDocument::ImageResource, testUrl, testResource);
    QVERIFY(ed->document()->resource(QTextDocument::ImageResource, testUrl) == testResource);

    ed->setPlainText("Blah");
    QVERIFY(ed->document()->resource(QTextDocument::ImageResource, testUrl) == testResource);

    ed->setPlainText("<b>Blah</b>");
    QVERIFY(ed->document()->resource(QTextDocument::ImageResource, testUrl) == testResource);

    ed->clear();
    QVERIFY(!ed->document()->resource(QTextDocument::ImageResource, testUrl).isValid());
    QVERIFY(ed->toPlainText().isEmpty());
}

#ifndef QT_NO_CLIPBOARD
void tst_QPlainTextEdit::copyAvailable_data()
{
    QTest::addColumn<pairListType>("keystrokes");
    QTest::addColumn<QList<bool> >("copyAvailable");
    QTest::addColumn<QString>("function");

    pairListType keystrokes;
    QList<bool> copyAvailable;

    keystrokes << qMakePair(Qt::Key_B, Qt::NoModifier) <<  qMakePair(Qt::Key_B, Qt::NoModifier)
               << qMakePair(Qt::Key_Left, Qt::ShiftModifier);
    copyAvailable << true ;
    QTest::newRow(QString("Case1 B,B, <- + shift | signals: true").toLatin1())
        << keystrokes << copyAvailable << QString();

    keystrokes.clear();
    copyAvailable.clear();

    keystrokes << qMakePair(Qt::Key_T, Qt::NoModifier) << qMakePair(Qt::Key_A, Qt::NoModifier)
               <<  qMakePair(Qt::Key_A, Qt::NoModifier) << qMakePair(Qt::Key_Left, Qt::ShiftModifier);
    copyAvailable << true << false;
    QTest::newRow(QString("Case2 T,A,A, <- + shift, cut() | signals: true, false").toLatin1())
        << keystrokes << copyAvailable << QString("cut");

    keystrokes.clear();
    copyAvailable.clear();

    keystrokes << qMakePair(Qt::Key_T, Qt::NoModifier) << qMakePair(Qt::Key_A, Qt::NoModifier)
               <<  qMakePair(Qt::Key_A, Qt::NoModifier) << qMakePair(Qt::Key_Left, Qt::ShiftModifier)
               << qMakePair(Qt::Key_Left, Qt::ShiftModifier)  << qMakePair(Qt::Key_Left, Qt::ShiftModifier);
    copyAvailable << true;
    QTest::newRow(QString("Case3 T,A,A, <- + shift, <- + shift, <- + shift, copy() | signals: true").toLatin1())
        << keystrokes << copyAvailable << QString("copy");

    keystrokes.clear();
    copyAvailable.clear();

    keystrokes << qMakePair(Qt::Key_T, Qt::NoModifier) << qMakePair(Qt::Key_A, Qt::NoModifier)
               <<  qMakePair(Qt::Key_A, Qt::NoModifier) << qMakePair(Qt::Key_Left, Qt::ShiftModifier)
               << qMakePair(Qt::Key_Left, Qt::ShiftModifier)  << qMakePair(Qt::Key_Left, Qt::ShiftModifier)
               << qMakePair(Qt::Key_X, Qt::ControlModifier);
    copyAvailable << true << false;
    QTest::newRow(QString("Case4 T,A,A, <- + shift, <- + shift, <- + shift, ctrl + x, paste() | signals: true, false").toLatin1())
        << keystrokes << copyAvailable << QString("paste");

    keystrokes.clear();
    copyAvailable.clear();

    keystrokes << qMakePair(Qt::Key_B, Qt::NoModifier) <<  qMakePair(Qt::Key_B, Qt::NoModifier)
               << qMakePair(Qt::Key_Left, Qt::ShiftModifier) << qMakePair(Qt::Key_Left, Qt::NoModifier);
    copyAvailable << true << false;
    QTest::newRow(QString("Case5 B,B, <- + shift, <- | signals: true, false").toLatin1())
        << keystrokes << copyAvailable << QString();

    keystrokes.clear();
    copyAvailable.clear();

    keystrokes << qMakePair(Qt::Key_B, Qt::NoModifier) <<  qMakePair(Qt::Key_A, Qt::NoModifier)
               << qMakePair(Qt::Key_Left, Qt::ShiftModifier) << qMakePair(Qt::Key_Left, Qt::NoModifier)
               << qMakePair(Qt::Key_Right, Qt::ShiftModifier);
    copyAvailable << true << false << true << false;
    QTest::newRow(QString("Case6 B,A, <- + shift, ->, <- + shift | signals: true, false, true, false").toLatin1())
        << keystrokes << copyAvailable << QString("cut");

    keystrokes.clear();
    copyAvailable.clear();

    keystrokes << qMakePair(Qt::Key_T, Qt::NoModifier) << qMakePair(Qt::Key_A, Qt::NoModifier)
               <<  qMakePair(Qt::Key_A, Qt::NoModifier) << qMakePair(Qt::Key_Left, Qt::ShiftModifier)
               << qMakePair(Qt::Key_Left, Qt::ShiftModifier)  << qMakePair(Qt::Key_Left, Qt::ShiftModifier)
               << qMakePair(Qt::Key_X, Qt::ControlModifier);
    copyAvailable << true << false << true;
    QTest::newRow(QString("Case7 T,A,A, <- + shift, <- + shift, <- + shift, ctrl + x, undo() | signals: true, false, true").toLatin1())
        << keystrokes << copyAvailable << QString("undo");
}

//Tests the copyAvailable slot for several cases
void tst_QPlainTextEdit::copyAvailable()
{
    QFETCH(pairListType,keystrokes);
    QFETCH(QList<bool>, copyAvailable);
    QFETCH(QString, function);

#ifdef Q_OS_MAC
    QSKIP("QTBUG-22283: copyAvailable has never passed on Mac");
#endif
    ed->clear();
    QApplication::clipboard()->clear();
    QVERIFY(!ed->canPaste());
    QSignalSpy spyCopyAvailabe(ed, SIGNAL(copyAvailable(bool)));

    //Execute Keystrokes
    foreach(keyPairType keyPair, keystrokes) {
        QTest::keyClick(ed, keyPair.first, keyPair.second );
    }

    //Execute ed->"function"
    if (function == "cut")
        ed->cut();
    else if (function == "copy")
        ed->copy();
    else if (function == "paste")
        ed->paste();
    else if (function == "undo")
        ed->paste();
    else if (function == "redo")
        ed->paste();

    //Compare spied signals
    QEXPECT_FAIL("Case7 T,A,A, <- + shift, <- + shift, <- + shift, ctrl + x, undo() | signals: true, false, true",
        "Wrong undo selection behaviour. Should be fixed in some future release. (See task: 132482)", Abort);
    QCOMPARE(spyCopyAvailabe.count(), copyAvailable.count());
    for (int i=0;i<spyCopyAvailabe.count(); i++) {
        QVariant variantSpyCopyAvailable = spyCopyAvailabe.at(i).at(0);
        QVERIFY2(variantSpyCopyAvailable.toBool() == copyAvailable.at(i), QString("Spied singnal: %1").arg(i).toLatin1());
    }
}
#endif

void tst_QPlainTextEdit::undoRedoAfterSetContent()
{
    QVERIFY(!ed->document()->isUndoAvailable());
    QVERIFY(!ed->document()->isRedoAvailable());
    ed->setPlainText("Foobar");
    QVERIFY(!ed->document()->isUndoAvailable());
    QVERIFY(!ed->document()->isRedoAvailable());
    ed->setPlainText("<p>bleh</p>");
    QVERIFY(!ed->document()->isUndoAvailable());
    QVERIFY(!ed->document()->isRedoAvailable());
}

void tst_QPlainTextEdit::numPadKeyNavigation()
{
    ed->setPlainText("Hello World");
    QCOMPARE(ed->textCursor().position(), 0);
    QTest::keyClick(ed, Qt::Key_Right, Qt::KeypadModifier);
    QCOMPARE(ed->textCursor().position(), 1);
}

void tst_QPlainTextEdit::moveCursor()
{
    ed->setPlainText("Test");

    QSignalSpy cursorMovedSpy(ed, SIGNAL(cursorPositionChanged()));

    QCOMPARE(ed->textCursor().position(), 0);
    ed->moveCursor(QTextCursor::NextCharacter);
    QCOMPARE(ed->textCursor().position(), 1);
    QCOMPARE(cursorMovedSpy.count(), 1);
    ed->moveCursor(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    QCOMPARE(ed->textCursor().position(), 2);
    QCOMPARE(cursorMovedSpy.count(), 2);
    QCOMPARE(ed->textCursor().selectedText(), QString("e"));
}

class MyTextEdit : public QPlainTextEdit
{
public:
    inline MyTextEdit()
        : createMimeDataCallCount(0),
          canInsertCallCount(0),
          insertCallCount(0)
    {}

    mutable int createMimeDataCallCount;
    mutable int canInsertCallCount;
    mutable int insertCallCount;

    virtual QMimeData *createMimeDataFromSelection() const {
        createMimeDataCallCount++;
        return QPlainTextEdit::createMimeDataFromSelection();
    }
    virtual bool canInsertFromMimeData(const QMimeData *source) const {
        canInsertCallCount++;
        return QPlainTextEdit::canInsertFromMimeData(source);
    }
    virtual void insertFromMimeData(const QMimeData *source) {
        insertCallCount++;
        QPlainTextEdit::insertFromMimeData(source);
    }

};

#ifndef QT_NO_CLIPBOARD
void tst_QPlainTextEdit::mimeDataReimplementations()
{
    MyTextEdit ed;
    ed.setPlainText("Hello World");

    QCOMPARE(ed.createMimeDataCallCount, 0);
    QCOMPARE(ed.canInsertCallCount, 0);
    QCOMPARE(ed.insertCallCount, 0);

    ed.selectAll();

    QCOMPARE(ed.createMimeDataCallCount, 0);
    QCOMPARE(ed.canInsertCallCount, 0);
    QCOMPARE(ed.insertCallCount, 0);

    ed.copy();

    QCOMPARE(ed.createMimeDataCallCount, 1);
    QCOMPARE(ed.canInsertCallCount, 0);
    QCOMPARE(ed.insertCallCount, 0);

#ifdef QT_BUILD_INTERNAL
    QWidgetTextControl *control = ed.findChild<QWidgetTextControl *>();
    QVERIFY(control);

    control->canInsertFromMimeData(QApplication::clipboard()->mimeData());

    QCOMPARE(ed.createMimeDataCallCount, 1);
    QCOMPARE(ed.canInsertCallCount, 1);
    QCOMPARE(ed.insertCallCount, 0);

    ed.paste();

    QCOMPARE(ed.createMimeDataCallCount, 1);
    QCOMPARE(ed.canInsertCallCount, 1);
    QCOMPARE(ed.insertCallCount, 1);
#endif
}
#endif

void tst_QPlainTextEdit::shiftEnterShouldInsertLineSeparator()
{
    QTest::keyClick(ed, Qt::Key_A);
    QTest::keyClick(ed, Qt::Key_Enter, Qt::ShiftModifier);
    QTest::keyClick(ed, Qt::Key_B);
    QString expected;
    expected += 'a';
    expected += QChar::LineSeparator;
    expected += 'b';
    QCOMPARE(ed->textCursor().block().text(), expected);
}

void tst_QPlainTextEdit::selectWordsFromStringsContainingSeparators_data()
{
    QTest::addColumn<QString>("testString");
    QTest::addColumn<QString>("selectedWord");

    QStringList wordSeparators;
    wordSeparators << "." << "," << "?" << "!" << ":" << ";" << "-" << "<" << ">" << "["
                   << "]" << "(" << ")" << "{" << "}" << "=" << "\t"<< QString(QChar::Nbsp);

    foreach (QString s, wordSeparators)
        QTest::newRow(QString("separator: " + s).toLocal8Bit()) << QString("foo") + s + QString("bar") << QString("foo");
}

void tst_QPlainTextEdit::selectWordsFromStringsContainingSeparators()
{
    QFETCH(QString, testString);
    QFETCH(QString, selectedWord);
    ed->setPlainText(testString);
    QTextCursor cursor = ed->textCursor();
    cursor.movePosition(QTextCursor::StartOfLine);
    cursor.select(QTextCursor::WordUnderCursor);
    QVERIFY(cursor.hasSelection());
    QCOMPARE(cursor.selection().toPlainText(), selectedWord);
    cursor.clearSelection();
}

#ifndef QT_NO_CLIPBOARD
void tst_QPlainTextEdit::canPaste()
{
    if (!PlatformClipboard::isAvailable())
        QSKIP("Clipboard not working with cron-started unit tests");

    QApplication::clipboard()->setText(QString());
    QVERIFY(!ed->canPaste());
    QApplication::clipboard()->setText("Test");
    QVERIFY(ed->canPaste());
    ed->setTextInteractionFlags(Qt::NoTextInteraction);
    QVERIFY(!ed->canPaste());
}
#endif

void tst_QPlainTextEdit::ensureCursorVisibleOnInitialShow()
{
    QString manyPagesOfPlainText;
    for (int i = 0; i < 800; ++i)
        manyPagesOfPlainText += QLatin1String("Blah blah blah blah blah blah\n");

    ed->setPlainText(manyPagesOfPlainText);
    QCOMPARE(ed->textCursor().position(), 0);

    ed->moveCursor(QTextCursor::End);
    ed->show();
    QVERIFY(ed->verticalScrollBar()->value() > 10);

    ed->moveCursor(QTextCursor::Start);
    QVERIFY(ed->verticalScrollBar()->value() < 10);
    ed->hide();
    ed->verticalScrollBar()->setValue(ed->verticalScrollBar()->maximum());
    ed->show();
    QCOMPARE(ed->verticalScrollBar()->value(), ed->verticalScrollBar()->maximum());
}

class TestEdit : public QPlainTextEdit
{
public:
    TestEdit() : resizeEventCalled(false) {}

    bool resizeEventCalled;

protected:
    virtual void resizeEvent(QResizeEvent *e)
    {
        QPlainTextEdit::resizeEvent(e);
        setPlainText("<img src=qtextbrowser-resizeevent.png width=" + QString::number(size().width()) + "><br>Size is " + QString::number(size().width()) + " x " + QString::number(size().height()));
        resizeEventCalled = true;
    }
};

void tst_QPlainTextEdit::setTextInsideResizeEvent()
{
    TestEdit edit;
    edit.show();
    edit.resize(800, 600);
    QVERIFY(edit.resizeEventCalled);
}

void tst_QPlainTextEdit::colorfulAppend()
{
    QTextCharFormat fmt;

    fmt.setForeground(QBrush(Qt::red));
    ed->mergeCurrentCharFormat(fmt);
    ed->appendPlainText("Red");
    fmt.setForeground(QBrush(Qt::blue));
    ed->mergeCurrentCharFormat(fmt);
    ed->appendPlainText("Blue");
    fmt.setForeground(QBrush(Qt::green));
    ed->mergeCurrentCharFormat(fmt);
    ed->appendPlainText("Green");

    QCOMPARE(ed->document()->blockCount(), 3);
    QTextBlock block = ed->document()->begin();
    QCOMPARE(block.begin().fragment().text(), QString("Red"));
    QVERIFY(block.begin().fragment().charFormat().foreground().color() == Qt::red);
    block = block.next();
    QCOMPARE(block.begin().fragment().text(), QString("Blue"));
    QVERIFY(block.begin().fragment().charFormat().foreground().color() == Qt::blue);
    block = block.next();
    QCOMPARE(block.begin().fragment().text(), QString("Green"));
    QVERIFY(block.begin().fragment().charFormat().foreground().color() == Qt::green);
}

void tst_QPlainTextEdit::ensureVisibleWithRtl()
{
    ed->setLayoutDirection(Qt::RightToLeft);
    ed->setLineWrapMode(QPlainTextEdit::NoWrap);
    QString txt(500, QChar(QLatin1Char('a')));
    QCOMPARE(txt.length(), 500);
    ed->setPlainText(txt);
    ed->resize(100, 100);
    ed->show();

    qApp->processEvents();

    QVERIFY(ed->horizontalScrollBar()->maximum() > 0);

    ed->moveCursor(QTextCursor::Start);
    QCOMPARE(ed->horizontalScrollBar()->value(), ed->horizontalScrollBar()->maximum());
    ed->moveCursor(QTextCursor::End);
    QCOMPARE(ed->horizontalScrollBar()->value(), 0);
    ed->moveCursor(QTextCursor::Start);
    QCOMPARE(ed->horizontalScrollBar()->value(), ed->horizontalScrollBar()->maximum());
    ed->moveCursor(QTextCursor::End);
    QCOMPARE(ed->horizontalScrollBar()->value(), 0);
}

void tst_QPlainTextEdit::preserveCharFormatAfterSetPlainText()
{
    QTextCharFormat fmt;
    fmt.setForeground(QBrush(Qt::blue));
    ed->mergeCurrentCharFormat(fmt);
    ed->setPlainText("This is blue");
    ed->appendPlainText("This should still be blue");
    QTextBlock block = ed->document()->begin();
    block = block.next();
    QCOMPARE(block.text(), QString("This should still be blue"));
    QVERIFY(block.begin().fragment().charFormat().foreground().color() == QColor(Qt::blue));
}

void tst_QPlainTextEdit::extraSelections()
{
    ed->setPlainText("Hello World");

    QTextCursor c = ed->textCursor();
    c.movePosition(QTextCursor::Start);
    c.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    const int endPos = c.position();

    QTextEdit::ExtraSelection sel;
    sel.cursor = c;
    ed->setExtraSelections(QList<QTextEdit::ExtraSelection>() << sel);

    c.movePosition(QTextCursor::Start);
    c.movePosition(QTextCursor::NextWord);
    const int wordPos = c.position();
    c.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    sel.cursor = c;
    ed->setExtraSelections(QList<QTextEdit::ExtraSelection>() << sel);

    QList<QTextEdit::ExtraSelection> selections = ed->extraSelections();
    QCOMPARE(selections.count(), 1);
    QCOMPARE(selections.at(0).cursor.position(), endPos);
    QCOMPARE(selections.at(0).cursor.anchor(), wordPos);
}

void tst_QPlainTextEdit::adjustScrollbars()
{
// For some reason ff is defined to be << on Mac Panther / gcc 3.3
#undef ff
    QFont ff(ed->font());
    ff.setFamily("Tahoma");
    ff.setPointSize(11);
    ed->setFont(ff);
    ed->setMinimumSize(140, 100);
    ed->setMaximumSize(140, 100);
    ed->show();
    QLatin1String txt("\nabc def ghi jkl mno pqr stu vwx");
    ed->setPlainText(txt + txt + txt + txt);

    QVERIFY(ed->verticalScrollBar()->maximum() > 0);

    ed->moveCursor(QTextCursor::End);
    int oldMaximum = ed->verticalScrollBar()->maximum();
    QTextCursor cursor = ed->textCursor();
    cursor.insertText(QLatin1String("\n"));
    cursor.deletePreviousChar();
    QCOMPARE(ed->verticalScrollBar()->maximum(), oldMaximum);
}

class SignalReceiver : public QObject
{
    Q_OBJECT
public:
    SignalReceiver() : received(0) {}

    int receivedSignals() const { return received; }
    QTextCharFormat charFormat() const { return format; }

public slots:
    void charFormatChanged(const QTextCharFormat &tcf) { ++received; format = tcf; }

private:
    QTextCharFormat format;
    int received;
};

void tst_QPlainTextEdit::textObscuredByScrollbars()
{
    ed->textCursor().insertText(
            "ab cab cab c abca kjsdf lka sjd lfk jsal df j kasdf abc ab abc "
            "a b c d e f g h i j k l m n o p q r s t u v w x y z "
            "abc abc abc abc abc abc abc abc abc abc abc abc abc abc abc abc "
            "ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab"
            "abc abc abc abc abc abc abc abc abc abc abc abc abc abc abc abc "
            "ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab"
            "abc abc abc abc abc abc abc abc abc abc abc abc abc abc abc abc "
            "ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab"
            "abc abc abc abc abc abc abc abc abc abc abc abc abc abc abc abc "
            "ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab"
            "abc abc abc abc abc abc abc abc abc abc abc abc abc abc abc abc "
            "ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab ab"
    );
    ed->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ed->show();

    QSize documentSize = ed->document()->documentLayout()->documentSize().toSize();
    QSize viewportSize = ed->viewport()->size();

    QVERIFY(documentSize.width() <= viewportSize.width());
}

void tst_QPlainTextEdit::setTextPreservesUndoRedoEnabled()
{
    QVERIFY(ed->isUndoRedoEnabled());

    ed->setPlainText("Test");

    QVERIFY(ed->isUndoRedoEnabled());

    ed->setUndoRedoEnabled(false);
    QVERIFY(!ed->isUndoRedoEnabled());
    ed->setPlainText("Test2");
    QVERIFY(!ed->isUndoRedoEnabled());

    ed->setPlainText("<p>hello");
    QVERIFY(!ed->isUndoRedoEnabled());
}

void tst_QPlainTextEdit::wordWrapProperty()
{
    {
        QPlainTextEdit edit;
        QTextDocument *doc = new QTextDocument(&edit);
        doc->setDocumentLayout(new QPlainTextDocumentLayout(doc));
        edit.setDocument(doc);
        edit.setWordWrapMode(QTextOption::NoWrap);
        QVERIFY(doc->defaultTextOption().wrapMode() == QTextOption::NoWrap);
    }
    {
        QPlainTextEdit edit;
        QTextDocument *doc = new QTextDocument(&edit);
        doc->setDocumentLayout(new QPlainTextDocumentLayout(doc));
        edit.setWordWrapMode(QTextOption::NoWrap);
        edit.setDocument(doc);
        QVERIFY(doc->defaultTextOption().wrapMode() == QTextOption::NoWrap);
    }
}

void tst_QPlainTextEdit::lineWrapProperty()
{
    QVERIFY(ed->wordWrapMode() == QTextOption::WrapAtWordBoundaryOrAnywhere);
    QVERIFY(ed->lineWrapMode() == QPlainTextEdit::WidgetWidth);
    ed->setLineWrapMode(QPlainTextEdit::NoWrap);
    QVERIFY(ed->lineWrapMode() == QPlainTextEdit::NoWrap);
    QVERIFY(ed->wordWrapMode() == QTextOption::WrapAtWordBoundaryOrAnywhere);
    QVERIFY(ed->document()->defaultTextOption().wrapMode() == QTextOption::NoWrap);
}

void tst_QPlainTextEdit::selectionChanged()
{
    ed->setPlainText("Hello World");

    ed->moveCursor(QTextCursor::Start);

    QSignalSpy selectionChangedSpy(ed, SIGNAL(selectionChanged()));

    QTest::keyClick(ed, Qt::Key_Right);
    QCOMPARE(ed->textCursor().position(), 1);
    QCOMPARE(selectionChangedSpy.count(), 0);

    QTest::keyClick(ed, Qt::Key_Right, Qt::ShiftModifier);
    QCOMPARE(ed->textCursor().position(), 2);
    QCOMPARE(selectionChangedSpy.count(), 1);

    QTest::keyClick(ed, Qt::Key_Right, Qt::ShiftModifier);
    QCOMPARE(ed->textCursor().position(), 3);
    QCOMPARE(selectionChangedSpy.count(), 2);

    QTest::keyClick(ed, Qt::Key_Right, Qt::ShiftModifier);
    QCOMPARE(ed->textCursor().position(), 4);
    QCOMPARE(selectionChangedSpy.count(), 3);

    QTest::keyClick(ed, Qt::Key_Right);
    QCOMPARE(ed->textCursor().position(), 4);
    QCOMPARE(selectionChangedSpy.count(), 4);

    QTest::keyClick(ed, Qt::Key_Right);
    QCOMPARE(ed->textCursor().position(), 5);
    QCOMPARE(selectionChangedSpy.count(), 4);
}

void tst_QPlainTextEdit::blockCountChanged()
{
    QSignalSpy blockCountCpangedSpy(ed, SIGNAL(blockCountChanged(int)));
    ed->setPlainText("Hello");
    QCOMPARE(blockCountCpangedSpy.count(), 0);
    ed->setPlainText("Hello World");
    QCOMPARE(blockCountCpangedSpy.count(), 0);
    ed->setPlainText("Hello \n World \n this \n has \n more \n blocks \n than \n just \n one");
    QCOMPARE(blockCountCpangedSpy.count(), 1);
    ed->setPlainText("One");
    QCOMPARE(blockCountCpangedSpy.count(), 2);
    ed->setPlainText("One \n Two");
    QCOMPARE(blockCountCpangedSpy.count(), 3);
    ed->setPlainText("Three \n Four");
    QCOMPARE(blockCountCpangedSpy.count(), 3);
}


void tst_QPlainTextEdit::insertAndScrollToBottom()
{
    ed->setPlainText("First Line");
    ed->show();
    QString text;
    for(int i = 0; i < 2000; ++i) {
        text += QLatin1String("this is another line of text to be appended. It is quite long and will probably wrap around, meaning the number of lines is larger than the number of blocks in the text.\n");
    }
    QTextCursor cursor = ed->textCursor();
    cursor.beginEditBlock();
    cursor.insertText(text);
    cursor.endEditBlock();
    ed->verticalScrollBar()->setValue(ed->verticalScrollBar()->maximum());
    QCOMPARE(ed->verticalScrollBar()->value(), ed->verticalScrollBar()->maximum());
}

Q_DECLARE_METATYPE(Qt::InputMethodHints)
void tst_QPlainTextEdit::inputMethodQueryImHints_data()
{
    QTest::addColumn<Qt::InputMethodHints>("hints");

    QTest::newRow("None") << static_cast<Qt::InputMethodHints>(Qt::ImhNone);
    QTest::newRow("Password") << static_cast<Qt::InputMethodHints>(Qt::ImhHiddenText);
    QTest::newRow("Normal") << static_cast<Qt::InputMethodHints>(Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText | Qt::ImhSensitiveData);
}

void tst_QPlainTextEdit::inputMethodQueryImHints()
{
    QFETCH(Qt::InputMethodHints, hints);
    ed->setInputMethodHints(hints);

    QVariant value = ed->inputMethodQuery(Qt::ImHints);
    QCOMPARE(static_cast<Qt::InputMethodHints>(value.toInt()), hints);
}

QTEST_MAIN(tst_QPlainTextEdit)
#include "tst_qplaintextedit.moc"
