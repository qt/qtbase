/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qaccessiblewidgets_p.h"
#include "qabstracttextdocumentlayout.h"
#include "qapplication.h"
#include "qclipboard.h"
#include "qtextedit.h"
#include "private/qtextedit_p.h"
#include "qtextdocument.h"
#include "qtextobject.h"
#include "qplaintextedit.h"
#include "qtextboundaryfinder.h"
#include "qscrollbar.h"
#include "qdebug.h"
#include <QApplication>
#include <QStackedWidget>
#include <QToolBox>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QDialogButtonBox>
#include <limits.h>
#include <QRubberBand>
#include <QTextBrowser>
#include <QCalendarWidget>
#include <QAbstractItemView>
#include <QDockWidget>
#include <QMainWindow>
#include <QAbstractButton>
#include <private/qdockwidget_p.h>
#include <QFocusFrame>

#ifndef QT_NO_ACCESSIBILITY

QT_BEGIN_NAMESPACE

QString qt_accStripAmp(const QString &text);
QString qt_accHotKey(const QString &text);

QList<QWidget*> childWidgets(const QWidget *widget)
{
    if (widget == 0)
        return QList<QWidget*>();
    QList<QWidget*> widgets;
    for (QObject *o : widget->children()) {
        QWidget *w = qobject_cast<QWidget *>(o);
        if (!w)
            continue;
        QString objectName = w->objectName();
        if (!w->isWindow()
              && !qobject_cast<QFocusFrame*>(w)
              && !qobject_cast<QMenu*>(w)
              && objectName != QLatin1String("qt_rubberband")
              && objectName != QLatin1String("qt_qmainwindow_extended_splitter")) {
            widgets.append(w);
        }
    }
    return widgets;
}

#if !defined(QT_NO_TEXTEDIT) && !defined(QT_NO_CURSOR)

QAccessiblePlainTextEdit::QAccessiblePlainTextEdit(QWidget* o)
  :QAccessibleTextWidget(o)
{
    Q_ASSERT(widget()->inherits("QPlainTextEdit"));
}

QPlainTextEdit* QAccessiblePlainTextEdit::plainTextEdit() const
{
    return static_cast<QPlainTextEdit *>(widget());
}

QString QAccessiblePlainTextEdit::text(QAccessible::Text t) const
{
    if (t == QAccessible::Value)
        return plainTextEdit()->toPlainText();

    return QAccessibleWidget::text(t);
}

void QAccessiblePlainTextEdit::setText(QAccessible::Text t, const QString &text)
{
    if (t != QAccessible::Value) {
        QAccessibleWidget::setText(t, text);
        return;
    }
    if (plainTextEdit()->isReadOnly())
        return;

    plainTextEdit()->setPlainText(text);
}

QAccessible::State QAccessiblePlainTextEdit::state() const
{
    QAccessible::State st = QAccessibleTextWidget::state();
    if (plainTextEdit()->isReadOnly())
        st.readOnly = true;
    else
        st.editable = true;
    return st;
}

void *QAccessiblePlainTextEdit::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::TextInterface)
        return static_cast<QAccessibleTextInterface*>(this);
    else if (t == QAccessible::EditableTextInterface)
        return static_cast<QAccessibleEditableTextInterface*>(this);
    return QAccessibleWidget::interface_cast(t);
}

QPoint QAccessiblePlainTextEdit::scrollBarPosition() const
{
    QPoint result;
    result.setX(plainTextEdit()->horizontalScrollBar() ? plainTextEdit()->horizontalScrollBar()->sliderPosition() : 0);
    result.setY(plainTextEdit()->verticalScrollBar() ? plainTextEdit()->verticalScrollBar()->sliderPosition() : 0);
    return result;
}

QTextCursor QAccessiblePlainTextEdit::textCursor() const
{
    return plainTextEdit()->textCursor();
}

void QAccessiblePlainTextEdit::setTextCursor(const QTextCursor &textCursor)
{
    plainTextEdit()->setTextCursor(textCursor);
}

QTextDocument* QAccessiblePlainTextEdit::textDocument() const
{
    return plainTextEdit()->document();
}

QWidget* QAccessiblePlainTextEdit::viewport() const
{
    return plainTextEdit()->viewport();
}

void QAccessiblePlainTextEdit::scrollToSubstring(int startIndex, int endIndex)
{
    //TODO: Not implemented
    Q_UNUSED(startIndex);
    Q_UNUSED(endIndex);
}


