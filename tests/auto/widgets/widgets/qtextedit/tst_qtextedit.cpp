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
#include <qtexttable.h>
#include <qpainter.h>
#include <qimagereader.h>
#include <qimagewriter.h>
#include <qcommonstyle.h>
#include <qlayout.h>
#include <qdir.h>
#include <qpaintengine.h>

#include <qabstracttextdocumentlayout.h>
#include <qtextdocumentfragment.h>
#include <qsyntaxhighlighter.h>

#include "../../../shared/platformclipboard.h"
#include "../../../shared/platforminputcontext.h"
#include <private/qinputmethod_p.h>

//Used in copyAvailable
typedef QPair<Qt::Key, Qt::KeyboardModifier> keyPairType;
typedef QList<keyPairType> pairListType;
Q_DECLARE_METATYPE(keyPairType);

Q_DECLARE_METATYPE(QList<QInputMethodEvent::Attribute>);

QT_FORWARD_DECLARE_CLASS(QTextEdit)

class tst_QTextEdit : public QObject
{
    Q_OBJECT
public:
    tst_QTextEdit();

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
private slots:
    void getSetCheck();
    void inlineAttributesOnInsert();
    void inlineAttributesOnSelection();
    void inlineAttributeSymmetry();
    void inlineAttributeSymmetryWithSelection();
    void autoBulletList1();
    void autoBulletList2();
    void preserveCharFormatAfterNewline();
#ifndef QT_NO_CLIPBOARD
    void clearMustNotChangeClipboard();
#endif
    void clearMustNotResetRootFrameMarginToDefault();
    void clearShouldPreserveTheCurrentCharFormat();
    void clearShouldClearExtraSelections();
    void paragSeparatorOnPlaintextAppend();
    void layoutingLoop();
#ifndef QT_NO_CLIPBOARD
    void selectAllSetsNotSelection();
#endif
    void asciiTab();
    void setDocument();
    void setDocument_shared();
    void mergeCurrentCharFormat();
    void mergeCurrentBlockCharFormat();
    void emptyAppend();
    void appendOnEmptyDocumentShouldReuseInitialParagraph();
    void cursorPositionChanged();
    void setTextCursor();
#ifndef QT_NO_CLIPBOARD
    void undoAvailableAfterPaste();
#endif
    void undoRedoAvailableRepetition();
    void appendShouldUseCurrentFormat();
    void appendShouldNotTouchTheSelection();
    void backspace();
    void shiftBackspace();
    void undoRedo();
    void preserveCharFormatInAppend();
#ifndef QT_NO_CLIPBOARD
    void copyAndSelectAllInReadonly();
#endif
    void noPropertiesOnDefaultTextEditCharFormat();
    void setPlainTextShouldUseCurrentCharFormat();
    void setPlainTextShouldEmitTextChangedOnce();
    void overwriteMode();
    void shiftDownInLineLastShouldSelectToEnd_data();
    void shiftDownInLineLastShouldSelectToEnd();
    void undoRedoShouldRepositionTextEditCursor();
    void lineWrapModes();
#ifndef QT_NO_CURSOR
    void mouseCursorShape();
#endif
    void implicitClear();
    void undoRedoAfterSetContent();
    void numPadKeyNavigation();
    void moveCursor();
#ifndef QT_NO_CLIPBOARD
    void mimeDataReimplementations();
#endif
    void ctrlEnterShouldInsertLineSeparator_NOT();
    void shiftEnterShouldInsertLineSeparator();
    void selectWordsFromStringsContainingSeparators_data();
    void selectWordsFromStringsContainingSeparators();
#ifndef QT_NO_CLIPBOARD
    void canPaste();
    void copyAvailable_data();
    void copyAvailable();
#endif
    void ensureCursorVisibleOnInitialShow();
    void setHtmlInsideResizeEvent();
    void colorfulAppend();
    void ensureVisibleWithRtl();
    void preserveCharFormatAfterSetPlainText();
    void extraSelections();
    void adjustScrollbars();
    void currentCharFormatChanged();
    void textObscuredByScrollbars();
    void setTextPreservesUndoRedoEnabled();
    void wordWrapProperty();
    void lineWrapProperty();
    void selectionChanged();
#ifndef QT_NO_CLIPBOARD
    void copyPasteBackgroundImage();
    void copyPasteForegroundImage();
#endif
    void setText();
    void cursorRect();
#ifdef QT_BUILD_INTERNAL
    void fullWidthSelection_data();
    void fullWidthSelection();
    void fullWidthSelection2();
    void setDocumentPreservesPalette();
#endif
    void pasteFromQt3RichText();
    void noWrapBackgrounds();
    void preserveCharFormatAfterUnchangingSetPosition();
    void twoSameInputMethodEvents();
#ifndef QT_NO_CONTEXTMENU
    void taskQTBUG_7902_contextMenuCrash();
#endif
    void bidiVisualMovement_data();
    void bidiVisualMovement();

    void bidiLogicalMovement_data();
    void bidiLogicalMovement();

    void inputMethodEvent();
    void inputMethodSelection();
    void inputMethodQuery();
    void inputMethodQueryImHints_data();
    void inputMethodQueryImHints();
    void inputMethodCursorRect();

    void highlightLongLine();

    void countTextChangedOnRemove();

#ifndef QT_NO_REGEXP
    void findWithRegExp();
    void findBackwardWithRegExp();
    void findWithRegExpReturnsFalseIfNoMoreResults();
#endif

#if QT_CONFIG(regularexpression)
    void findWithRegularExpression();
    void findBackwardWithRegularExpression();
    void findWithRegularExpressionReturnsFalseIfNoMoreResults();
#endif

#if QT_CONFIG(wheelevent)
    void wheelEvent();
#endif

    void preeditCharFormat_data();
    void preeditCharFormat();

private:
    void createSelection();
    int blockCount() const;
    void compareWidgetAndImage(QTextEdit &widget, const QString &imageFileName);

    QTextEdit *ed;
    qreal rootFrameMargin;
    PlatformInputContext m_platformInputContext;
    const QString m_fullWidthSelectionImagesFolder;
};

// Testing get/set functions
void tst_QTextEdit::getSetCheck()
{
    QTextEdit obj1;
    // QTextDocument * QTextEdit::document()
    // void QTextEdit::setDocument(QTextDocument *)
    QTextDocument *var1 = new QTextDocument;
    obj1.setDocument(var1);
    QCOMPARE(var1, obj1.document());
    obj1.setDocument((QTextDocument *)0);
    QVERIFY(var1 != obj1.document()); // QTextEdit creates a new document when setting 0
    QVERIFY((QTextDocument *)0 != obj1.document());
    delete var1;

    // AutoFormatting QTextEdit::autoFormatting()
    // void QTextEdit::setAutoFormatting(AutoFormatting)
    obj1.setAutoFormatting(QTextEdit::AutoFormatting(QTextEdit::AutoNone));
    QCOMPARE(QTextEdit::AutoFormatting(QTextEdit::AutoNone), obj1.autoFormatting());
    obj1.setAutoFormatting(QTextEdit::AutoFormatting(QTextEdit::AutoBulletList));
    QCOMPARE(QTextEdit::AutoFormatting(QTextEdit::AutoBulletList), obj1.autoFormatting());
    obj1.setAutoFormatting(QTextEdit::AutoFormatting(QTextEdit::AutoAll));
    QCOMPARE(QTextEdit::AutoFormatting(QTextEdit::AutoAll), obj1.autoFormatting());

    // bool QTextEdit::tabChangesFocus()
    // void QTextEdit::setTabChangesFocus(bool)
    obj1.setTabChangesFocus(false);
    QCOMPARE(false, obj1.tabChangesFocus());
    obj1.setTabChangesFocus(true);
    QCOMPARE(true, obj1.tabChangesFocus());

    // LineWrapMode QTextEdit::lineWrapMode()
    // void QTextEdit::setLineWrapMode(LineWrapMode)
    obj1.setLineWrapMode(QTextEdit::LineWrapMode(QTextEdit::NoWrap));
    QCOMPARE(QTextEdit::LineWrapMode(QTextEdit::NoWrap), obj1.lineWrapMode());
    obj1.setLineWrapMode(QTextEdit::LineWrapMode(QTextEdit::WidgetWidth));
    QCOMPARE(QTextEdit::LineWrapMode(QTextEdit::WidgetWidth), obj1.lineWrapMode());
    obj1.setLineWrapMode(QTextEdit::LineWrapMode(QTextEdit::FixedPixelWidth));
    QCOMPARE(QTextEdit::LineWrapMode(QTextEdit::FixedPixelWidth), obj1.lineWrapMode());
    obj1.setLineWrapMode(QTextEdit::LineWrapMode(QTextEdit::FixedColumnWidth));
    QCOMPARE(QTextEdit::LineWrapMode(QTextEdit::FixedColumnWidth), obj1.lineWrapMode());

    // int QTextEdit::lineWrapColumnOrWidth()
    // void QTextEdit::setLineWrapColumnOrWidth(int)
    obj1.setLineWrapColumnOrWidth(0);
    QCOMPARE(0, obj1.lineWrapColumnOrWidth());
    obj1.setLineWrapColumnOrWidth(INT_MIN);
    QCOMPARE(INT_MIN, obj1.lineWrapColumnOrWidth());
    obj1.setLineWrapColumnOrWidth(INT_MAX);
    QCOMPARE(INT_MAX, obj1.lineWrapColumnOrWidth());

    // bool QTextEdit::overwriteMode()
    // void QTextEdit::setOverwriteMode(bool)
    obj1.setOverwriteMode(false);
    QCOMPARE(false, obj1.overwriteMode());
    obj1.setOverwriteMode(true);
    QCOMPARE(true, obj1.overwriteMode());

    // int QTextEdit::tabStopWidth()
    // void QTextEdit::setTabStopWidth(int)
    obj1.setTabStopDistance(0);
    QCOMPARE(0, obj1.tabStopDistance());
    obj1.setTabStopDistance(-1);
    QCOMPARE(0, obj1.tabStopDistance()); // Makes no sense to set a negative tabstop value
    obj1.setTabStopDistance(std::numeric_limits<qreal>::max());
    QCOMPARE(std::numeric_limits<qreal>::max(), obj1.tabStopDistance());

    // bool QTextEdit::acceptRichText()
    // void QTextEdit::setAcceptRichText(bool)
    obj1.setAcceptRichText(false);
    QCOMPARE(false, obj1.acceptRichText());
    obj1.setAcceptRichText(true);
    QCOMPARE(true, obj1.acceptRichText());

    // qreal QTextEdit::fontPointSize()
    // void QTextEdit::setFontPointSize(qreal)
    obj1.setFontPointSize(qreal(1.1));
    QCOMPARE(qreal(1.1), obj1.fontPointSize());
    // we currently assert in QFont::setPointSizeF for that
    //obj1.setFontPointSize(0.0);
    //QCOMPARE(1.1, obj1.fontPointSize()); // Should not accept 0.0 => keep old

    // int QTextEdit::fontWeight()
    // void QTextEdit::setFontWeight(int)
    obj1.setFontWeight(1);
    QCOMPARE(1, obj1.fontWeight()); // Range<1, 99>
    obj1.setFontWeight(99);
    QCOMPARE(99, obj1.fontWeight()); // Range<1, 99>
    /* assertion in qfont.cpp
    obj1.setFontWeight(INT_MIN);
    QCOMPARE(1, obj1.fontWeight()); // Range<1, 99>
    obj1.setFontWeight(INT_MAX);
    QCOMPARE(99, obj1.fontWeight()); // Range<1, 99>
    */

    // bool QTextEdit::fontUnderline()
    // void QTextEdit::setFontUnderline(bool)
    obj1.setFontUnderline(false);
    QCOMPARE(false, obj1.fontUnderline());
    obj1.setFontUnderline(true);
    QCOMPARE(true, obj1.fontUnderline());

    // bool QTextEdit::fontItalic()
    // void QTextEdit::setFontItalic(bool)
    obj1.setFontItalic(false);
    QCOMPARE(false, obj1.fontItalic());
    obj1.setFontItalic(true);
    QCOMPARE(true, obj1.fontItalic());
}

class QtTestDocumentLayout : public QAbstractTextDocumentLayout
{
    Q_OBJECT
public:
    inline QtTestDocumentLayout(QTextEdit *edit, QTextDocument *doc, int &itCount)
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

    QTextEdit *ed;

    int &iterationCounter;
};

tst_QTextEdit::tst_QTextEdit() :
    m_fullWidthSelectionImagesFolder(QFINDTESTDATA("fullWidthSelection"))
{

}

void tst_QTextEdit::initTestCase()
{
    QInputMethodPrivate *inputMethodPrivate = QInputMethodPrivate::get(qApp->inputMethod());
    inputMethodPrivate->testContext = &m_platformInputContext;
    QVERIFY2(!m_fullWidthSelectionImagesFolder.isEmpty(), qPrintable(QString::fromLatin1("Cannot locate 'fullWidthSelection' starting from %1").arg(QDir::currentPath())));
}

void tst_QTextEdit::cleanupTestCase()
{
    QInputMethodPrivate *inputMethodPrivate = QInputMethodPrivate::get(qApp->inputMethod());
    inputMethodPrivate->testContext = 0;
}

void tst_QTextEdit::init()
{
    ed = new QTextEdit(0);
    rootFrameMargin = ed->document()->documentMargin();
}

void tst_QTextEdit::cleanup()
{
    delete ed;
    ed = 0;
}

void tst_QTextEdit::inlineAttributesOnInsert()
{
    const QColor blue(Qt::blue);
    QVERIFY(ed->textCursor().charFormat().foreground().color() != blue);

    ed->setTextColor(blue);
    QTest::keyClick(ed, Qt::Key_A);

    QCOMPARE(ed->textCursor().charFormat().foreground().color(), blue);
}

void tst_QTextEdit::inlineAttributesOnSelection()
{
    createSelection();

    ed->setFontItalic(true);

    QVERIFY(ed->textCursor().charFormat().fontItalic());
}

void tst_QTextEdit::inlineAttributeSymmetry()
{
    ed->setFontPointSize(42.0);
    QCOMPARE(double(ed->fontPointSize()), 42.0);

    ed->setFontFamily("Test");
    QCOMPARE(ed->fontFamily(), QString("Test"));

    ed->setFontWeight(QFont::Bold);
    QCOMPARE((int)ed->fontWeight(), (int)QFont::Bold);

    ed->setFontUnderline(true);
    QCOMPARE(ed->fontUnderline(), true);

    ed->setFontItalic(true);
    QCOMPARE(ed->fontItalic(), true);

    ed->setTextColor(Qt::blue);
    QCOMPARE(ed->textColor(), QColor(Qt::blue));

    ed->setTextBackgroundColor(Qt::red);
    QCOMPARE(ed->textBackgroundColor(), QColor(Qt::red));

    ed->setAlignment(Qt::AlignRight);
    QCOMPARE((int)ed->alignment(), (int)Qt::AlignRight);
}

void tst_QTextEdit::inlineAttributeSymmetryWithSelection()
{
    createSelection();

    inlineAttributeSymmetry();
}

void tst_QTextEdit::autoBulletList1()
{
    ed->setAutoFormatting(QTextEdit::AutoBulletList);

    QTest::keyClick(ed, Qt::Key_Return);
    QTest::keyClicks(ed, "*This should become a list");

    QVERIFY(ed->textCursor().currentList());
    QCOMPARE(ed->textCursor().currentList()->format().style(), QTextListFormat::ListDisc);
}

void tst_QTextEdit::autoBulletList2()
{
    ed->setAutoFormatting(QTextEdit::AutoNone);
    QTest::keyClick(ed, Qt::Key_Return);
    QTest::keyClicks(ed, "*This should NOT become a list");

    QVERIFY(!ed->textCursor().currentList());
}

void tst_QTextEdit::preserveCharFormatAfterNewline()
{
    ed->setTextColor(Qt::blue);
    QTest::keyClicks(ed, "Hello");

    QTest::keyClick(ed, Qt::Key_Return);

    QCOMPARE(ed->textColor(), QColor(Qt::blue));
}

void tst_QTextEdit::createSelection()
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
void tst_QTextEdit::clearMustNotChangeClipboard()
{
    if (!PlatformClipboard::isAvailable())
        QSKIP("Clipboard not working with cron-started unit tests");

    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    ed->textCursor().insertText("Hello World");
    QString txt("This is different text");
    QApplication::clipboard()->setText(txt);
    ed->clear();
    QCOMPARE(QApplication::clipboard()->text(), txt);
}
#endif

void tst_QTextEdit::clearMustNotResetRootFrameMarginToDefault()
{
    QCOMPARE(ed->document()->rootFrame()->frameFormat().margin(), rootFrameMargin);
    ed->clear();
    QCOMPARE(ed->document()->rootFrame()->frameFormat().margin(), rootFrameMargin);
}

void tst_QTextEdit::clearShouldPreserveTheCurrentCharFormat()
{
    ed->setFontUnderline(true);
    QVERIFY(ed->fontUnderline());
    ed->clear();
    QVERIFY(ed->fontUnderline());
}

void tst_QTextEdit::clearShouldClearExtraSelections()
{
    QTextEdit::ExtraSelection sel;
    sel.cursor = ed->textCursor();
    sel.format.setProperty(QTextFormat::FullWidthSelection, true);
    ed->setExtraSelections(QList<QTextEdit::ExtraSelection>() << sel);
    QCOMPARE(ed->extraSelections().count(), 1);
    ed->clear();
    QCOMPARE(ed->extraSelections().count(), 0);
}

void tst_QTextEdit::paragSeparatorOnPlaintextAppend()
{
    ed->append("Hello\nWorld");
    int cnt = 0;
    QTextBlock blk = ed->document()->begin();
    while (blk.isValid()) {
        ++cnt;
        blk = blk.next();
    }
    QCOMPARE(cnt, 2);
}

void tst_QTextEdit::layoutingLoop()
{
    QPointer<QTextEdit> ed = new QTextEdit(0);
    // this is a testcase for an ugly layouting problem, causing an infinite loop.
    // QTextEdit's resizeEvent has a long comment about what and why it can happen.

    int callsToSetPageSize = 0;

    QTextDocument *doc = new QTextDocument;
    QtTestDocumentLayout *lout = new QtTestDocumentLayout(ed, doc, callsToSetPageSize);
    doc->setDocumentLayout(lout);
    ed->setDocument(doc);

    ed->show();
    ed->resize(100, 100);

    qApp->processEvents();
    delete doc;
    delete ed;

    // ###### should need less!
    QVERIFY(callsToSetPageSize < 10);
}