/*!
  \class QAccessibleTextEdit
  \brief The QAccessibleTextEdit class implements the QAccessibleInterface for richtext editors.
  \internal
*/

/*!
  \fn QAccessibleTextEdit::QAccessibleTextEdit(QWidget *widget)

  Constructs a QAccessibleTextEdit object for a \a widget.
*/
QAccessibleTextEdit::QAccessibleTextEdit(QWidget *o)
: QAccessibleTextWidget(o, QAccessible::EditableText)
{
    Q_ASSERT(widget()->inherits("QTextEdit"));
}

/*! Returns the text edit. */
QTextEdit *QAccessibleTextEdit::textEdit() const
{
    return static_cast<QTextEdit *>(widget());
}

QTextCursor QAccessibleTextEdit::textCursor() const
{
    return textEdit()->textCursor();
}

QTextDocument *QAccessibleTextEdit::textDocument() const
{
    return textEdit()->document();
}

void QAccessibleTextEdit::setTextCursor(const QTextCursor &textCursor)
{
    textEdit()->setTextCursor(textCursor);
}

QWidget *QAccessibleTextEdit::viewport() const
{
    return textEdit()->viewport();
}

QPoint QAccessibleTextEdit::scrollBarPosition() const
{
    QPoint result;
    result.setX(textEdit()->horizontalScrollBar() ? textEdit()->horizontalScrollBar()->sliderPosition() : 0);
    result.setY(textEdit()->verticalScrollBar() ? textEdit()->verticalScrollBar()->sliderPosition() : 0);
    return result;
}

QString QAccessibleTextEdit::text(QAccessible::Text t) const
{
    if (t == QAccessible::Value)
        return textEdit()->toPlainText();

    return QAccessibleWidget::text(t);
}

void QAccessibleTextEdit::setText(QAccessible::Text t, const QString &text)
{
    if (t != QAccessible::Value) {
        QAccessibleWidget::setText(t, text);
        return;
    }
    if (textEdit()->isReadOnly())
        return;

    textEdit()->setText(text);
}

QAccessible::State QAccessibleTextEdit::state() const
{
    QAccessible::State st = QAccessibleTextWidget::state();
    if (textEdit()->isReadOnly())
        st.readOnly = true;
    else
        st.editable = true;
    return st;
}

void *QAccessibleTextEdit::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::TextInterface)
        return static_cast<QAccessibleTextInterface*>(this);
    else if (t == QAccessible::EditableTextInterface)
        return static_cast<QAccessibleEditableTextInterface*>(this);
    return QAccessibleWidget::interface_cast(t);
}

void QAccessibleTextEdit::scrollToSubstring(int startIndex, int endIndex)
{
    QTextEdit *edit = textEdit();

    QTextCursor cursor = textCursor();
    cursor.setPosition(startIndex);
    QRect r = edit->cursorRect(cursor);

    cursor.setPosition(endIndex);
    r.setBottomRight(edit->cursorRect(cursor).bottomRight());

    r.moveTo(r.x() + edit->horizontalScrollBar()->value(),
             r.y() + edit->verticalScrollBar()->value());

    // E V I L, but ensureVisible is not public
    if (Q_UNLIKELY(!QMetaObject::invokeMethod(edit, "_q_ensureVisible", Q_ARG(QRectF, r))))
        qWarning("AccessibleTextEdit::scrollToSubstring failed!");
}

#endif // QT_NO_TEXTEDIT && QT_NO_CURSOR

#ifndef QT_NO_STACKEDWIDGET
// ======================= QAccessibleStackedWidget ======================
QAccessibleStackedWidget::QAccessibleStackedWidget(QWidget *widget)
    : QAccessibleWidget(widget, QAccessible::LayeredPane)
{
    Q_ASSERT(qobject_cast<QStackedWidget *>(widget));
}

QAccessibleInterface *QAccessibleStackedWidget::childAt(int x, int y) const
{
    if (!stackedWidget()->isVisible())
        return 0;
    QWidget *currentWidget = stackedWidget()->currentWidget();
    if (!currentWidget)
        return 0;
    QPoint position = currentWidget->mapFromGlobal(QPoint(x, y));
    if (currentWidget->rect().contains(position))
        return child(stackedWidget()->currentIndex());
    return 0;
}

int QAccessibleStackedWidget::childCount() const
{
    return stackedWidget()->count();
}

int QAccessibleStackedWidget::indexOfChild(const QAccessibleInterface *child) const
{
    if (!child)
        return -1;

    QWidget *widget = qobject_cast<QWidget*>(child->object());
    return stackedWidget()->indexOf(widget);
}