#ifndef QT_NO_CLIPBOARD
void tst_QTextEdit::selectAllSetsNotSelection()
{
    if (!QApplication::clipboard()->supportsSelection()) {
        QSKIP("Test only relevant for systems with selection");
        return;
    }

    QApplication::clipboard()->setText(QString("foobar"), QClipboard::Selection);
    QCOMPARE(QApplication::clipboard()->text(QClipboard::Selection), QString("foobar"));

    ed->insertPlainText("Hello World");
    ed->selectAll();

    QCOMPARE(QApplication::clipboard()->text(QClipboard::Selection), QString::fromLatin1("foobar"));
}
#endif
void tst_QTextEdit::asciiTab()
{
    QTextEdit edit;
    edit.setPlainText("\t");
    edit.show();
    qApp->processEvents();
    QCOMPARE(edit.toPlainText().at(0), QChar('\t'));
}


void tst_QTextEdit::setDocument()
{
    QTextDocument *document = new QTextDocument(ed);
    QCOMPARE(document->isModified(), false);
    QCOMPARE(document->isUndoAvailable(), false);
    QTextCursor(document).insertText("Test");
    QCOMPARE(document->isUndoAvailable(), true);
    QCOMPARE(document->isModified(), true);
    ed->setDocument(document);
    QCOMPARE(ed->toPlainText(), QString("Test"));
    QCOMPARE(document->isUndoAvailable(), true);
    QCOMPARE(document->isModified(), true);
}

void tst_QTextEdit::setDocument_shared()
{
    QTextDocument *document = new QTextDocument(ed);
    QCOMPARE(document->isModified(), false);
    QCOMPARE(document->isUndoAvailable(), false);
    QTextCursor(document).insertText("Test");
    QCOMPARE(document->isUndoAvailable(), true);
    document->setModified(false);
    ed->setDocument(document);
    QCOMPARE(ed->toPlainText(), QString("Test"));
    QCOMPARE(document->isUndoAvailable(), true);
    QCOMPARE(document->isModified(), false);
    QTextCursor(document).insertText("Test2");
    QCOMPARE(document->isModified(), true);
    QTextEdit editor2;
    editor2.setDocument(document);
    QCOMPARE(document->isUndoAvailable(), true);
    QCOMPARE(document->isModified(), true);
}

void tst_QTextEdit::mergeCurrentCharFormat()
{
    ed->setPlainText("Hello Test World");
    QTextCursor cursor = ed->textCursor();
    cursor.setPosition(7);
    ed->setTextCursor(cursor);

    QTextCharFormat mod;
    mod.setFontItalic(true);
    mod.setForeground(Qt::red);
    ed->mergeCurrentCharFormat(mod);

    cursor.movePosition(QTextCursor::Right);
    cursor.movePosition(QTextCursor::Right);
    // do NOT select the current word under the cursor, /JUST/
    // call mergeCharFormat on the cursor
    QVERIFY(!cursor.charFormat().fontItalic());
    QVERIFY(cursor.charFormat().foreground().color() != Qt::red);
}

void tst_QTextEdit::mergeCurrentBlockCharFormat()
{
    ed->setPlainText("FirstLine\n\nThirdLine");
    QTextCursor cursor = ed->textCursor();
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::Down);

    // make sure we're in the empty second line
    QVERIFY(cursor.atBlockStart());
    QVERIFY(cursor.atBlockEnd());

    ed->setTextCursor(cursor);

    QTextCharFormat mod;
    mod.setForeground(Qt::red);
    ed->mergeCurrentCharFormat(mod);

    QVERIFY(cursor.blockCharFormat().foreground().color() != Qt::red);
    cursor.movePosition(QTextCursor::Up);
    QVERIFY(cursor.blockCharFormat().foreground().color() != Qt::red);
    cursor.movePosition(QTextCursor::Down);
    cursor.movePosition(QTextCursor::Down);
    QVERIFY(cursor.blockCharFormat().foreground().color() != Qt::red);
}

int tst_QTextEdit::blockCount() const
{
    int blocks = 0;
    for (QTextBlock block = ed->document()->begin(); block.isValid(); block = block.next())
        ++blocks;
    return blocks;
}

// Supporter issue #56783
void tst_QTextEdit::emptyAppend()
{
    ed->append("Blah");
    QCOMPARE(blockCount(), 1);
    ed->append(QString());
    QCOMPARE(blockCount(), 2);
    ed->append(QString("   "));
    QCOMPARE(blockCount(), 3);
}

void tst_QTextEdit::appendOnEmptyDocumentShouldReuseInitialParagraph()
{
    QCOMPARE(blockCount(), 1);
    ed->append("Blah");
    QCOMPARE(blockCount(), 1);
}

class CursorPositionChangedRecorder : public QObject
{
    Q_OBJECT
public:
    inline CursorPositionChangedRecorder(QTextEdit *ed)
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
    QTextEdit *editor;
};

void tst_QTextEdit::cursorPositionChanged()
{
    QSignalSpy spy(ed, SIGNAL(cursorPositionChanged()));
    ed->show();

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

    cursor.movePosition(QTextCursor::Start);
    ed->setTextCursor(cursor);
    spy.clear();
    QVERIFY(!ed->textCursor().hasSelection());
    QTest::mouseDClick(ed->viewport(), Qt::LeftButton, {}, ed->cursorRect().center());
    QVERIFY(ed->textCursor().hasSelection());

    QCOMPARE(spy.count(), 1);

    CursorPositionChangedRecorder spy2(ed);
    QVERIFY(ed->textCursor().position() > 0);
    ed->setPlainText("Hello World");
    QCOMPARE(spy2.cursorPositions.count(), 1);
    QCOMPARE(spy2.cursorPositions.at(0), 0);
    QCOMPARE(ed->textCursor().position(), 0);
}

void tst_QTextEdit::setTextCursor()
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
void tst_QTextEdit::undoAvailableAfterPaste()
{
    if (!PlatformClipboard::isAvailable())
        QSKIP("Clipboard not working with cron-started unit tests");

    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

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

void tst_QTextEdit::undoRedoAvailableRepetition()
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

void tst_QTextEdit::appendShouldUseCurrentFormat()
{
    ed->textCursor().insertText("A");
    QTextCharFormat fmt;
    fmt.setForeground(Qt::blue);
    fmt.setFontItalic(true);
    ed->setCurrentCharFormat(fmt);
    ed->append("Hello");
    const QColor blue(Qt::blue);

    QTextCursor cursor(ed->document());

    QVERIFY(cursor.movePosition(QTextCursor::NextCharacter));
    QVERIFY(cursor.charFormat().foreground().color() != blue);
    QVERIFY(!cursor.charFormat().fontItalic());

    QVERIFY(cursor.movePosition(QTextCursor::NextBlock));

    {
        QTextCursor tmp = cursor;
        tmp.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        QCOMPARE(tmp.selectedText(), QString::fromLatin1("Hello"));
    }

    QVERIFY(cursor.movePosition(QTextCursor::NextCharacter));
    QCOMPARE(cursor.charFormat().foreground().color(), blue);
    QVERIFY(cursor.charFormat().fontItalic());
}

void tst_QTextEdit::appendShouldNotTouchTheSelection()
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

    ed->append("<b>Some Bold Text</b>");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter);
    QCOMPARE(cursor.charFormat().foreground().color(), QColor(Qt::blue));
}

void tst_QTextEdit::backspace()
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

void tst_QTextEdit::shiftBackspace()
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