QAccessibleInterface *QAccessibleStackedWidget::child(int index) const
{
    if (index < 0 || index >= stackedWidget()->count())
        return 0;
    return QAccessible::queryAccessibleInterface(stackedWidget()->widget(index));
}

QStackedWidget *QAccessibleStackedWidget::stackedWidget() const
{
    return static_cast<QStackedWidget *>(object());
}
#endif // QT_NO_STACKEDWIDGET

#ifndef QT_NO_TOOLBOX
// ======================= QAccessibleToolBox ======================
QAccessibleToolBox::QAccessibleToolBox(QWidget *widget)
    : QAccessibleWidget(widget, QAccessible::LayeredPane)
{
    Q_ASSERT(qobject_cast<QToolBox *>(widget));
}

QToolBox * QAccessibleToolBox::toolBox() const
{
    return static_cast<QToolBox *>(object());
}
#endif // QT_NO_TOOLBOX

// ======================= QAccessibleMdiArea ======================
#ifndef QT_NO_MDIAREA
QAccessibleMdiArea::QAccessibleMdiArea(QWidget *widget)
    : QAccessibleWidget(widget, QAccessible::LayeredPane)
{
    Q_ASSERT(qobject_cast<QMdiArea *>(widget));
}

int QAccessibleMdiArea::childCount() const
{
    return mdiArea()->subWindowList().count();
}

QAccessibleInterface *QAccessibleMdiArea::child(int index) const
{
    QList<QMdiSubWindow *> subWindows = mdiArea()->subWindowList();
    QWidget *targetObject = subWindows.value(index);
    if (!targetObject)
       return 0;
    return QAccessible::queryAccessibleInterface(targetObject);
}


int QAccessibleMdiArea::indexOfChild(const QAccessibleInterface *child) const
{
    if (!child || !child->object() || mdiArea()->subWindowList().isEmpty())
        return -1;
    if (QMdiSubWindow *window = qobject_cast<QMdiSubWindow *>(child->object())) {
        return mdiArea()->subWindowList().indexOf(window);
    }
    return -1;
}

QMdiArea *QAccessibleMdiArea::mdiArea() const
{
    return static_cast<QMdiArea *>(object());
}

// ======================= QAccessibleMdiSubWindow ======================
QAccessibleMdiSubWindow::QAccessibleMdiSubWindow(QWidget *widget)
    : QAccessibleWidget(widget, QAccessible::Window)
{
    Q_ASSERT(qobject_cast<QMdiSubWindow *>(widget));
}

QString QAccessibleMdiSubWindow::text(QAccessible::Text textType) const
{
    if (textType == QAccessible::Name) {
        QString title = mdiSubWindow()->windowTitle();
        title.replace(QLatin1String("[*]"), QLatin1String(""));
        return title;
    }
    return QAccessibleWidget::text(textType);
}

void QAccessibleMdiSubWindow::setText(QAccessible::Text textType, const QString &text)
{
    if (textType == QAccessible::Name)
        mdiSubWindow()->setWindowTitle(text);
    else
        QAccessibleWidget::setText(textType, text);
}

QAccessible::State QAccessibleMdiSubWindow::state() const
{
    QAccessible::State state;
    state.focusable = true;
    if (!mdiSubWindow()->isMaximized()) {
        state.movable = true;
        state.sizeable = true;
    }
    if (mdiSubWindow()->isAncestorOf(QApplication::focusWidget())
            || QApplication::focusWidget() == mdiSubWindow())
        state.focused = true;
    if (!mdiSubWindow()->isVisible())
        state.invisible = true;
    if (const QWidget *parent = mdiSubWindow()->parentWidget())
        if (!parent->contentsRect().contains(mdiSubWindow()->geometry()))
            state.offscreen = true;
    if (!mdiSubWindow()->isEnabled())
        state.disabled = true;
    return state;
}

int QAccessibleMdiSubWindow::childCount() const
{
    if (mdiSubWindow()->widget())
        return 1;
    return 0;
}

QAccessibleInterface *QAccessibleMdiSubWindow::child(int index) const
{
    QMdiSubWindow *source = mdiSubWindow();
    if (index != 0 || !source->widget())
        return 0;

    return QAccessible::queryAccessibleInterface(source->widget());
}

int QAccessibleMdiSubWindow::indexOfChild(const QAccessibleInterface *child) const
{
    if (child && child->object() && child->object() == mdiSubWindow()->widget())
        return 0;
    return -1;
}