void tst_QTextEdit::undoRedo()
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
void tst_QTextEdit::preserveCharFormatInAppend()
{
    ed->append("First para");
    ed->append("<b>Second para</b>");
    ed->append("third para");

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
void tst_QTextEdit::copyAndSelectAllInReadonly()
{
    if (!PlatformClipboard::isAvailable())
        QSKIP("Clipboard not working with cron-started unit tests");

    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

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

void tst_QTextEdit::noPropertiesOnDefaultTextEditCharFormat()
{
    // there should be no properties set on the default/initial char format
    // on a text edit. Font properties instead should be taken from the
    // widget's font (in sync with defaultFont property in document) and the
    // foreground color should be taken from the palette.
    QCOMPARE(ed->currentCharFormat().properties().count(), 0);
}

void tst_QTextEdit::setPlainTextShouldUseCurrentCharFormat()
{
    ed->setFontUnderline(true);
    ed->setPlainText("Hello World");
    QTextCursor cursor(ed->document());
    cursor.movePosition(QTextCursor::NextCharacter);
    QCOMPARE(cursor.charFormat().fontUnderline(), true);

    ed->setHtml("<p style=\"color:blue\">Foo</p>");
    ed->setPlainText("Hello World");
    cursor = QTextCursor(ed->document());
    cursor.movePosition(QTextCursor::NextCharacter);
    QCOMPARE(cursor.charFormat().fontUnderline(), true);
}

void tst_QTextEdit::setPlainTextShouldEmitTextChangedOnce()
{
    QSignalSpy spy(ed, SIGNAL(textChanged()));
    ed->setPlainText("Yankee Doodle");
    QCOMPARE(spy.count(), 1);
    ed->setPlainText("");
    QCOMPARE(spy.count(), 2);
}

void tst_QTextEdit::overwriteMode()
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

void tst_QTextEdit::shiftDownInLineLastShouldSelectToEnd_data()
{
    // shift cursor-down in the last line should select to the end of the document

    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("totalLineCount");

    QTest::newRow("1") << QString("Foo\nBar") << 2;
    QTest::newRow("2") << QString("Foo\nBar") + QChar(QChar::LineSeparator) + QString("Baz") << 3;
}

void tst_QTextEdit::shiftDownInLineLastShouldSelectToEnd()
{
    QFETCH(QString, input);
    QFETCH(int, totalLineCount);

    ed->setPlainText(input);
    ed->show();

    // ensure we're layouted
    ed->document()->documentLayout()->documentSize();

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

void tst_QTextEdit::undoRedoShouldRepositionTextEditCursor()
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

void tst_QTextEdit::lineWrapModes()
{
    ed->setLineWrapMode(QTextEdit::NoWrap);
    // NoWrap at the same time as having all lines that are all left aligned means we optimize to only layout once. The effect is that the width is always 0
    QCOMPARE(ed->document()->pageSize().width(), qreal(0));

    QTextCursor cursor = QTextCursor(ed->document());
    cursor.insertText(QString("A simple line"));
    cursor.insertBlock();
    QTextBlockFormat fmt;
    fmt.setAlignment(Qt::AlignRight);
    cursor.mergeBlockFormat(fmt);
    cursor.insertText(QString("Another line"));
    ed->show(); // relayout;
    QVERIFY(ed->document()->pageSize().width() > qreal(0));

    ed->setLineWrapColumnOrWidth(10);
    ed->setLineWrapMode(QTextEdit::FixedColumnWidth);
    QVERIFY(!qIsNull(ed->document()->pageSize().width()));

    ed->setLineWrapColumnOrWidth(1000);
    ed->setLineWrapMode(QTextEdit::FixedPixelWidth);
    QCOMPARE(ed->document()->pageSize().width(), qreal(1000));
}

#ifndef QT_NO_CURSOR
void tst_QTextEdit::mouseCursorShape()
{
    // always show an IBeamCursor, see change 170146
    QVERIFY(!ed->isReadOnly());
    QCOMPARE(ed->viewport()->cursor().shape(), Qt::IBeamCursor);

    ed->setReadOnly(true);
    QCOMPARE(ed->viewport()->cursor().shape(), Qt::IBeamCursor);

    ed->setPlainText("Foo");
    QCOMPARE(ed->viewport()->cursor().shape(), Qt::IBeamCursor);
}
#endif

void tst_QTextEdit::implicitClear()
{
    // test that QTextEdit::setHtml, etc. avoid calling clear() but instead call
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
void tst_QTextEdit::copyAvailable_data()
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
void tst_QTextEdit::copyAvailable()
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

void tst_QTextEdit::undoRedoAfterSetContent()
{
    QVERIFY(!ed->document()->isUndoAvailable());
    QVERIFY(!ed->document()->isRedoAvailable());
    ed->setPlainText("Foobar");
    QVERIFY(!ed->document()->isUndoAvailable());
    QVERIFY(!ed->document()->isRedoAvailable());
    ed->setHtml("<p>bleh</p>");
    QVERIFY(!ed->document()->isUndoAvailable());
    QVERIFY(!ed->document()->isRedoAvailable());
}

void tst_QTextEdit::numPadKeyNavigation()
{
    ed->setText("Hello World");
    QCOMPARE(ed->textCursor().position(), 0);
    QTest::keyClick(ed, Qt::Key_Right, Qt::KeypadModifier);
    QCOMPARE(ed->textCursor().position(), 1);
}

void tst_QTextEdit::moveCursor()
{
    ed->setText("Test");

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

class MyTextEdit : public QTextEdit
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
        return QTextEdit::createMimeDataFromSelection();
    }
    virtual bool canInsertFromMimeData(const QMimeData *source) const {
        canInsertCallCount++;
        return QTextEdit::canInsertFromMimeData(source);
    }
    virtual void insertFromMimeData(const QMimeData *source) {
        insertCallCount++;
        QTextEdit::insertFromMimeData(source);
    }

};

#ifndef QT_NO_CLIPBOARD
void tst_QTextEdit::mimeDataReimplementations()
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

void tst_QTextEdit::ctrlEnterShouldInsertLineSeparator_NOT()
{
    QTest::keyClick(ed, Qt::Key_A);
    QTest::keyClick(ed, Qt::Key_Enter, Qt::ControlModifier);
    QTest::keyClick(ed, Qt::Key_B);
    QString expected;
    expected += 'a';
//    expected += QChar::LineSeparator; // do NOT insert
    expected += 'b';
    QCOMPARE(ed->textCursor().block().text(), expected);
}

void tst_QTextEdit::shiftEnterShouldInsertLineSeparator()
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

void tst_QTextEdit::selectWordsFromStringsContainingSeparators_data()
{
    QTest::addColumn<QString>("testString");
    QTest::addColumn<QString>("selectedWord");

    const ushort wordSeparators[] =
        {'.', ',', '?', '!', ':', ';', '-', '<', '>', '[', ']', '(', ')', '{', '}',
         '=', '\t', ushort(QChar::Nbsp)};

    for (size_t i = 0, count = sizeof(wordSeparators) / sizeof(wordSeparators[0]); i < count; ++i) {
        const ushort u = wordSeparators[i];
        QByteArray rowName = QByteArrayLiteral("separator: ");
        if (u >= 32 && u < 128)
            rowName += char(u);
        else
            rowName += QByteArrayLiteral("0x") + QByteArray::number(u, 16);
        QTest::newRow(rowName.constData()) << QString("foo") + QChar(u) + QString("bar") << QString("foo");
    }
}

void tst_QTextEdit::selectWordsFromStringsContainingSeparators()
{
    QFETCH(QString, testString);
    QFETCH(QString, selectedWord);
    ed->setText(testString);
    QTextCursor cursor = ed->textCursor();
    cursor.movePosition(QTextCursor::StartOfLine);
    cursor.select(QTextCursor::WordUnderCursor);
    QVERIFY(cursor.hasSelection());
    QCOMPARE(cursor.selection().toPlainText(), selectedWord);
    cursor.clearSelection();
}

#ifndef QT_NO_CLIPBOARD
void tst_QTextEdit::canPaste()
{
    if (!PlatformClipboard::isAvailable())
        QSKIP("Clipboard not working with cron-started unit tests");

    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QApplication::clipboard()->setText(QString());
    QVERIFY(!ed->canPaste());
    QApplication::clipboard()->setText("Test");
    QVERIFY(ed->canPaste());
    ed->setTextInteractionFlags(Qt::NoTextInteraction);
    QVERIFY(!ed->canPaste());
}
#endif

void tst_QTextEdit::ensureCursorVisibleOnInitialShow()
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

class TestEdit : public QTextEdit
{
public:
    TestEdit() : resizeEventCalled(false) {}

    bool resizeEventCalled;

protected:
    virtual void resizeEvent(QResizeEvent *e)
    {
        QTextEdit::resizeEvent(e);
        setHtml("<img src=qtextbrowser-resizeevent.png width=" + QString::number(size().width()) + "><br>Size is " + QString::number(size().width()) + " x " + QString::number(size().height()));
        resizeEventCalled = true;
    }
};

void tst_QTextEdit::setHtmlInsideResizeEvent()
{
    TestEdit edit;
    edit.show();
    edit.resize(800, 600);
    QVERIFY(edit.resizeEventCalled);
}

void tst_QTextEdit::colorfulAppend()
{
    ed->setTextColor(Qt::red);
    ed->append("Red");
    ed->setTextColor(Qt::blue);
    ed->append("Blue");
    ed->setTextColor(Qt::green);
    ed->append("Green");

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

void tst_QTextEdit::ensureVisibleWithRtl()
{
    ed->setLayoutDirection(Qt::RightToLeft);
    ed->setLineWrapMode(QTextEdit::NoWrap);
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

void tst_QTextEdit::preserveCharFormatAfterSetPlainText()
{
    ed->setTextColor(Qt::blue);
    ed->setPlainText("This is blue");
    ed->append("This should still be blue");
    QTextBlock block = ed->document()->begin();
    block = block.next();
    QCOMPARE(block.text(), QString("This should still be blue"));
    QCOMPARE(block.begin().fragment().charFormat().foreground().color(), QColor(Qt::blue));
}

void tst_QTextEdit::extraSelections()
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

void tst_QTextEdit::adjustScrollbars()
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
    ed->setText(txt + txt + txt + txt);

#ifdef Q_OS_WINRT
    QEXPECT_FAIL("", "setMinimum/MaximumSize does not work on WinRT", Abort);
#endif
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

void tst_QTextEdit::currentCharFormatChanged()
{
    QFont ff(ed->font());
    ff.setFamily("Tahoma");
    ff.setPointSize(11);

    SignalReceiver receiver;
    QObject::connect(ed, SIGNAL(currentCharFormatChanged(QTextCharFormat)) , &receiver, SLOT(charFormatChanged(QTextCharFormat)));

    ed->show();
    ed->setCurrentFont(ff);

    QVERIFY(receiver.receivedSignals() > 0);
    QCOMPARE(receiver.charFormat().font(), ff);
}

void tst_QTextEdit::textObscuredByScrollbars()
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

void tst_QTextEdit::setTextPreservesUndoRedoEnabled()
{
    QVERIFY(ed->isUndoRedoEnabled());

    ed->setPlainText("Test");

    QVERIFY(ed->isUndoRedoEnabled());

    ed->setUndoRedoEnabled(false);
    QVERIFY(!ed->isUndoRedoEnabled());
    ed->setPlainText("Test2");
    QVERIFY(!ed->isUndoRedoEnabled());

    ed->setHtml("<p>hello");
    QVERIFY(!ed->isUndoRedoEnabled());
}

void tst_QTextEdit::wordWrapProperty()
{
    {
        QTextEdit edit;
        QTextDocument *doc = new QTextDocument(&edit);
        edit.setDocument(doc);
        edit.setWordWrapMode(QTextOption::NoWrap);
        QCOMPARE(doc->defaultTextOption().wrapMode(), QTextOption::NoWrap);
    }
    {
        QTextEdit edit;
        QTextDocument *doc = new QTextDocument(&edit);
        edit.setWordWrapMode(QTextOption::NoWrap);
        edit.setDocument(doc);
        QCOMPARE(doc->defaultTextOption().wrapMode(), QTextOption::NoWrap);
    }
}

void tst_QTextEdit::lineWrapProperty()
{
    QCOMPARE(ed->wordWrapMode(), QTextOption::WrapAtWordBoundaryOrAnywhere);
    QCOMPARE(ed->lineWrapMode(), QTextEdit::WidgetWidth);
    ed->setLineWrapMode(QTextEdit::NoWrap);
    QCOMPARE(ed->lineWrapMode(), QTextEdit::NoWrap);
    QCOMPARE(ed->wordWrapMode(), QTextOption::WrapAtWordBoundaryOrAnywhere);
    QCOMPARE(ed->document()->defaultTextOption().wrapMode(), QTextOption::NoWrap);
}

void tst_QTextEdit::selectionChanged()
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

#ifndef QT_NO_CLIPBOARD
void tst_QTextEdit::copyPasteBackgroundImage()
{
    if (!PlatformClipboard::isAvailable())
        QSKIP("Native clipboard not working in this setup");

    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QImage foo(16, 16, QImage::Format_ARGB32_Premultiplied);
    foo.save("foo.png");
    ed->setHtml("<body><table><tr><td background=\"foo.png\">Foo</td></tr></table></body>");

    ed->moveCursor(QTextCursor::Start);
    ed->moveCursor(QTextCursor::End, QTextCursor::KeepAnchor);

    ed->copy();

    ed->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
    ed->paste();

    ed->moveCursor(QTextCursor::Start);

    ed->moveCursor(QTextCursor::NextBlock);
    QTextTable *a = ed->textCursor().currentTable();

    ed->moveCursor(QTextCursor::End);
    ed->moveCursor(QTextCursor::PreviousBlock);
    QTextTable *b = ed->textCursor().currentTable();

    QVERIFY(a);
    QVERIFY(b);
    QVERIFY(a != b);

    QBrush ba = a->cellAt(0, 0).format().background();
    QBrush bb = b->cellAt(0, 0).format().background();

#ifdef Q_OS_WINRT
    QEXPECT_FAIL("", "Fails on WinRT - QTBUG-68297", Abort);
#endif
    QCOMPARE(ba.style(), Qt::TexturePattern);
    QCOMPARE(ba.style(), bb.style());

    // we don't want a copy/paste of the background image to cause
    // a new image/pixmap to be created, it should use the cached resource
    // already in the document
    QVERIFY(ba.textureImage().cacheKey() == bb.textureImage().cacheKey() ||
            ba.texture().cacheKey() == bb.texture().cacheKey());
    QFile::remove(QLatin1String("foo.png"));
}

void tst_QTextEdit::copyPasteForegroundImage()
{
    ed->clear();

    QPixmap pix(20, 20);
    pix.fill(Qt::blue);

    QTextCharFormat fmt;
    {
        QBrush textureBrush;
        {
            textureBrush.setTexture(pix);
        }
        textureBrush.setStyle(Qt::TexturePattern);
        fmt.setForeground(textureBrush);
    }
    ed->textCursor().insertText("Foobar", fmt);

    ed->moveCursor(QTextCursor::Start);
    ed->moveCursor(QTextCursor::End, QTextCursor::KeepAnchor);

    ed->copy();
    ed->clear();
    ed->paste();

    QBrush brush = ed->textCursor().charFormat().foreground();
    QCOMPARE(brush.style(), Qt::TexturePattern);
    QCOMPARE(brush.texture().cacheKey(), pix.cacheKey());
}
#endif

void tst_QTextEdit::setText()
{
    QTextEdit browser;
    browser.setText(QLatin1String("hello"));
    QCOMPARE(browser.toPlainText(), QLatin1String("hello"));
    browser.setText(QLatin1String("<b>bold   bold</b>"));
    QCOMPARE(browser.toPlainText(), QLatin1String("bold bold"));
    browser.setText(QLatin1String("with   space"));
    QCOMPARE(browser.toPlainText(), QLatin1String("with   space"));
}

#ifdef QT_BUILD_INTERNAL
QT_BEGIN_NAMESPACE
// qfontdatabase.cpp
Q_AUTOTEST_EXPORT void qt_setQtEnableTestFont(bool value);
QT_END_NAMESPACE
#endif

#ifdef QT_BUILD_INTERNAL
void tst_QTextEdit::fullWidthSelection_data()
{
    QTest::addColumn<int>("cursorFrom");
    QTest::addColumn<int>("cursorTo");
    QTest::addColumn<QString>("imageFileName");

    QTest::newRow("centered fully selected")
        << 0 << 15 << (m_fullWidthSelectionImagesFolder + QStringLiteral("/centered-fully-selected.png"));
    QTest::newRow("centered partly selected")
       << 2 << 15 << (m_fullWidthSelectionImagesFolder + QStringLiteral("/centered-partly-selected.png"));
    QTest::newRow("last char on line")
       << 42 << 44 << (m_fullWidthSelectionImagesFolder + QStringLiteral("/last-char-on-line.png"));
    QTest::newRow("last char on parag")
       << 545 << 548 << (m_fullWidthSelectionImagesFolder + QStringLiteral("/last-char-on-parag.png"));
    QTest::newRow("multiple full width lines")
       << 20 << 60 << (m_fullWidthSelectionImagesFolder + QStringLiteral("/multiple-full-width-lines.png"));
    QTest::newRow("single full width line")
       << 20 << 30 << (m_fullWidthSelectionImagesFolder + QStringLiteral("/single-full-width-line.png"));
}
#endif

#ifdef QT_BUILD_INTERNAL

// With the fix for QTBUG-78318 scaling of documentMargin is added. The testing framework
// forces qt_defaultDpi() to always return 96 DPI. For systems where the actual DPI differs
// (typically 72 DPI) this would now cause scaling of the documentMargin when
// drawing QTextEdit into QImage. In order to avoid the need of multiple reference PNGs
// for comparison we disable the Qt::AA_Use96Dpi attribute for these tests.

struct ForceSystemDpiHelper {
  ForceSystemDpiHelper() { QCoreApplication::setAttribute(Qt::AA_Use96Dpi, false); }
  ~ForceSystemDpiHelper() { QCoreApplication::setAttribute(Qt::AA_Use96Dpi, old); }
  bool old = QCoreApplication::testAttribute(Qt::AA_Use96Dpi);
};

void tst_QTextEdit::fullWidthSelection()
{
    ForceSystemDpiHelper useSystemDpi;

    QFETCH(int, cursorFrom);
    QFETCH(int, cursorTo);
    QFETCH(QString, imageFileName);

    // enable full-width-selection for our test widget.
    class FullWidthStyle : public QCommonStyle {
        int styleHint(StyleHint stylehint, const QStyleOption *opt, const QWidget *widget, QStyleHintReturn *returnData) const {
            if (stylehint == QStyle::SH_RichText_FullWidthSelection)
                return 1;
            return QCommonStyle::styleHint(stylehint, opt, widget, returnData);
        };
    };
    FullWidthStyle myStyle;

    QPalette myPalette = myStyle.standardPalette();
    myPalette.setColor(QPalette::All, QPalette::HighlightedText, QColor(0,0,0,0));
    myPalette.setColor(QPalette::All, QPalette::Highlight, QColor(239,221,85));

    QTextEdit widget;
    widget.document()->setDocumentMargin(2);
    widget.setPalette(myPalette);
    widget.setStyle(&myStyle);
    QTextCursor cursor = widget.textCursor();
    QTextBlockFormat bf1;
    bf1.setAlignment(Qt::AlignCenter);
    cursor.setBlockFormat(bf1);

    // use the test font so we always know where stuff will end up.
    qt_setQtEnableTestFont(true);
    QFont testFont;
    testFont.setFamily("__Qt__Box__Engine__");
    testFont.setPixelSize(11);
    testFont.setWeight(QFont::Normal);
    QTextCharFormat cf;
    cf.setFont(testFont);
    cf.setForeground(QColor(0,0,0,0)); // tricky bit, this :)
    cursor.setCharFormat(cf);

    // populate with some demo text.
    cursor.insertText("centered");
    QTextBlockFormat bf;
    cursor.insertBlock(bf, cf);
    cursor.insertText("Duis autem vel eum iriure dolor in hendrerit in vulputate velit esse molestie consequat, vel illum dolore eu feugiat nulla facilisis at vero eros et accumsan et iusto odio dignissim qui blandit praesent luptatum zzril delenit augue duis dolore te feugait nulla facilisi. Lorem ipsum dolor sit amet, consectetuer adipiscing elit, sed diam nonummy nibh euismod tincidunt ut laoreet dolore magna aliquam erat volutpat. Ut wisi enim ad minim veniam, quis nostrud exerci tation ullamcorper suscipit lobortis nisl ut aliquip ex ea commodo cons.\nfoo\n");

    // Usecase 1 for full-width-selections; the 'show-cursor-position' one.
    QList<QTextEdit::ExtraSelection> selections;
    QTextCursor cursor2(widget.document());
    QTextEdit::ExtraSelection es;
    cursor2.setPosition(300);
    es.cursor = cursor2;
    es.format.setProperty( QTextFormat::FullWidthSelection, true );
    es.format.setBackground(QColor(255, 0, 0));
    selections.append(es);
    widget.setExtraSelections(selections);

    // Usecase 2; show it based on the style.
    // Select part of the centered text and part of the next; which means we should show the part right of the centered text.
    cursor.setPosition(cursorFrom);
    cursor.setPosition(cursorTo, QTextCursor::KeepAnchor);
    widget.setTextCursor(cursor);

    compareWidgetAndImage(widget, imageFileName);
}
#endif

#ifdef QT_BUILD_INTERNAL
void tst_QTextEdit::fullWidthSelection2()
{
    ForceSystemDpiHelper useSystemDpi;

    QPalette myPalette;
    myPalette.setColor(QPalette::All, QPalette::HighlightedText, QColor(0,0,0,0));
    myPalette.setColor(QPalette::All, QPalette::Highlight, QColor(239,221,85));
    myPalette.setColor(QPalette::All, QPalette::Base, QColor(255,255,255));

    QTextEdit widget;
    widget.setPalette(myPalette);
    widget.setLineWrapMode(QTextEdit::NoWrap);
    QTextCursor cursor = widget.textCursor();

    // use the test font so we always know where stuff will end up.
    qt_setQtEnableTestFont(true);
    QFont testFont;
    testFont.setFamily("__Qt__Box__Engine__");
    testFont.setPixelSize(11);
    testFont.setWeight(QFont::Normal);
    QTextCharFormat cf;
    cf.setFont(testFont);
    cf.setForeground(QColor(0,0,0,0)); // tricky bit, this :)
    cursor.setCharFormat(cf);
    cursor.insertText("foo");

    QList<QTextEdit::ExtraSelection> selections;
    QTextCursor cursor2(widget.document());
    QTextEdit::ExtraSelection es;
    cursor2.setPosition(1);
    es.cursor = cursor2;
    es.format.setProperty( QTextFormat::FullWidthSelection, true );
    es.format.setBackground(QColor(255, 0, 0));
    selections.append(es);
    widget.setExtraSelections(selections);

    compareWidgetAndImage(widget, m_fullWidthSelectionImagesFolder + "/nowrap_long.png");
}
#endif

void tst_QTextEdit::compareWidgetAndImage(QTextEdit &widget, const QString &imageFileName)
{
    widget.setGeometry(0, 0, 300, 390);
    widget.setFrameShape(QFrame::NoFrame);

    QImage image(300, 390, QImage::Format_RGB32);
    QPainter painter(&image);
    widget.viewport()->render(&painter);
    painter.end();
    QImageReader reader(imageFileName, "PNG");

    QImage original = reader.read();

    QVERIFY2(!original.isNull(),
             qPrintable(QString::fromLatin1("Unable to read image %1: %2").arg(imageFileName, reader.errorString())));
    QCOMPARE(original.size(), image.size());
    QCOMPARE(image.depth(), 32);
    QCOMPARE(original.depth(), image.depth());

    const int bytesPerLine = image.bytesPerLine();
    const int width = image.width();
    const int height = image.height();

    for (int y = 0; y < height; ++y) {
        const QRgb * const firstLine = reinterpret_cast<const QRgb *>(image.scanLine(y));
        const QRgb * const secondLine = reinterpret_cast<const QRgb *>(original.scanLine(y));

        if (memcmp(firstLine, secondLine, bytesPerLine) != 0) {
            for (int x = 0; x < width; ++x) {
                const QRgb a = firstLine[x];
                const QRgb b = secondLine[x];
                const bool same = qAbs(qRed(a) - qRed(b)) <= 20
                    && qAbs(qGreen(a) - qGreen(b)) <= 20
                    && qAbs(qBlue(a) - qBlue(b)) <= 20;
                if (!same) {
                    QString fileName = imageFileName;
                    QImageWriter writer("failed_"+ fileName.replace('/','_'), "PNG");
                    writer.write(image);
                }
                QVERIFY(same);
            }
        }
    }
}

void tst_QTextEdit::cursorRect()
{
    ed->show();
    ed->setPlainText("Hello Test World");
    ed->setCursorWidth(1);
    QCOMPARE(ed->cursorRect().width(), 1);
    ed->setCursorWidth(2);
    QCOMPARE(ed->cursorRect().width(), 2);
    ed->setCursorWidth(4);
    QCOMPARE(ed->cursorRect().width(), 4);
    ed->setCursorWidth(10);
    QCOMPARE(ed->cursorRect().width(), 10);
}

#ifdef QT_BUILD_INTERNAL
void tst_QTextEdit::setDocumentPreservesPalette()
{
    QWidgetTextControl *control = ed->findChild<QWidgetTextControl *>();
    QVERIFY(control);

    QPalette defaultPal = ed->palette();
    QPalette whitePal = ed->palette();
    whitePal.setColor(QPalette::Active, QPalette::Text, "white");


    QVERIFY(whitePal != ed->palette());
    ed->setPalette(whitePal);
    QVERIFY(whitePal.color(QPalette::Active, QPalette::Text)
            == ed->palette().color(QPalette::Active, QPalette::Text));
    QVERIFY(whitePal.color(QPalette::Active, QPalette::Text)
            == control->palette().color(QPalette::Active, QPalette::Text));

    QTextDocument *newDoc = new QTextDocument(ed);
    ed->setDocument(newDoc);
    QCOMPARE(control->document(), newDoc);
    QVERIFY(whitePal.color(QPalette::Active, QPalette::Text)
            == control->palette().color(QPalette::Active, QPalette::Text));
}
#endif

class PublicTextEdit : public QTextEdit
{
public:
    void publicInsertFromMimeData(const QMimeData *source)
    { insertFromMimeData(source); }
};

void tst_QTextEdit::pasteFromQt3RichText()
{
    QByteArray richtext("<!--StartFragment--><p>  QTextEdit is an  ");

    QMimeData mimeData;
    mimeData.setData("application/x-qrichtext", richtext);

    static_cast<PublicTextEdit *>(ed)->publicInsertFromMimeData(&mimeData);

    QCOMPARE(ed->toPlainText(), QString::fromLatin1("  QTextEdit is an  "));
    ed->clear();

    richtext = "<!--StartFragment-->  QTextEdit is an  ";
    mimeData.setData("application/x-qrichtext", richtext);

    static_cast<PublicTextEdit *>(ed)->publicInsertFromMimeData(&mimeData);

    QCOMPARE(ed->toPlainText(), QString::fromLatin1("  QTextEdit is an  "));
}

void tst_QTextEdit::noWrapBackgrounds()
{
    QWidget topLevel;
    QVBoxLayout *layout = new QVBoxLayout(&topLevel);

    QTextEdit edit;
    edit.setLineWrapMode(QTextEdit::NoWrap);

    // hide the cursor in order to make the image comparison below reliable
    edit.setCursorWidth(0);

    QTextFrame *root = edit.document()->rootFrame();
    QTextFrameFormat frameFormat = root->frameFormat();
    frameFormat.setLeftMargin(2);
    frameFormat.setRightMargin(2);
    root->setFrameFormat(frameFormat);

    QTextBlockFormat format;
    format.setBackground(Qt::red);
    edit.textCursor().setBlockFormat(format);
    edit.insertPlainText(QLatin1String(" \n  \n   \n    \n"));
    edit.setFixedSize(100, 200);

    layout->addWidget(&edit);
    topLevel.show();

    const QImage img = edit.viewport()->grab().toImage();
    QCOMPARE(img, img.mirrored(true, false));
}

void tst_QTextEdit::preserveCharFormatAfterUnchangingSetPosition()
{
    QColor color(Qt::yellow);
    QTextEdit edit;
    edit.setTextColor(color);

    QTextCursor c = edit.textCursor();
    c.setPosition(c.position());
    edit.setTextCursor(c);

    QCOMPARE(edit.textColor(), color);
}

// Regression test for QTBUG-4696
void tst_QTextEdit::twoSameInputMethodEvents()
{
    ed->setText("testLine");
    ed->show();
    QList<QInputMethodEvent::Attribute> attributes;
    attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Cursor,
                                                   ed->textCursor().position(),
                                                   0,
                                                   QVariant()));

    QInputMethodEvent event("PreEditText", attributes);
    QApplication::sendEvent(ed, &event);
    QTRY_COMPARE(ed->document()->firstBlock().layout()->lineCount(), 1);
    QApplication::sendEvent(ed, &event);
    QCOMPARE(ed->document()->firstBlock().layout()->lineCount(), 1);
}

#ifndef QT_NO_CONTEXTMENU
void tst_QTextEdit::taskQTBUG_7902_contextMenuCrash()
{
    QTextEdit *w = new QTextEdit;
    w->show();
    QVERIFY(QTest::qWaitForWindowExposed(w));

    QTimer ti;
    w->connect(&ti, SIGNAL(timeout()), w, SLOT(deleteLater()));
    ti.start(200);

    QContextMenuEvent *cme = new QContextMenuEvent(QContextMenuEvent::Mouse, w->rect().center());
    qApp->postEvent(w->viewport(), cme);

    QTest::qWait(300);
    // No crash, it's allright.
}
#endif

void tst_QTextEdit::bidiVisualMovement_data()
{
    QTest::addColumn<QString>("logical");
    QTest::addColumn<int>("basicDir");
    QTest::addColumn<QList<int> >("positionList");

    QTest::newRow("Latin text")
        << QString::fromUtf8("abc")
        << (int) QChar::DirL
        << (QList<int>() << 0 << 1 << 2 << 3);
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

void tst_QTextEdit::bidiVisualMovement()
{
    QFETCH(QString,      logical);
    QFETCH(int,          basicDir);
    QFETCH(QList<int>,   positionList);

    ed->setText(logical);

    QTextOption option = ed->document()->defaultTextOption();
    option.setTextDirection(basicDir == QChar::DirL ? Qt::LeftToRight : Qt::RightToLeft);
    ed->document()->setDefaultTextOption(option);

    ed->document()->setDefaultCursorMoveStyle(Qt::VisualMoveStyle);
    ed->moveCursor(QTextCursor::Start);
    ed->show();

    bool moved;
    int i = 0, oldPos, newPos = 0;

    do {
        oldPos = newPos;
        QCOMPARE(oldPos, positionList[i]);
        if (basicDir == QChar::DirL) {
            ed->moveCursor(QTextCursor::Right);
        } else
        {
            ed->moveCursor(QTextCursor::Left);
        }
        newPos = ed->textCursor().position();
        moved = (oldPos != newPos);
        i++;
    } while (moved);

    QCOMPARE(i, positionList.size());

    do {
        i--;
        oldPos = newPos;
        QCOMPARE(oldPos, positionList[i]);
        if (basicDir == QChar::DirL) {
            ed->moveCursor(QTextCursor::Left);
        } else
        {
            ed->moveCursor(QTextCursor::Right);
        }
        newPos = ed->textCursor().position();
        moved = (oldPos != newPos);
    } while (moved && i >= 0);
}

void tst_QTextEdit::bidiLogicalMovement_data()
{
    bidiVisualMovement_data();
}

void tst_QTextEdit::bidiLogicalMovement()
{
    QFETCH(QString,      logical);
    QFETCH(int,          basicDir);
    QFETCH(QList<int>,   positionList);

    ed->setText(logical);

    QTextOption option = ed->document()->defaultTextOption();
    option.setTextDirection(basicDir == QChar::DirL ? Qt::LeftToRight : Qt::RightToLeft);
    ed->document()->setDefaultTextOption(option);

    ed->document()->setDefaultCursorMoveStyle(Qt::LogicalMoveStyle);
    ed->moveCursor(QTextCursor::Start);
    ed->show();

    bool moved;
    int i = 0, oldPos, newPos = 0;

    do {
        oldPos = newPos;
        QCOMPARE(oldPos, i);
        if (basicDir == QChar::DirL) {
            ed->moveCursor(QTextCursor::Right);
        } else
        {
            ed->moveCursor(QTextCursor::Left);
        }
        newPos = ed->textCursor().position();
        moved = (oldPos != newPos);
        i++;
    } while (moved);

    QCOMPARE(i, positionList.size());

    do {
        i--;
        oldPos = newPos;
        QCOMPARE(oldPos, i);
        if (basicDir == QChar::DirL) {
            ed->moveCursor(QTextCursor::Left);
        } else
        {
            ed->moveCursor(QTextCursor::Right);
        }
        newPos = ed->textCursor().position();
        moved = (oldPos != newPos);
    } while (moved && i >= 0);
}

void tst_QTextEdit::inputMethodEvent()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    ed->show();

    // test that text change with an input method event triggers change signal
    QSignalSpy spy(ed, SIGNAL(textChanged()));

    QInputMethodEvent event;
    event.setCommitString("text");
    QApplication::sendEvent(ed, &event);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(ed->toPlainText(), QString("text"));

    // test that input method gets chance to commit preedit when removing focus
    ed->setText("");
    QApplication::setActiveWindow(ed);
    QTRY_VERIFY(QApplication::focusWindow());
    QCOMPARE(qApp->focusObject(), ed);

    m_platformInputContext.setCommitString("text");
    m_platformInputContext.m_commitCallCount = 0;
    QList<QInputMethodEvent::Attribute> attributes;
    QInputMethodEvent preeditEvent("preedit text", attributes);
    QApplication::sendEvent(ed, &preeditEvent);

    ed->clearFocus();
    QCOMPARE(m_platformInputContext.m_commitCallCount, 1);
    QCOMPARE(ed->toPlainText(), QString("text"));
}

void tst_QTextEdit::inputMethodSelection()
{
    ed->setText("Lorem ipsum dolor sit amet, consectetur adipiscing elit.");

    QSignalSpy selectionSpy(ed, SIGNAL(selectionChanged()));
    QTextCursor cursor = ed->textCursor();
    cursor.setPosition(0);
    cursor.setPosition(5, QTextCursor::KeepAnchor);
    ed->setTextCursor(cursor);

    QCOMPARE(selectionSpy.count(), 1);
    QCOMPARE(ed->textCursor().selectionStart(), 0);
    QCOMPARE(ed->textCursor().selectionEnd(), 5);

    QList<QInputMethodEvent::Attribute> attributes;
    attributes << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, 12, 5, QVariant());
    QInputMethodEvent event("", attributes);
    QApplication::sendEvent(ed, &event);

    QCOMPARE(selectionSpy.count(), 2);
    QCOMPARE(ed->textCursor().selectionStart(), 12);
    QCOMPARE(ed->textCursor().selectionEnd(), 17);
}

void tst_QTextEdit::inputMethodQuery()
{
    QString text("first line of text\nsecond line of text");
    ed->setText(text);
    ed->selectAll();

    QInputMethodQueryEvent event(Qt::ImQueryInput | Qt::ImEnabled);
    QGuiApplication::sendEvent(ed, &event);
    int anchor = event.value(Qt::ImAnchorPosition).toInt();
    int position = event.value(Qt::ImCursorPosition).toInt();
    QCOMPARE(qAbs(position - anchor), text.length());
    QCOMPARE(event.value(Qt::ImEnabled).toBool(), true);

    ed->setEnabled(false);
    QGuiApplication::sendEvent(ed, &event);
    QCOMPARE(event.value(Qt::ImEnabled).toBool(), false);
}

Q_DECLARE_METATYPE(Qt::InputMethodHints)
void tst_QTextEdit::inputMethodQueryImHints_data()
{
    QTest::addColumn<Qt::InputMethodHints>("hints");

    QTest::newRow("None") << static_cast<Qt::InputMethodHints>(Qt::ImhNone);
    QTest::newRow("Password") << static_cast<Qt::InputMethodHints>(Qt::ImhHiddenText);
    QTest::newRow("Normal") << static_cast<Qt::InputMethodHints>(Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText | Qt::ImhSensitiveData);
}

void tst_QTextEdit::inputMethodQueryImHints()
{
    QFETCH(Qt::InputMethodHints, hints);
    ed->setInputMethodHints(hints);

    QVariant value = ed->inputMethodQuery(Qt::ImHints);
    QCOMPARE(static_cast<Qt::InputMethodHints>(value.toInt()), hints);
}

// QTBUG-51923: Verify that the cursor rectangle returned by the input
// method query correctly reflects the viewport offset.
void tst_QTextEdit::inputMethodCursorRect()
{
    ed->setPlainText("Line1\nLine2Line3\nLine3");
    ed->moveCursor(QTextCursor::End);
    const QRectF cursorRect = ed->cursorRect();
    const QVariant cursorRectV = ed->inputMethodQuery(Qt::ImCursorRectangle);
    QCOMPARE(cursorRectV.type(), QVariant::RectF);
    QCOMPARE(cursorRectV.toRect(), cursorRect.toRect());
}

void tst_QTextEdit::highlightLongLine()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QTextEdit edit;
    edit.setAcceptRichText(false);
    edit.setWordWrapMode(QTextOption::NoWrap);

    QString singeLongLine;
    for (int i = 0; i < 10000; ++i)
        singeLongLine += "0123456789";
    edit.setPlainText(singeLongLine);

    class NumHighlighter : public QSyntaxHighlighter {
    public:
        explicit NumHighlighter(QTextDocument*doc) : QSyntaxHighlighter(doc) {};
        virtual void highlightBlock(const QString& text) {
            // odd number in bold
            QTextCharFormat format;
            format.setFontWeight(QFont::Bold);
            for (int i = 0; i < text.size(); ++i) {
                if (text.at(i).unicode() % 2)
                    setFormat(i, 1, format);
            }
        }
    };
    NumHighlighter nh(edit.document());
    edit.show();
    QVERIFY(QTest::qWaitForWindowActive(edit.windowHandle()));
    QCoreApplication::processEvents();
    //If there is a quadratic behaviour, this would take forever.
    QVERIFY(true);
}

//check for bug 15003, are there multiple textChanged() signals on remove?
void tst_QTextEdit::countTextChangedOnRemove()
{
    QTextEdit edit;
    edit.insertPlainText("Hello");

    QSignalSpy spy(&edit, SIGNAL(textChanged()));

    QKeyEvent event(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier);
    QCoreApplication::instance()->notify(&edit, &event);

    QCOMPARE(spy.count(), 1);
}

#ifndef QT_NO_REGEXP
void tst_QTextEdit::findWithRegExp()
{
    ed->setHtml(QStringLiteral("arbitrary te<span style=\"color:#ff0000\">xt</span>"));
    QRegExp rx("\\w{2}xt");

    bool found = ed->find(rx);

    QVERIFY(found);
    QCOMPARE(ed->textCursor().selectedText(), QStringLiteral("text"));
}

void tst_QTextEdit::findBackwardWithRegExp()
{
    ed->setPlainText(QStringLiteral("arbitrary text"));
    QTextCursor cursor = ed->textCursor();
    cursor.movePosition(QTextCursor::End);
    ed->setTextCursor(cursor);
    QRegExp rx("a\\w*t");

    bool found = ed->find(rx, QTextDocument::FindBackward);

    QVERIFY(found);
    QCOMPARE(ed->textCursor().selectedText(), QStringLiteral("arbit"));
}

void tst_QTextEdit::findWithRegExpReturnsFalseIfNoMoreResults()
{
    ed->setPlainText(QStringLiteral("arbitrary text"));
    QRegExp rx("t.xt");
    ed->find(rx);

    bool found = ed->find(rx);

    QVERIFY(!found);
    QCOMPARE(ed->textCursor().selectedText(), QStringLiteral("text"));
}
#endif

#if QT_CONFIG(regularexpression)
void tst_QTextEdit::findWithRegularExpression()
{
    ed->setHtml(QStringLiteral("arbitrary te<span style=\"color:#ff0000\">xt</span>"));
    QRegularExpression rx("\\w{2}xt");

    bool found = ed->find(rx);

    QVERIFY(found);
    QCOMPARE(ed->textCursor().selectedText(), QStringLiteral("text"));
}

void tst_QTextEdit::findBackwardWithRegularExpression()
{
    ed->setPlainText(QStringLiteral("arbitrary text"));
    QTextCursor cursor = ed->textCursor();
    cursor.movePosition(QTextCursor::End);
    ed->setTextCursor(cursor);
    QRegularExpression rx("a\\w*t");

    bool found = ed->find(rx, QTextDocument::FindBackward);

    QVERIFY(found);
    QCOMPARE(ed->textCursor().selectedText(), QStringLiteral("arbit"));
}

void tst_QTextEdit::findWithRegularExpressionReturnsFalseIfNoMoreResults()
{
    ed->setPlainText(QStringLiteral("arbitrary text"));
    QRegularExpression rx("t.xt");
    ed->find(rx);

    bool found = ed->find(rx);

    QVERIFY(!found);
    QCOMPARE(ed->textCursor().selectedText(), QStringLiteral("text"));
}
#endif

#if QT_CONFIG(wheelevent)

class TextEdit : public QTextEdit
{
public:
    TextEdit(QWidget *parent = 0)
        : QTextEdit(parent)
    {}
    void wheelEvent(QWheelEvent *event)
    {
        QTextEdit::wheelEvent(event);
    }
};

void tst_QTextEdit::wheelEvent()
{
    TextEdit ed(0);
    ed.setPlainText(QStringLiteral("Line\nLine\nLine\n"));
    ed.setReadOnly(true);

    float defaultFontSize = ed.font().pointSizeF();
    QWheelEvent wheelUp(QPointF(), QPointF(), QPoint(), QPoint(0, 120),
                        Qt::NoButton, Qt::ControlModifier, Qt::NoScrollPhase, Qt::MouseEventNotSynthesized);
    ed.wheelEvent(&wheelUp);

    QCOMPARE(defaultFontSize + 1, ed.font().pointSizeF());

    QWheelEvent wheelHalfDown(QPointF(), QPointF(), QPoint(), QPoint(0, -60),
                              Qt::NoButton, Qt::ControlModifier, Qt::NoScrollPhase, Qt::MouseEventNotSynthesized);
    ed.wheelEvent(&wheelHalfDown);

    QCOMPARE(defaultFontSize + 0.5, ed.font().pointSizeF());
}

#endif

namespace {
    class MyPaintEngine : public QPaintEngine
    {
    public:
        bool begin(QPaintDevice *) override { return true; }

        bool end() override { return true; }

        void updateState(const QPaintEngineState &) override  { }

        void drawPixmap(const QRectF &, const QPixmap &, const QRectF &) override { }

        void drawTextItem(const QPointF &, const QTextItem &textItem) override
        {
            itemFonts.append(qMakePair(textItem.text(), textItem.font()));
        }

        Type type() const override { return User; }


        QList<QPair<QString, QFont> > itemFonts;
    };

    class MyPaintDevice : public QPaintDevice
    {
    public:
        MyPaintDevice() : m_paintEngine(new MyPaintEngine)
        {
        }


        QPaintEngine *paintEngine () const
        {
            return m_paintEngine;
        }

        int metric (QPaintDevice::PaintDeviceMetric metric) const {
            switch (metric) {
            case QPaintDevice::PdmWidth:
            case QPaintDevice::PdmHeight:
            case QPaintDevice::PdmWidthMM:
            case QPaintDevice::PdmHeightMM:
            case QPaintDevice::PdmNumColors:
                return INT_MAX;
            case QPaintDevice::PdmDepth:
                return 32;
            case QPaintDevice::PdmDpiX:
            case QPaintDevice::PdmDpiY:
            case QPaintDevice::PdmPhysicalDpiX:
            case QPaintDevice::PdmPhysicalDpiY:
                return 72;
            case QPaintDevice::PdmDevicePixelRatio:
            case QPaintDevice::PdmDevicePixelRatioScaled:
                ; // fall through
            }
            return 0;
        }

        MyPaintEngine *m_paintEngine;
    };
}

void tst_QTextEdit::preeditCharFormat_data()
{
    QTest::addColumn<QList<QInputMethodEvent::Attribute> >("imeAttributes");
    QTest::addColumn<QStringList>("substrings");
    QTest::addColumn<QList<bool> >("boldnessList");
    QTest::addColumn<QList<bool> >("italicnessList");
    QTest::addColumn<QList<int> >("pointSizeList");

    {
        QList<QInputMethodEvent::Attribute> attributes;
        {
            QTextCharFormat tcf;
            tcf.setFontPointSize(13);
            tcf.setFontItalic(true);
            attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat, 1, 1, tcf));
        }

        {
            QTextCharFormat tcf;
            tcf.setFontPointSize(8);
            tcf.setFontWeight(QFont::Normal);
            attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat, 4, 2, tcf));
        }

        QTest::newRow("Two formats, middle, in order")
            << attributes
            << (QStringList() << "P" << "r" << "eE" << "di" << "tText")
            << (QList<bool>() << true << true << true << false << true)
            << (QList<bool>() << false << true << false << false << false)
            << (QList<int>() << 20 << 13 << 20 << 8 << 20);
    }

    {
        QList<QInputMethodEvent::Attribute> attributes;
        {
            QTextCharFormat tcf;
            tcf.setFontPointSize(8);
            tcf.setFontWeight(QFont::Normal);
            attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat, 4, 2, tcf));
        }

        {
            QTextCharFormat tcf;
            tcf.setFontPointSize(13);
            tcf.setFontItalic(true);
            attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat, 1, 1, tcf));
        }

        QTest::newRow("Two formats, middle, out of order")
            << attributes
            << (QStringList() << "P" << "r" << "eE" << "di" << "tText")
            << (QList<bool>() << true << true << true << false << true)
            << (QList<bool>() << false << true << false << false << false)
            << (QList<int>() << 20 << 13 << 20 << 8 << 20);
    }

    {
        QList<QInputMethodEvent::Attribute> attributes;
        {
            QTextCharFormat tcf;
            tcf.setFontPointSize(13);
            tcf.setFontItalic(true);
            attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat, 0, 1, tcf));
        }

        {
            QTextCharFormat tcf;
            tcf.setFontPointSize(8);
            tcf.setFontWeight(QFont::Normal);
            attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat, 4, 2, tcf));
        }

        QTest::newRow("Two formats, front, in order")
            << attributes
            << (QStringList() << "P" << "reE" << "di" << "tText")
            << (QList<bool>() << true << true << false << true)
            << (QList<bool>() << true << false << false << false)
            << (QList<int>() << 13 << 20 << 8 << 20);
    }

    {
        QList<QInputMethodEvent::Attribute> attributes;
        {
            QTextCharFormat tcf;
            tcf.setFontPointSize(8);
            tcf.setFontWeight(QFont::Normal);
            attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat, 4, 2, tcf));
        }

        {
            QTextCharFormat tcf;
            tcf.setFontPointSize(13);
            tcf.setFontItalic(true);
            attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat, 0, 1, tcf));
        }

        QTest::newRow("Two formats, front, out of order")
            << attributes
            << (QStringList() << "P" << "reE" << "di" << "tText")
            << (QList<bool>() << true << true << false << true)
            << (QList<bool>() << true << false << false << false)
            << (QList<int>() << 13 << 20 << 8 << 20);
    }
}