QRect QAccessibleMdiSubWindow::rect() const
{
    if (mdiSubWindow()->isHidden())
        return QRect();
    if (!mdiSubWindow()->parent())
        return QAccessibleWidget::rect();
    const QPoint pos = mdiSubWindow()->mapToGlobal(QPoint(0, 0));
    return QRect(pos, mdiSubWindow()->size());
}

QMdiSubWindow *QAccessibleMdiSubWindow::mdiSubWindow() const
{
    return static_cast<QMdiSubWindow *>(object());
}
#endif // QT_NO_MDIAREA

#ifndef QT_NO_DIALOGBUTTONBOX
// ======================= QAccessibleDialogButtonBox ======================
QAccessibleDialogButtonBox::QAccessibleDialogButtonBox(QWidget *widget)
    : QAccessibleWidget(widget, QAccessible::Grouping)
{
    Q_ASSERT(qobject_cast<QDialogButtonBox*>(widget));
}

#endif // QT_NO_DIALOGBUTTONBOX

#if !defined(QT_NO_TEXTBROWSER) && !defined(QT_NO_CURSOR)
QAccessibleTextBrowser::QAccessibleTextBrowser(QWidget *widget)
    : QAccessibleTextEdit(widget)
{
    Q_ASSERT(qobject_cast<QTextBrowser *>(widget));
}

QAccessible::Role QAccessibleTextBrowser::role() const
{
    return QAccessible::StaticText;
}
#endif // QT_NO_TEXTBROWSER && QT_NO_CURSOR

#ifndef QT_NO_CALENDARWIDGET
// ===================== QAccessibleCalendarWidget ========================
QAccessibleCalendarWidget::QAccessibleCalendarWidget(QWidget *widget)
    : QAccessibleWidget(widget, QAccessible::Table)
{
    Q_ASSERT(qobject_cast<QCalendarWidget *>(widget));
}

int QAccessibleCalendarWidget::childCount() const
{
   return calendarWidget()->isNavigationBarVisible() ? 2 : 1;
}

int QAccessibleCalendarWidget::indexOfChild(const QAccessibleInterface *child) const
{
    if (!child || !child->object() || childCount() <= 0)
        return -1;
    if (qobject_cast<QAbstractItemView *>(child->object()))
        return childCount() - 1; // FIXME
    return 0;
}

QAccessibleInterface *QAccessibleCalendarWidget::child(int index) const
{
    if (index < 0 || index >= childCount())
        return 0;

    if (childCount() > 1 && index == 0)
        return QAccessible::queryAccessibleInterface(navigationBar());

    return QAccessible::queryAccessibleInterface(calendarView());
}

QCalendarWidget *QAccessibleCalendarWidget::calendarWidget() const
{
    return static_cast<QCalendarWidget *>(object());
}

QAbstractItemView *QAccessibleCalendarWidget::calendarView() const
{
    foreach (QObject *child, calendarWidget()->children()) {
        if (child->objectName() == QLatin1String("qt_calendar_calendarview"))
            return static_cast<QAbstractItemView *>(child);
    }
    return 0;
}

QWidget *QAccessibleCalendarWidget::navigationBar() const
{
    foreach (QObject *child, calendarWidget()->children()) {
        if (child->objectName() == QLatin1String("qt_calendar_navigationbar"))
            return static_cast<QWidget *>(child);
    }
    return 0;
}
#endif // QT_NO_CALENDARWIDGET

#ifndef QT_NO_DOCKWIDGET

// Dock Widget - order of children:
// - Content widget
// - Float button
// - Close button
// If there is a custom title bar widget, that one becomes child 1, after the content 0
// (in that case the buttons are ignored)
QAccessibleDockWidget::QAccessibleDockWidget(QWidget *widget)
    : QAccessibleWidget(widget, QAccessible::Window)
{
}

QDockWidgetLayout *QAccessibleDockWidget::dockWidgetLayout() const
{
    return qobject_cast<QDockWidgetLayout*>(dockWidget()->layout());
}

int QAccessibleDockWidget::childCount() const
{
    if (dockWidget()->titleBarWidget()) {
        return dockWidget()->widget() ? 2 : 1;
    }
    return dockWidgetLayout()->count();
}

QAccessibleInterface *QAccessibleDockWidget::child(int index) const
{
    if (dockWidget()->titleBarWidget()) {
        if ((!dockWidget()->widget() && index == 0) || (index == 1))
            return QAccessible::queryAccessibleInterface(dockWidget()->titleBarWidget());
        if (index == 0)
            return QAccessible::queryAccessibleInterface(dockWidget()->widget());
    } else {
        QLayoutItem *item = dockWidgetLayout()->itemAt(index);
        if (item)
            return QAccessible::queryAccessibleInterface(item->widget());
    }
    return 0;
}

int QAccessibleDockWidget::indexOfChild(const QAccessibleInterface *child) const
{
    if (!child || !child->object() || child->object()->parent() != object())
        return -1;

    if (dockWidget()->titleBarWidget() == child->object()) {
        return dockWidget()->widget() ? 1 : 0;
    }

    return dockWidgetLayout()->indexOf(qobject_cast<QWidget*>(child->object()));
}

QRect QAccessibleDockWidget::rect() const
{
    QRect rect;

    if (dockWidget()->isFloating()) {
        rect = dockWidget()->frameGeometry();
    } else {
        rect = dockWidget()->rect();
        rect.moveTopLeft(dockWidget()->mapToGlobal(rect.topLeft()));
    }

    return rect;
}

QDockWidget *QAccessibleDockWidget::dockWidget() const
{
    return static_cast<QDockWidget *>(object());
}

QString QAccessibleDockWidget::text(QAccessible::Text t) const
{
    if (t == QAccessible::Name) {
        return qt_accStripAmp(dockWidget()->windowTitle());
    } else if (t == QAccessible::Accelerator) {
        return qt_accHotKey(dockWidget()->windowTitle());
    }
    return QString();
}
#endif // QT_NO_DOCKWIDGET

#ifndef QT_NO_CURSOR

QAccessibleTextWidget::QAccessibleTextWidget(QWidget *o, QAccessible::Role r, const QString &name):
    QAccessibleWidget(o, r, name)
{

}

QAccessible::State QAccessibleTextWidget::state() const
{
    QAccessible::State s = QAccessibleWidget::state();
    s.selectableText = true;
    s.multiLine = true;
    return s;
}

QRect QAccessibleTextWidget::characterRect(int offset) const
{
    QTextBlock block = textDocument()->findBlock(offset);
    if (!block.isValid())
        return QRect();

    QTextLayout *layout = block.layout();
    QPointF layoutPosition = layout->position();
    int relativeOffset = offset - block.position();
    QTextLine line = layout->lineForTextPosition(relativeOffset);

    QRect r;

    if (line.isValid()) {
        qreal x = line.cursorToX(relativeOffset);

        QTextCharFormat format;
        QTextBlock::iterator iter = block.begin();
        if (iter.atEnd())
            format = block.charFormat();
        else {
            while (!iter.atEnd() && !iter.fragment().contains(offset))
                ++iter;
            if (iter.atEnd()) // newline should have same format as preceding character
                --iter;
            format = iter.fragment().charFormat();
        }

        QFontMetrics fm(format.font());
        const QString ch = text(offset, offset + 1);
        if (!ch.isEmpty()) {
            int w = fm.width(ch);
            int h = fm.height();
            r = QRect(layoutPosition.x() + x, layoutPosition.y() + line.y() + line.ascent() + fm.descent() - h,
                      w, h);
            r.moveTo(viewport()->mapToGlobal(r.topLeft()));
        }
        r.translate(-scrollBarPosition());
    }

    return r;
}

int QAccessibleTextWidget::offsetAtPoint(const QPoint &point) const
{
    QPoint p = viewport()->mapFromGlobal(point);
    // convert to document coordinates
    p += scrollBarPosition();
    return textDocument()->documentLayout()->hitTest(p, Qt::ExactHit);
}

int QAccessibleTextWidget::selectionCount() const
{
    return textCursor().hasSelection() ? 1 : 0;
}

namespace {
/*!
    \internal
    \brief Helper class for AttributeFormatter

    This class is returned from AttributeFormatter's indexing operator to act
    as a proxy for the following assignment.

    It uses perfect forwarding in its assignment operator to amend the RHS
    with the formatting of the key, using QStringBuilder. Consequently, the
    RHS can be anything that QStringBuilder supports.
*/
class AttributeFormatterRef {
    QString &string;
    const char *key;
    friend class AttributeFormatter;
    AttributeFormatterRef(QString &string, const char *key) : string(string), key(key) {}
public:
    template <typename RHS>
    void operator=(RHS &&rhs)
    { string += QLatin1String(key) + QLatin1Char(':') + std::forward<RHS>(rhs) + QLatin1Char(';'); }
};

/*!
    \internal
    \brief Small string-builder class that supports a map-like API to serialize key-value pairs.
    \code
    AttributeFormatter attrs;
    attrs["foo"] = QLatinString("hello") + world + QLatin1Char('!');
    \endcode
    The key type is always \c{const char*}, and the right-hand-side can
    be any QStringBuilder expression.

    Breaking it down, this class provides the indexing operator, stores
    the key in an instance of, and then returns, AttributeFormatterRef,
    which is the class that provides the assignment part of the operation.
*/
class AttributeFormatter {
    QString string;
public:
    AttributeFormatterRef operator[](const char *key)
    { return AttributeFormatterRef(string, key); }