void tst_QTextEdit::preeditCharFormat()
{
    QFETCH(QList<QInputMethodEvent::Attribute>, imeAttributes);
    QFETCH(QStringList, substrings);
    QFETCH(QList<bool>, boldnessList);
    QFETCH(QList<bool>, italicnessList);
    QFETCH(QList<int>, pointSizeList);

    QTextEdit *w = new QTextEdit;
    w->show();
    QVERIFY(QTest::qWaitForWindowExposed(w));

    // Set main char format
    {
        QTextCharFormat tcf;
        tcf.setFontPointSize(20);
        tcf.setFontWeight(QFont::Bold);
        w->mergeCurrentCharFormat(tcf);
    }

    QList<QInputMethodEvent::Attribute> attributes;
    attributes.prepend(QInputMethodEvent::Attribute(QInputMethodEvent::Cursor,
                                                   w->textCursor().position(),
                                                   0,
                                                   QVariant()));

    attributes += imeAttributes;

    QInputMethodEvent event("PreEditText", attributes);
    QApplication::sendEvent(w, &event);

    MyPaintDevice device;
    {
        QPainter p(&device);
        w->document()->drawContents(&p);
    }

    QCOMPARE(device.m_paintEngine->itemFonts.size(), substrings.size());
    for (int i = 0; i < substrings.size(); ++i)
        QCOMPARE(device.m_paintEngine->itemFonts.at(i).first, substrings.at(i));

    for (int i = 0; i < substrings.size(); ++i)
        QCOMPARE(device.m_paintEngine->itemFonts.at(i).second.bold(), boldnessList.at(i));

    for (int i = 0; i < substrings.size(); ++i)
        QCOMPARE(device.m_paintEngine->itemFonts.at(i).second.italic(), italicnessList.at(i));

    for (int i = 0; i < substrings.size(); ++i)
        QCOMPARE(device.m_paintEngine->itemFonts.at(i).second.pointSize(), pointSizeList.at(i));

    delete w;
}

QTEST_MAIN(tst_QTextEdit)
#include "tst_qtextedit.moc"