    QString toFormatted() const { return string; }
};
} // unnamed namespace

QString QAccessibleTextWidget::attributes(int offset, int *startOffset, int *endOffset) const
{
    /* The list of attributes can be found at:
     http://linuxfoundation.org/collaborate/workgroups/accessibility/iaccessible2/textattributes
    */

    // IAccessible2 defines -1 as length and -2 as cursor position
    if (offset == -2)
        offset = cursorPosition();

    const int charCount = characterCount();

    // -1 doesn't make much sense here, but it's better to return something
    // screen readers may ask for text attributes at the cursor pos which may be equal to length
    if (offset == -1 || offset == charCount)
        offset = charCount - 1;

    if (offset < 0 || offset > charCount) {
        *startOffset = -1;
        *endOffset = -1;
        return QString();
    }


    QTextCursor cursor = textCursor();
    cursor.setPosition(offset);
    QTextBlock block = cursor.block();

    int blockStart = block.position();
    int blockEnd = blockStart + block.length();

    QTextBlock::iterator iter = block.begin();
    int lastFragmentIndex = blockStart;
    while (!iter.atEnd()) {
        QTextFragment f = iter.fragment();
        if (f.contains(offset))
            break;
        lastFragmentIndex = f.position() + f.length();
        ++iter;
    }

    QTextCharFormat charFormat;
    if (!iter.atEnd()) {
        QTextFragment fragment = iter.fragment();
        charFormat = fragment.charFormat();
        int pos = fragment.position();
        // text block and fragment may overlap, use the smallest common range
        *startOffset = qMax(pos, blockStart);
        *endOffset = qMin(pos + fragment.length(), blockEnd);
    } else {
        charFormat = block.charFormat();
        *startOffset = lastFragmentIndex;
        *endOffset = blockEnd;
    }
    Q_ASSERT(*startOffset <= offset);
    Q_ASSERT(*endOffset >= offset);

    QTextBlockFormat blockFormat = cursor.blockFormat();

    const QFont charFormatFont = charFormat.font();

    AttributeFormatter attrs;
    QString family = charFormatFont.family();
    if (!family.isEmpty()) {
        family = family.replace('\\', QLatin1String("\\\\"));
        family = family.replace(':', QLatin1String("\\:"));
        family = family.replace(',', QLatin1String("\\,"));
        family = family.replace('=', QLatin1String("\\="));
        family = family.replace(';', QLatin1String("\\;"));
        family = family.replace('\"', QLatin1String("\\\""));
        attrs["font-family"] = QLatin1Char('"') + family + QLatin1Char('"');
    }

    int fontSize = int(charFormatFont.pointSize());
    if (fontSize)
        attrs["font-size"] = QString::fromLatin1("%1pt").arg(fontSize);

    //Different weight values are not handled
    attrs["font-weight"] = QString::fromLatin1(charFormatFont.weight() > QFont::Normal ? "bold" : "normal");

    QFont::Style style = charFormatFont.style();
    attrs["font-style"] = QString::fromLatin1((style == QFont::StyleItalic) ? "italic" : ((style == QFont::StyleOblique) ? "oblique": "normal"));

    QTextCharFormat::UnderlineStyle underlineStyle = charFormat.underlineStyle();
    if (underlineStyle == QTextCharFormat::NoUnderline && charFormatFont.underline()) // underline could still be set in the default font
        underlineStyle = QTextCharFormat::SingleUnderline;
    QString underlineStyleValue;
    switch (underlineStyle) {
        case QTextCharFormat::NoUnderline:
            break;
        case QTextCharFormat::SingleUnderline:
            underlineStyleValue = QStringLiteral("solid");
            break;
        case QTextCharFormat::DashUnderline:
            underlineStyleValue = QStringLiteral("dash");
            break;
        case QTextCharFormat::DotLine:
            underlineStyleValue = QStringLiteral("dash");
            break;
        case QTextCharFormat::DashDotLine:
            underlineStyleValue = QStringLiteral("dot-dash");
            break;
        case QTextCharFormat::DashDotDotLine:
            underlineStyleValue = QStringLiteral("dot-dot-dash");
            break;
        case QTextCharFormat::WaveUnderline:
            underlineStyleValue = QStringLiteral("wave");
            break;
        case QTextCharFormat::SpellCheckUnderline:
            underlineStyleValue = QStringLiteral("wave"); // this is not correct, but provides good approximation at least
            break;
        default:
            qWarning() << "Unknown QTextCharFormat::​UnderlineStyle value " << underlineStyle << " could not be translated to IAccessible2 value";
            break;
    }
    if (!underlineStyleValue.isNull()) {
        attrs["text-underline-style"] = underlineStyleValue;
        attrs["text-underline-type"] = QStringLiteral("single"); // if underlineStyleValue is set, there is an underline, and Qt does not support other than single ones
    } // else both are "none" which is the default - no need to set them

    if (block.textDirection() == Qt::RightToLeft)
        attrs["writing-mode"] = QStringLiteral("rl");

    QTextCharFormat::VerticalAlignment alignment = charFormat.verticalAlignment();
    attrs["text-position"] = QString::fromLatin1((alignment == QTextCharFormat::AlignSubScript) ? "sub" : ((alignment == QTextCharFormat::AlignSuperScript) ? "super" : "baseline" ));

    QBrush background = charFormat.background();
    if (background.style() == Qt::SolidPattern) {
        attrs["background-color"] = QString::fromLatin1("rgb(%1,%2,%3)").arg(background.color().red()).arg(background.color().green()).arg(background.color().blue());
    }

    QBrush foreground = charFormat.foreground();
    if (foreground.style() == Qt::SolidPattern) {
        attrs["color"] = QString::fromLatin1("rgb(%1,%2,%3)").arg(foreground.color().red()).arg(foreground.color().green()).arg(foreground.color().blue());
    }

    switch (blockFormat.alignment() & (Qt::AlignLeft | Qt::AlignRight | Qt::AlignHCenter | Qt::AlignJustify)) {
    case Qt::AlignLeft:
        attrs["text-align"] = QStringLiteral("left");
        break;
    case Qt::AlignRight:
        attrs["text-align"] = QStringLiteral("right");
        break;
    case Qt::AlignHCenter:
        attrs["text-align"] = QStringLiteral("center");
        break;
    case Qt::AlignJustify:
        attrs["text-align"] = QStringLiteral("justify");
        break;
    }

    return attrs.toFormatted();
}

int QAccessibleTextWidget::cursorPosition() const
{
    return textCursor().position();
}

void QAccessibleTextWidget::selection(int selectionIndex, int *startOffset, int *endOffset) const
{
    *startOffset = *endOffset = 0;
    QTextCursor cursor = textCursor();

    if (selectionIndex != 0 || !cursor.hasSelection())
        return;

    *startOffset = cursor.selectionStart();
    *endOffset = cursor.selectionEnd();
}

QString QAccessibleTextWidget::text(int startOffset, int endOffset) const
{
    QTextCursor cursor(textCursor());

    cursor.setPosition(startOffset, QTextCursor::MoveAnchor);
    cursor.setPosition(endOffset, QTextCursor::KeepAnchor);

    return cursor.selectedText().replace(QChar(QChar::ParagraphSeparator), QLatin1Char('\n'));
}

QPoint QAccessibleTextWidget::scrollBarPosition() const
{
    return QPoint(0, 0);
}


QString QAccessibleTextWidget::textBeforeOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                                                int *startOffset, int *endOffset) const
{
    Q_ASSERT(startOffset);
    Q_ASSERT(endOffset);

    QTextCursor cursor = textCursor();
    cursor.setPosition(offset);
    QPair<int, int> boundaries = QAccessible::qAccessibleTextBoundaryHelper(cursor, boundaryType);
    cursor.setPosition(boundaries.first - 1);
    boundaries = QAccessible::qAccessibleTextBoundaryHelper(cursor, boundaryType);

    *startOffset = boundaries.first;
    *endOffset = boundaries.second;

    return text(boundaries.first, boundaries.second);
 }


QString QAccessibleTextWidget::textAfterOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                                              int *startOffset, int *endOffset) const
{
    Q_ASSERT(startOffset);
    Q_ASSERT(endOffset);

    QTextCursor cursor = textCursor();
    cursor.setPosition(offset);
    QPair<int, int> boundaries = QAccessible::qAccessibleTextBoundaryHelper(cursor, boundaryType);
    cursor.setPosition(boundaries.second);
    boundaries = QAccessible::qAccessibleTextBoundaryHelper(cursor, boundaryType);

    *startOffset = boundaries.first;
    *endOffset = boundaries.second;

    return text(boundaries.first, boundaries.second);
}

QString QAccessibleTextWidget::textAtOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                                            int *startOffset, int *endOffset) const
{
    Q_ASSERT(startOffset);
    Q_ASSERT(endOffset);

    QTextCursor cursor = textCursor();
    cursor.setPosition(offset);
    QPair<int, int> boundaries = QAccessible::qAccessibleTextBoundaryHelper(cursor, boundaryType);

    *startOffset = boundaries.first;
    *endOffset = boundaries.second;

    return text(boundaries.first, boundaries.second);
}

void QAccessibleTextWidget::setCursorPosition(int position)
{
    QTextCursor cursor = textCursor();
    cursor.setPosition(position);
    setTextCursor(cursor);
}

void QAccessibleTextWidget::addSelection(int startOffset, int endOffset)
{
    setSelection(0, startOffset, endOffset);
}

void QAccessibleTextWidget::removeSelection(int selectionIndex)
{
    if (selectionIndex != 0)
        return;

    QTextCursor cursor = textCursor();
    cursor.clearSelection();
    setTextCursor(cursor);
}

void QAccessibleTextWidget::setSelection(int selectionIndex, int startOffset, int endOffset)
{
    if (selectionIndex != 0)
        return;

    QTextCursor cursor = textCursor();
    cursor.setPosition(startOffset, QTextCursor::MoveAnchor);
    cursor.setPosition(endOffset, QTextCursor::KeepAnchor);
    setTextCursor(cursor);
}

int QAccessibleTextWidget::characterCount() const
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    return cursor.position();
}

QTextCursor QAccessibleTextWidget::textCursorForRange(int startOffset, int endOffset) const
{
    QTextCursor cursor = textCursor();
    cursor.setPosition(startOffset, QTextCursor::MoveAnchor);
    cursor.setPosition(endOffset, QTextCursor::KeepAnchor);

    return cursor;
}

void QAccessibleTextWidget::deleteText(int startOffset, int endOffset)
{
    QTextCursor cursor = textCursorForRange(startOffset, endOffset);
    cursor.removeSelectedText();
}

void QAccessibleTextWidget::insertText(int offset, const QString &text)
{
    QTextCursor cursor = textCursor();
    cursor.setPosition(offset);
    cursor.insertText(text);
}

void QAccessibleTextWidget::replaceText(int startOffset, int endOffset, const QString &text)
{
    QTextCursor cursor = textCursorForRange(startOffset, endOffset);
    cursor.removeSelectedText();
    cursor.insertText(text);
}
#endif // QT_NO_CURSOR


#ifndef QT_NO_MAINWINDOW
QAccessibleMainWindow::QAccessibleMainWindow(QWidget *widget)
    : QAccessibleWidget(widget, QAccessible::Window) { }

QAccessibleInterface *QAccessibleMainWindow::child(int index) const
{
    QList<QWidget*> kids = childWidgets(mainWindow());
    if (index >= 0 && index < kids.count()) {
        return QAccessible::queryAccessibleInterface(kids.at(index));
    }
    return 0;
}

int QAccessibleMainWindow::childCount() const
{
    QList<QWidget*> kids = childWidgets(mainWindow());
    return kids.count();
}

int QAccessibleMainWindow::indexOfChild(const QAccessibleInterface *iface) const
{
    QList<QWidget*> kids = childWidgets(mainWindow());
    return kids.indexOf(static_cast<QWidget*>(iface->object()));
}

QAccessibleInterface *QAccessibleMainWindow::childAt(int x, int y) const
{
    QWidget *w = widget();
    if (!w->isVisible())
        return 0;
    QPoint gp = w->mapToGlobal(QPoint(0, 0));
    if (!QRect(gp.x(), gp.y(), w->width(), w->height()).contains(x, y))
        return 0;

    const QWidgetList kids = childWidgets(mainWindow());
    QPoint rp = mainWindow()->mapFromGlobal(QPoint(x, y));
    for (QWidget *child : kids) {
        if (!child->isWindow() && !child->isHidden() && child->geometry().contains(rp)) {
            return QAccessible::queryAccessibleInterface(child);
        }
    }
    return 0;
}

QMainWindow *QAccessibleMainWindow::mainWindow() const
{
    return qobject_cast<QMainWindow *>(object());
}

#endif //QT_NO_MAINWINDOW

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY
