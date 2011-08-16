/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qaccessiblewidgets.h"
#include "qabstracttextdocumentlayout.h"
#include "qapplication.h"
#include "qclipboard.h"
#include "qtextedit.h"
#include "private/qtextedit_p.h"
#include "qtextdocument.h"
#include "qtextobject.h"
#include "qscrollbar.h"
#include "qdebug.h"
#include <QApplication>
#include <QStackedWidget>
#include <QToolBox>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QWorkspace>
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
#include <QtGui/QFocusFrame>

#ifndef QT_NO_ACCESSIBILITY

QT_BEGIN_NAMESPACE

using namespace QAccessible2;

QString Q_GUI_EXPORT qt_accStripAmp(const QString &text);
QString Q_GUI_EXPORT qt_accHotKey(const QString &text);

QList<QWidget*> childWidgets(const QWidget *widget, bool includeTopLevel)
{
    if (widget == 0)
        return QList<QWidget*>();
    QList<QObject*> list = widget->children();
    QList<QWidget*> widgets;
    for (int i = 0; i < list.size(); ++i) {
        QWidget *w = qobject_cast<QWidget *>(list.at(i));
        if (!w)
            continue;
        QString objectName = w->objectName();
        if ((includeTopLevel || !w->isWindow()) 
              && !qobject_cast<QFocusFrame*>(w)
              && !qobject_cast<QMenu*>(w)
              && objectName != QLatin1String("qt_rubberband")
              && objectName != QLatin1String("qt_qmainwindow_extended_splitter")) {
            widgets.append(w);
        }
    }
    return widgets;
}

static inline int distance(QWidget *source, QWidget *target,
                           QAccessible::RelationFlag relation)
{
    if (!source || !target)
        return -1;

    int returnValue = -1;
    switch (relation) {
    case QAccessible::Up:
        if (target->y() <= source->y())
            returnValue = source->y() - target->y();
        break;
    case QAccessible::Down:
        if (target->y() >= source->y() + source->height())
            returnValue = target->y() - (source->y() + source->height());
        break;
    case QAccessible::Right:
        if (target->x() >= source->x() + source->width())
            returnValue = target->x() - (source->x() + source->width());
        break;
    case QAccessible::Left:
        if (target->x() <= source->x())
            returnValue = source->x() - target->x();
        break;
    default:
        break;
    }
    return returnValue;
}

static inline QWidget *mdiAreaNavigate(QWidget *area,
                                       QAccessible::RelationFlag relation, int entry)
{
#if defined(QT_NO_MDIAREA) && defined(QT_NO_WORKSPACE)
    Q_UNUSED(area);
#endif
#ifndef QT_NO_MDIAREA
    const QMdiArea *mdiArea = qobject_cast<QMdiArea *>(area);
#endif
#ifndef QT_NO_WORKSPACE
    const QWorkspace *workspace = qobject_cast<QWorkspace *>(area);
#endif
    if (true
#ifndef QT_NO_MDIAREA
        && !mdiArea
#endif
#ifndef QT_NO_WORKSPACE
    && !workspace
#endif
    )
        return 0;

    QWidgetList windows;
#ifndef QT_NO_MDIAREA
    if (mdiArea) {
        foreach (QMdiSubWindow *window, mdiArea->subWindowList())
            windows.append(window);
    } else
#endif
    {
#ifndef QT_NO_WORKSPACE
        foreach (QWidget *window, workspace->windowList())
            windows.append(window->parentWidget());
#endif
    }

    if (windows.isEmpty() || entry < 1 || entry > windows.count())
        return 0;

    QWidget *source = windows.at(entry - 1);
    QMap<int, QWidget *> candidates;
    foreach (QWidget *window, windows) {
        if (source == window)
            continue;
        int candidateDistance = distance(source, window, relation);
        if (candidateDistance >= 0)
            candidates.insert(candidateDistance, window);
    }

    int minimumDistance = INT_MAX;
    QWidget *target = 0;
    foreach (QWidget *candidate, candidates) {
        switch (relation) {
        case QAccessible::Up:
        case QAccessible::Down:
            if (qAbs(candidate->x() - source->x()) < minimumDistance) {
                target = candidate;
                minimumDistance = qAbs(candidate->x() - source->x());
            }
            break;
        case QAccessible::Left:
        case QAccessible::Right:
            if (qAbs(candidate->y() - source->y()) < minimumDistance) {
                target = candidate;
                minimumDistance = qAbs(candidate->y() - source->y());
            }
            break;
        default:
            break;
        }
        if (minimumDistance == 0)
            break;
    }

#ifndef QT_NO_WORKSPACE
    if (workspace) {
        foreach (QWidget *widget, workspace->windowList()) {
            if (widget->parentWidget() == target)
                target = widget;
        }
    }
#endif
    return target;
}

#ifndef QT_NO_TEXTEDIT

/*!
  \class QAccessibleTextEdit
  \brief The QAccessibleTextEdit class implements the QAccessibleInterface for richtext editors.
  \internal
*/

static QTextBlock qTextBlockAt(const QTextDocument *doc, int pos)
{
    Q_ASSERT(pos >= 0);

    QTextBlock block = doc->begin();
    int i = 0;
    while (block.isValid() && i < pos) {
        block = block.next();
        ++i;
    }
    return block;
}

static int qTextBlockPosition(QTextBlock block)
{
    int child = 0;
    while (block.isValid()) {
        block = block.previous();
        ++child;
    }

    return child;
}

/*!
  \fn QAccessibleTextEdit::QAccessibleTextEdit(QWidget* widget)

  Constructs a QAccessibleTextEdit object for a \a widget.
*/
QAccessibleTextEdit::QAccessibleTextEdit(QWidget *o)
: QAccessibleWidgetEx(o, EditableText)
{
    Q_ASSERT(widget()->inherits("QTextEdit"));
    childOffset = QAccessibleWidgetEx::childCount();
}

/*! Returns the text edit. */
QTextEdit *QAccessibleTextEdit::textEdit() const
{
    return static_cast<QTextEdit *>(widget());
}

QRect QAccessibleTextEdit::rect(int child) const
{
    if (child <= childOffset)
        return QAccessibleWidgetEx::rect(child);

     QTextEdit *edit = textEdit();
     QTextBlock block = qTextBlockAt(edit->document(), child - childOffset - 1);
     if (!block.isValid())
         return QRect();

     QRect rect = edit->document()->documentLayout()->blockBoundingRect(block).toRect();
     rect.translate(-edit->horizontalScrollBar()->value(), -edit->verticalScrollBar()->value());

     rect = edit->viewport()->rect().intersect(rect);
     if (rect.isEmpty())
         return QRect();

     return rect.translated(edit->viewport()->mapToGlobal(QPoint(0, 0)));
}

int QAccessibleTextEdit::childAt(int x, int y) const
{
    QTextEdit *edit = textEdit();
    if (!edit->isVisible())
        return -1;

    QPoint point = edit->viewport()->mapFromGlobal(QPoint(x, y));
    QTextBlock block = edit->cursorForPosition(point).block();
    if (block.isValid())
        return qTextBlockPosition(block) + childOffset;

    return QAccessibleWidgetEx::childAt(x, y);
}

/*! \reimp */
QString QAccessibleTextEdit::text(Text t, int child) const
{
    if (t == Value) {
        if (child > childOffset)
            return qTextBlockAt(textEdit()->document(), child - childOffset - 1).text();
        if (!child)
            return textEdit()->toPlainText();
    }

    return QAccessibleWidgetEx::text(t, child);
}

/*! \reimp */
void QAccessibleTextEdit::setText(Text t, int child, const QString &text)
{
    if (t != Value || (child > 0 && child <= childOffset)) {
        QAccessibleWidgetEx::setText(t, child, text);
        return;
    }
    if (textEdit()->isReadOnly())
        return;

    if (!child) {
        textEdit()->setText(text);
        return;
    }
    QTextBlock block = qTextBlockAt(textEdit()->document(), child - childOffset - 1);
    if (!block.isValid())
        return;

    QTextCursor cursor(block);
    cursor.select(QTextCursor::BlockUnderCursor);
    cursor.insertText(text);
}

/*! \reimp */
QAccessible::Role QAccessibleTextEdit::role(int child) const
{
    if (child > childOffset)
        return EditableText;
    return QAccessibleWidgetEx::role(child);
}

QVariant QAccessibleTextEdit::invokeMethodEx(QAccessible::Method method, int child,
                                                     const QVariantList &params)
{
    if (child)
        return QVariant();

    switch (method) {
    case ListSupportedMethods: {
        QSet<QAccessible::Method> set;
        set << ListSupportedMethods << SetCursorPosition << GetCursorPosition;
        return QVariant::fromValue(set | qvariant_cast<QSet<QAccessible::Method> >(
                    QAccessibleWidgetEx::invokeMethodEx(method, child, params)));
    }
    case SetCursorPosition:
        setCursorPosition(params.value(0).toInt());
        return true;
    case GetCursorPosition:
        return textEdit()->textCursor().position();
    default:
        return QAccessibleWidgetEx::invokeMethodEx(method, child, params);
    }
}

int QAccessibleTextEdit::childCount() const
{
    return childOffset + textEdit()->document()->blockCount();
}
#endif // QT_NO_TEXTEDIT

#ifndef QT_NO_STACKEDWIDGET
// ======================= QAccessibleStackedWidget ======================
QAccessibleStackedWidget::QAccessibleStackedWidget(QWidget *widget)
    : QAccessibleWidgetEx(widget, LayeredPane)
{
    Q_ASSERT(qobject_cast<QStackedWidget *>(widget));
}

QVariant QAccessibleStackedWidget::invokeMethodEx(QAccessible::Method, int, const QVariantList &)
{
    return QVariant();
}


int QAccessibleStackedWidget::childAt(int x, int y) const
{
    if (!stackedWidget()->isVisible())
        return -1;
    QWidget *currentWidget = stackedWidget()->currentWidget();
    if (!currentWidget)
        return -1;
    QPoint position = currentWidget->mapFromGlobal(QPoint(x, y));
    if (currentWidget->rect().contains(position))
        return 1;
    return -1;
}

int QAccessibleStackedWidget::childCount() const
{
    return stackedWidget()->count();
}

int QAccessibleStackedWidget::indexOfChild(const QAccessibleInterface *child) const
{
    if (!child)
        return -1;

    QWidget* widget = qobject_cast<QWidget*>(child->object());
    int index = stackedWidget()->indexOf(widget);
    if (index >= 0) // one based counting of children
        return index + 1;
    return -1;
}

int QAccessibleStackedWidget::navigate(RelationFlag relation, int entry, QAccessibleInterface **target) const
{
    *target = 0;

    QObject *targetObject = 0;
    switch (relation) {
    case Child:
        if (entry < 1 || entry > stackedWidget()->count())
            return -1;
        targetObject = stackedWidget()->widget(entry-1);
        break;
    default:
        return QAccessibleWidgetEx::navigate(relation, entry, target);
    }
    *target = QAccessible::queryAccessibleInterface(targetObject);
    return *target ? 0 : -1;
}

QStackedWidget *QAccessibleStackedWidget::stackedWidget() const
{
    return static_cast<QStackedWidget *>(object());
}
#endif // QT_NO_STACKEDWIDGET

#ifndef QT_NO_TOOLBOX
// ======================= QAccessibleToolBox ======================
QAccessibleToolBox::QAccessibleToolBox(QWidget *widget)
    : QAccessibleWidgetEx(widget, LayeredPane)
{
    Q_ASSERT(qobject_cast<QToolBox *>(widget));
}

QString QAccessibleToolBox::text(Text textType, int child) const
{
    if (textType != Value || child <= 0 || child > toolBox()->count())
        return QAccessibleWidgetEx::text(textType, child);
    return toolBox()->itemText(child - 1);
}

void QAccessibleToolBox::setText(Text textType, int child, const QString &text)
{
    if (textType != Value || child <= 0 || child > toolBox()->count()) {
        QAccessibleWidgetEx::setText(textType, child, text);
        return;
    }
    toolBox()->setItemText(child - 1, text);
}

QAccessible::State QAccessibleToolBox::state(int child) const
{
    QWidget *childWidget = toolBox()->widget(child - 1);
    if (!childWidget)
        return QAccessibleWidgetEx::state(child);
    QAccessible::State childState = QAccessible::Normal;
    if (toolBox()->currentWidget() == childWidget)
        childState |= QAccessible::Expanded;
    else
        childState |= QAccessible::Collapsed;
    return childState;
}

QVariant QAccessibleToolBox::invokeMethodEx(QAccessible::Method, int, const QVariantList &)
{
    return QVariant();
}

int QAccessibleToolBox::childCount() const
{
    return toolBox()->count();
}

int QAccessibleToolBox::indexOfChild(const QAccessibleInterface *child) const
{
    if (!child)
        return -1;
    QWidget *childWidget = qobject_cast<QWidget *>(child->object());
    if (!childWidget)
        return -1;
    int index = toolBox()->indexOf(childWidget);
    if (index != -1)
        ++index;
    return index;
}

int QAccessibleToolBox::navigate(RelationFlag relation, int entry, QAccessibleInterface **target) const
{
    *target = 0;
    if (entry <= 0 || entry > toolBox()->count())
        return QAccessibleWidgetEx::navigate(relation, entry, target);
    int index = -1;
    if (relation == QAccessible::Up)
        index = entry - 2;
    else if (relation == QAccessible::Down)
        index = entry;
    *target = QAccessible::queryAccessibleInterface(toolBox()->widget(index));
    return *target ? 0: -1;
}

QToolBox * QAccessibleToolBox::toolBox() const
{
    return static_cast<QToolBox *>(object());
}
#endif // QT_NO_TOOLBOX

// ======================= QAccessibleMdiArea ======================
#ifndef QT_NO_MDIAREA
QAccessibleMdiArea::QAccessibleMdiArea(QWidget *widget)
    : QAccessibleWidgetEx(widget, LayeredPane)
{
    Q_ASSERT(qobject_cast<QMdiArea *>(widget));
}

QAccessible::State QAccessibleMdiArea::state(int child) const
{
    if (child < 0)
        return QAccessibleWidgetEx::state(child);
    if (child == 0)
        return QAccessible::Normal;
    QList<QMdiSubWindow *> subWindows = mdiArea()->subWindowList();
    if (subWindows.isEmpty() || child > subWindows.count())
        return QAccessibleWidgetEx::state(child);
    if (subWindows.at(child - 1) == mdiArea()->activeSubWindow())
        return QAccessible::Focused;
    return QAccessible::Normal;
}

QVariant QAccessibleMdiArea::invokeMethodEx(QAccessible::Method, int, const QVariantList &)
{
    return QVariant();
}

int QAccessibleMdiArea::childCount() const
{
    return mdiArea()->subWindowList().count();
}

int QAccessibleMdiArea::indexOfChild(const QAccessibleInterface *child) const
{
    if (!child || !child->object() || mdiArea()->subWindowList().isEmpty())
        return -1;
    if (QMdiSubWindow *window = qobject_cast<QMdiSubWindow *>(child->object())) {
        int index = mdiArea()->subWindowList().indexOf(window);
        if (index != -1)
            return ++index;
    }
    return -1;
}

int QAccessibleMdiArea::navigate(RelationFlag relation, int entry, QAccessibleInterface **target) const
{
    *target = 0;
    QWidget *targetObject = 0;
    QList<QMdiSubWindow *> subWindows = mdiArea()->subWindowList();
    switch (relation) {
    case Child:
        if (entry < 1 || subWindows.isEmpty() || entry > subWindows.count())
            return -1;
        targetObject = subWindows.at(entry - 1);
        break;
    case Up:
    case Down:
    case Left:
    case Right:
        targetObject = mdiAreaNavigate(mdiArea(), relation, entry);
        break;
    default:
        return QAccessibleWidgetEx::navigate(relation, entry, target);
    }
    *target = QAccessible::queryAccessibleInterface(targetObject);
    return *target ? 0: -1;
}

QMdiArea *QAccessibleMdiArea::mdiArea() const
{
    return static_cast<QMdiArea *>(object());
}

// ======================= QAccessibleMdiSubWindow ======================
QAccessibleMdiSubWindow::QAccessibleMdiSubWindow(QWidget *widget)
    : QAccessibleWidgetEx(widget, QAccessible::Window)
{
    Q_ASSERT(qobject_cast<QMdiSubWindow *>(widget));
}

QString QAccessibleMdiSubWindow::text(Text textType, int child) const
{
    if (textType == QAccessible::Name && (child == 0 || child == 1)) {
        QString title = mdiSubWindow()->windowTitle();
        title.replace(QLatin1String("[*]"), QLatin1String(""));
        return title;
    }
    return QAccessibleWidgetEx::text(textType, child);
}

void QAccessibleMdiSubWindow::setText(Text textType, int child, const QString &text)
{
    if (textType == QAccessible::Name && (child == 0 || child == 1))
        mdiSubWindow()->setWindowTitle(text);
    else
        QAccessibleWidgetEx::setText(textType, child, text);
}

QAccessible::State QAccessibleMdiSubWindow::state(int child) const
{
    if (child != 0 || !mdiSubWindow()->parent())
        return QAccessibleWidgetEx::state(child);
    QAccessible::State state = QAccessible::Normal | QAccessible::Focusable;
    if (!mdiSubWindow()->isMaximized())
        state |= (QAccessible::Movable | QAccessible::Sizeable);
    if (mdiSubWindow()->isAncestorOf(QApplication::focusWidget())
            || QApplication::focusWidget() == mdiSubWindow())
        state |= QAccessible::Focused;
    if (!mdiSubWindow()->isVisible())
        state |= QAccessible::Invisible;
    if (!mdiSubWindow()->parentWidget()->contentsRect().contains(mdiSubWindow()->geometry()))
        state |= QAccessible::Offscreen;
    if (!mdiSubWindow()->isEnabled())
        state |= QAccessible::Unavailable;
    return state;
}

QVariant QAccessibleMdiSubWindow::invokeMethodEx(QAccessible::Method, int, const QVariantList &)
{
    return QVariant();
}

int QAccessibleMdiSubWindow::childCount() const
{
    if (mdiSubWindow()->widget())
        return 1;
    return 0;
}

int QAccessibleMdiSubWindow::indexOfChild(const QAccessibleInterface *child) const
{
    if (child && child->object() && child->object() == mdiSubWindow()->widget())
        return 1;
    return -1;
}

int QAccessibleMdiSubWindow::navigate(RelationFlag relation, int entry, QAccessibleInterface **target) const
{
    *target = 0;

    if (!mdiSubWindow()->parent())
        return QAccessibleWidgetEx::navigate(relation, entry, target);

    QWidget *targetObject = 0;
    QMdiSubWindow *source = mdiSubWindow();
    switch (relation) {
    case Child:
        if (entry != 1 || !source->widget())
            return -1;
        targetObject = source->widget();
        break;
    case Up:
    case Down:
    case Left:
    case Right: {
        if (entry != 0)
            break;
        QWidget *parent = source->parentWidget();
        while (parent && !parent->inherits("QMdiArea"))
            parent = parent->parentWidget();
        QMdiArea *mdiArea = qobject_cast<QMdiArea *>(parent);
        if (!mdiArea)
            break;
        int index = mdiArea->subWindowList().indexOf(source);
        if (index == -1)
            break;
        if (QWidget *dest = mdiAreaNavigate(mdiArea, relation, index + 1)) {
            *target = QAccessible::queryAccessibleInterface(dest);
            return *target ? 0 : -1;
        }
        break;
    }
    default:
        return QAccessibleWidgetEx::navigate(relation, entry, target);
    }
    *target = QAccessible::queryAccessibleInterface(targetObject);
    return *target ? 0: -1;
}

QRect QAccessibleMdiSubWindow::rect(int child) const
{
    if (mdiSubWindow()->isHidden())
        return QRect();
    if (!mdiSubWindow()->parent())
        return QAccessibleWidgetEx::rect(child);
    const QPoint pos = mdiSubWindow()->mapToGlobal(QPoint(0, 0));
    if (child == 0)
        return QRect(pos, mdiSubWindow()->size());
    if (child == 1 && mdiSubWindow()->widget()) {
        if (mdiSubWindow()->widget()->isHidden())
            return QRect();
        const QRect contentsRect = mdiSubWindow()->contentsRect();
        return QRect(pos.x() + contentsRect.x(), pos.y() + contentsRect.y(),
                     contentsRect.width(), contentsRect.height());
    }
    return QRect();
}

int QAccessibleMdiSubWindow::childAt(int x, int y) const
{
    if (!mdiSubWindow()->isVisible())
        return -1;
    if (!mdiSubWindow()->parent())
        return QAccessibleWidgetEx::childAt(x, y);
    const QRect globalGeometry = rect(0);
    if (!globalGeometry.isValid())
        return -1;
    const QRect globalChildGeometry = rect(1);
    if (globalChildGeometry.isValid() && globalChildGeometry.contains(QPoint(x, y)))
        return 1;
    if (globalGeometry.contains(QPoint(x, y)))
        return 0;
    return -1;
}

QMdiSubWindow *QAccessibleMdiSubWindow::mdiSubWindow() const
{
    return static_cast<QMdiSubWindow *>(object());
}
#endif // QT_NO_MDIAREA

// ======================= QAccessibleWorkspace ======================
#ifndef QT_NO_WORKSPACE
QAccessibleWorkspace::QAccessibleWorkspace(QWidget *widget)
    : QAccessibleWidgetEx(widget, LayeredPane)
{
    Q_ASSERT(qobject_cast<QWorkspace *>(widget));
}

QAccessible::State QAccessibleWorkspace::state(int child) const
{
    if (child < 0)
        return QAccessibleWidgetEx::state(child);
    if (child == 0)
        return QAccessible::Normal;
    QWidgetList subWindows = workspace()->windowList();
    if (subWindows.isEmpty() || child > subWindows.count())
        return QAccessibleWidgetEx::state(child);
    if (subWindows.at(child - 1) == workspace()->activeWindow())
        return QAccessible::Focused;
    return QAccessible::Normal;
}

QVariant QAccessibleWorkspace::invokeMethodEx(QAccessible::Method, int, const QVariantList &)
{
    return QVariant();
}

int QAccessibleWorkspace::childCount() const
{
    return workspace()->windowList().count();
}

int QAccessibleWorkspace::indexOfChild(const QAccessibleInterface *child) const
{
    if (!child || !child->object() || workspace()->windowList().isEmpty())
        return -1;
    if (QWidget *window = qobject_cast<QWidget *>(child->object())) {
        int index = workspace()->windowList().indexOf(window);
        if (index != -1)
            return ++index;
    }
    return -1;
}

int QAccessibleWorkspace::navigate(RelationFlag relation, int entry, QAccessibleInterface **target) const
{
    *target = 0;
    QWidget *targetObject = 0;
    QWidgetList subWindows = workspace()->windowList();
    switch (relation) {
    case Child:
        if (entry < 1 || subWindows.isEmpty() || entry > subWindows.count())
            return -1;
        targetObject = subWindows.at(entry - 1);
        break;
    case Up:
    case Down:
    case Left:
    case Right:
        targetObject = mdiAreaNavigate(workspace(), relation, entry);
        break;
    default:
        return QAccessibleWidgetEx::navigate(relation, entry, target);
    }
    *target = QAccessible::queryAccessibleInterface(targetObject);
    return *target ? 0: -1;
}

QWorkspace *QAccessibleWorkspace::workspace() const
{
    return static_cast<QWorkspace *>(object());
}
#endif

#ifndef QT_NO_DIALOGBUTTONBOX
// ======================= QAccessibleDialogButtonBox ======================
QAccessibleDialogButtonBox::QAccessibleDialogButtonBox(QWidget *widget)
    : QAccessibleWidgetEx(widget, Grouping)
{
    Q_ASSERT(qobject_cast<QDialogButtonBox*>(widget));
}

QVariant QAccessibleDialogButtonBox::invokeMethodEx(QAccessible::Method, int, const QVariantList &)
{
    return QVariant();
}
#endif // QT_NO_DIALOGBUTTONBOX

#ifndef QT_NO_TEXTBROWSER
QAccessibleTextBrowser::QAccessibleTextBrowser(QWidget *widget)
    : QAccessibleTextEdit(widget)
{
    Q_ASSERT(qobject_cast<QTextBrowser *>(widget));
}

QAccessible::Role QAccessibleTextBrowser::role(int child) const
{
    if (child != 0)
        return QAccessibleTextEdit::role(child);
    return QAccessible::StaticText;
}
#endif // QT_NO_TEXTBROWSER

#ifndef QT_NO_CALENDARWIDGET
// ===================== QAccessibleCalendarWidget ========================
QAccessibleCalendarWidget::QAccessibleCalendarWidget(QWidget *widget)
    : QAccessibleWidgetEx(widget, Table)
{
    Q_ASSERT(qobject_cast<QCalendarWidget *>(widget));
}

QVariant QAccessibleCalendarWidget::invokeMethodEx(QAccessible::Method, int, const QVariantList &)
{
    return QVariant();
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
        return childCount();
    return 1;
}

int QAccessibleCalendarWidget::navigate(RelationFlag relation, int entry, QAccessibleInterface **target) const
{
    *target = 0;
    if (entry <= 0 || entry > childCount())
        return QAccessibleWidgetEx::navigate(relation, entry, target);
    QWidget *targetWidget = 0;
    switch (relation) {
    case Child:
        if (childCount() == 1) {
            targetWidget = calendarView();
        } else {
            if (entry == 1)
                targetWidget = navigationBar();
            else
                targetWidget = calendarView();
        }
        break;
    case Up:
        if (entry == 2)
            targetWidget = navigationBar();
        break;
    case Down:
        if (entry == 1 && childCount() == 2)
            targetWidget = calendarView();
        break;
    default:
        return QAccessibleWidgetEx::navigate(relation, entry, target);
    }
    *target = queryAccessibleInterface(targetWidget);
    return *target ? 0: -1;
}

QRect QAccessibleCalendarWidget::rect(int child) const
{
    if (!calendarWidget()->isVisible() || child > childCount())
        return QRect();
    if (child == 0)
        return QAccessibleWidgetEx::rect(child);
    QWidget *childWidget = 0;
    if (childCount() == 2)
        childWidget = child == 1 ? navigationBar() : calendarView();
    else
        childWidget = calendarView();
    return QRect(childWidget->mapToGlobal(QPoint(0, 0)), childWidget->size());
}

int QAccessibleCalendarWidget::childAt(int x, int y) const
{
    const QPoint globalTargetPos = QPoint(x, y);
    if (!rect(0).contains(globalTargetPos))
        return -1;
    if (rect(1).contains(globalTargetPos))
        return 1;
    if (rect(2).contains(globalTargetPos))
        return 2;
    return 0;
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
QAccessibleDockWidget::QAccessibleDockWidget(QWidget *widget)
    : QAccessibleWidgetEx(widget, Window)
{

}

int QAccessibleDockWidget::navigate(RelationFlag relation, int entry, QAccessibleInterface **iface) const
{
    if (relation == Child) {
        if (entry == 1) {
            *iface = new QAccessibleTitleBar(dockWidget());
            return 0;
        } else if (entry == 2) {
            if (dockWidget()->widget())
                *iface = QAccessible::queryAccessibleInterface(dockWidget()->widget());
            return 0;
        }
        *iface = 0;
        return -1;
    }
    return QAccessibleWidgetEx::navigate(relation, entry, iface);
}

int QAccessibleDockWidget::childAt(int x, int y) const
{
    for (int i = childCount(); i >= 0; --i) {
        if (rect(i).contains(x,y))
            return i;
    }
    return -1;
}

int QAccessibleDockWidget::childCount() const
{
    return dockWidget()->widget() ? 2 : 1;
}

int QAccessibleDockWidget::indexOfChild(const QAccessibleInterface *child) const
{
    if (child) {
        if (child->role(0) == TitleBar) {
            return 1;
        } else {
            return 2;   //###
        }
    }
    return -1;
}

QAccessible::Role QAccessibleDockWidget::role(int child) const
{
    switch (child) {
        case 0:
            return Window;
        case 1:
            return TitleBar;
        case 2:
            //###
            break;
        default:
            break;
    }
    return NoRole;
}

QAccessible::State QAccessibleDockWidget::state(int child) const
{
    //### mark tabified widgets as invisible
    return QAccessibleWidgetEx::state(child);
}

QRect QAccessibleDockWidget::rect (int child ) const
{
    QRect rect;
    bool mapToGlobal = true;
    if (child == 0) {
        if (dockWidget()->isFloating()) {
            rect = dockWidget()->frameGeometry();
            mapToGlobal = false;
        } else {
            rect = dockWidget()->rect();
        }
    }else if (child == 1) {
        QDockWidgetLayout *layout = qobject_cast<QDockWidgetLayout*>(dockWidget()->layout());
        rect = layout->titleArea();
    }else if (child == 2) {
        if (dockWidget()->widget())
            rect = dockWidget()->widget()->geometry();
    }
    if (rect.isNull())
        return rect;

    if (mapToGlobal)
        rect.moveTopLeft(dockWidget()->mapToGlobal(rect.topLeft()));

    return rect;
}

QVariant QAccessibleDockWidget::invokeMethodEx(QAccessible::Method, int, const QVariantList &)
{
    return QVariant();
}

QDockWidget *QAccessibleDockWidget::dockWidget() const
{
    return static_cast<QDockWidget *>(object());
}

////
//      QAccessibleTitleBar
////
QAccessibleTitleBar::QAccessibleTitleBar(QDockWidget *widget)
    : m_dockWidget(widget)
{

}

int QAccessibleTitleBar::navigate(RelationFlag relation, int entry, QAccessibleInterface **iface) const
{
    if (entry == 0 || relation == Self) {
        *iface = new QAccessibleTitleBar(dockWidget());
        return 0;
    }
    switch (relation) {
    case Child:
    case FocusChild:
        if (entry >= 1) {
            QDockWidgetLayout *layout = dockWidgetLayout();
            int index = 1;
            int role;
            for (role = QDockWidgetLayout::CloseButton; role <= QDockWidgetLayout::FloatButton; ++role) {
                QWidget *w = layout->widgetForRole((QDockWidgetLayout::Role)role);
                if (!w->isVisible())
                    continue;
                if (index == entry)
                    break;
                ++index;
            }
            *iface = 0;
            return role > QDockWidgetLayout::FloatButton ? -1 : index;
        }
        break;
    case Ancestor:
        {
        QAccessibleDockWidget *target = new QAccessibleDockWidget(dockWidget());
        int index;
        if (entry == 1) {
            *iface = target;
            return 0;
        }
        index = target->navigate(Ancestor, entry - 1, iface);
        delete target;
        return index;

        break;}
    case Sibling:
        return navigate(Child, entry, iface);
        break;
    default:
        break;
    }
    *iface = 0;
    return -1;
}

QAccessible::Relation QAccessibleTitleBar::relationTo(int /*child*/,  const QAccessibleInterface * /*other*/, int /*otherChild*/) const
{
    return Unrelated;   //###
}

int QAccessibleTitleBar::indexOfChild(const QAccessibleInterface * /*child*/) const
{
    return -1;
}

int QAccessibleTitleBar::childCount() const
{
    QDockWidgetLayout *layout = dockWidgetLayout();
    int count = 0;
    for (int role = QDockWidgetLayout::CloseButton; role <= QDockWidgetLayout::FloatButton; ++role) {
        QWidget *w = layout->widgetForRole((QDockWidgetLayout::Role)role);
        if (w && w->isVisible())
            ++count;
    }
    return count;
}

QString QAccessibleTitleBar::text(Text t, int child) const
{
    if (!child) {
        if (t == Name || t == Value) {
            return qt_accStripAmp(dockWidget()->windowTitle());
        }
    }
    return QString();
}

QAccessible::State QAccessibleTitleBar::state(int child) const
{
    QAccessible::State state = Normal;
    if (child) {
        QDockWidgetLayout *layout = dockWidgetLayout();
        QAbstractButton *b = static_cast<QAbstractButton *>(layout->widgetForRole((QDockWidgetLayout::Role)child));
        if (b) {
            if (b->isDown())
                state |= Pressed;
        }
    } else {
        QDockWidget *w = dockWidget();
        if (w->testAttribute(Qt::WA_WState_Visible) == false)
            state |= Invisible;
        if (w->focusPolicy() != Qt::NoFocus && w->isActiveWindow())
            state |= Focusable;
        if (w->hasFocus())
            state |= Focused;
        if (!w->isEnabled())
            state |= Unavailable;
    }

    return state;
}

QRect QAccessibleTitleBar::rect(int child) const
{
    bool mapToGlobal = true;
    QRect rect;
    if (child == 0) {
        if (dockWidget()->isFloating()) {
            rect = dockWidget()->frameGeometry();
            if (dockWidget()->widget()) {
                QPoint globalPos = dockWidget()->mapToGlobal(dockWidget()->widget()->rect().topLeft());
                globalPos.ry()--;
                rect.setBottom(globalPos.y());
                mapToGlobal = false;
            }
        } else {
            QDockWidgetLayout *layout = qobject_cast<QDockWidgetLayout*>(dockWidget()->layout());
            rect = layout->titleArea();
        }
    }else if (child >= 1 && child <= childCount()) {
        QDockWidgetLayout *layout = dockWidgetLayout();
        int index = 1;
        for (int role = QDockWidgetLayout::CloseButton; role <= QDockWidgetLayout::FloatButton; ++role) {
            QWidget *w = layout->widgetForRole((QDockWidgetLayout::Role)role);
            if (!w || !w->isVisible())
                continue;
            if (index == child) {
                rect = w->geometry();
                break;
            }
            ++index;
        }
    }
    if (rect.isNull())
        return rect;

    if (mapToGlobal)
        rect.moveTopLeft(dockWidget()->mapToGlobal(rect.topLeft()));
    return rect;
}

int QAccessibleTitleBar::childAt(int x, int y) const
{
    for (int i = childCount(); i >= 0; --i) {
        if (rect(i).contains(x,y))
            return i;
    }
    return -1;
}

QObject *QAccessibleTitleBar::object() const
{
    return 0;
}

QDockWidgetLayout *QAccessibleTitleBar::dockWidgetLayout() const
{
    return qobject_cast<QDockWidgetLayout*>(dockWidget()->layout());
}

QDockWidget *QAccessibleTitleBar::dockWidget() const
{
    return m_dockWidget;
}

QString QAccessibleTitleBar::actionText(int action, Text t, int child) const
{
    QString str;
    if (child >= 1 && child <= childCount()) {
        if (t == Name) {
            switch (action) {
            case Press:
            case DefaultAction:
                if (child == QDockWidgetLayout::CloseButton) {
                    str = QDockWidget::tr("Close");
                } else if (child == QDockWidgetLayout::FloatButton) {
                    str = dockWidget()->isFloating() ? QDockWidget::tr("Dock")
                                                     : QDockWidget::tr("Float");
                }
                break;
            default:
                break;
            }
        }
    }
    return str;
}

bool QAccessibleTitleBar::doAction(int action, int child, const QVariantList& /*params*/)
{
    if (!child || !dockWidget()->isEnabled())
        return false;

    switch (action) {
    case DefaultAction:
    case Press: {
        QDockWidgetLayout *layout = dockWidgetLayout();
        QAbstractButton *btn = static_cast<QAbstractButton *>(layout->widgetForRole((QDockWidgetLayout::Role)child));
        if (btn)
            btn->animateClick();
        return true;
        break;}
    default:
        break;
    }

    return false;
}

int QAccessibleTitleBar::userActionCount (int /*child*/) const
{
    return 0;
}

QAccessible::Role QAccessibleTitleBar::role(int child) const
{
    switch (child) {
        case 0:
            return TitleBar;
            break;
        default:
            if (child >= 1 && child <= childCount())
                return PushButton;
            break;
    }

    return NoRole;
}

void QAccessibleTitleBar::setText(Text /*t*/, int /*child*/, const QString &/*text*/)
{

}

bool QAccessibleTitleBar::isValid() const
{
    return dockWidget();
}

#endif // QT_NO_DOCKWIDGET

#ifndef QT_NO_TEXTEDIT
void QAccessibleTextEdit::addSelection(int startOffset, int endOffset)
{
    setSelection(0, startOffset, endOffset);
}

QString QAccessibleTextEdit::attributes(int offset, int *startOffset, int *endOffset)
{
    /* The list of attributes can be found at:
     http://linuxfoundation.org/collaborate/workgroups/accessibility/iaccessible2/textattributes
    */

    if (offset >= characterCount()) {
        *startOffset = -1;
        *endOffset = -1;
        return QString();
    }

    QMap<QString, QString> attrs;

    QTextCursor cursor = textEdit()->textCursor();

    //cursor.charFormat returns the format of the previous character
    cursor.setPosition(offset + 1);
    QTextCharFormat charFormat = cursor.charFormat();

    cursor.setPosition(offset);
    QTextBlockFormat blockFormat = cursor.blockFormat();

    QTextCharFormat charFormatComp;
    QTextBlockFormat blockFormatComp;

    *startOffset = offset;
    cursor.setPosition(*startOffset);
    while (*startOffset > 0) {
        charFormatComp = cursor.charFormat();
        cursor.setPosition(*startOffset - 1);
        blockFormatComp = cursor.blockFormat();
        if ((charFormat == charFormatComp) && (blockFormat == blockFormatComp))
            (*startOffset)--;
        else
            break;
    }

    int limit = characterCount() + 1;
    *endOffset = offset + 1;
    cursor.setPosition(*endOffset);
    while (*endOffset < limit) {
        blockFormatComp = cursor.blockFormat();
        cursor.setPosition(*endOffset + 1);
        charFormatComp = cursor.charFormat();
        if ((charFormat == charFormatComp) && (cursor.blockFormat() == blockFormatComp))
            (*endOffset)++;
        else
            break;
    }

    QString family = charFormat.fontFamily();
    if (!family.isEmpty()) {
        family = family.replace('\\',"\\\\");
        family = family.replace(':',"\\:");
        family = family.replace(',',"\\,");
        family = family.replace('=',"\\=");
        family = family.replace(';',"\\;");
        family = family.replace('\"',"\\\"");
        attrs["font-family"] = '"'+family+'"';
    }

    int fontSize = int(charFormat.fontPointSize());
    if (fontSize)
        attrs["font-size"] = QString::number(fontSize).append("pt");

    //Different weight values are not handled
    attrs["font-weight"] = (charFormat.fontWeight() > QFont::Normal) ? "bold" : "normal";

    QFont::Style style = charFormat.font().style();
    attrs["font-style"] = (style == QFont::StyleItalic) ? "italic" : ((style == QFont::StyleOblique) ? "oblique": "normal");

    attrs["text-underline-style"] = charFormat.font().underline() ? "solid" : "none";

    QTextCharFormat::VerticalAlignment alignment = charFormat.verticalAlignment();
    attrs["text-position"] = (alignment == QTextCharFormat::AlignSubScript) ? "sub" : ((alignment == QTextCharFormat::AlignSuperScript) ? "super" : "baseline" );

    QBrush background = charFormat.background();
    if (background.style() == Qt::SolidPattern) {
        attrs["background-color"] = QString("rgb(%1,%2,%3)").arg(background.color().red()).arg(background.color().green()).arg(background.color().blue());
    }

    QBrush foreground = charFormat.foreground();
    if (foreground.style() == Qt::SolidPattern) {
        attrs["color"] = QString("rgb(%1,%2,%3)").arg(foreground.color().red()).arg(foreground.color().green()).arg(foreground.color().blue());
    }

    switch (blockFormat.alignment() & (Qt::AlignLeft | Qt::AlignRight | Qt::AlignHCenter | Qt::AlignJustify)) {
    case Qt::AlignLeft:
        attrs["text-align"] = "left";
        break;
    case Qt::AlignRight:
        attrs["text-align"] = "right";
        break;
    case Qt::AlignHCenter:
        attrs["text-align"] = "center";
        break;
    case Qt::AlignJustify:
        attrs["text-align"] = "left";
        break;
    }

    QString result;
    foreach (const QString &attributeName, attrs.keys()) {
        result.append(attributeName).append(':').append(attrs[attributeName]).append(';');
    }

    return result;
}

int QAccessibleTextEdit::cursorPosition()
{
    return textEdit()->textCursor().position();
}

QRect QAccessibleTextEdit::characterRect(int offset, CoordinateType coordType)
{
    QTextEdit *edit = textEdit();
    QTextCursor cursor(edit->document());
    cursor.setPosition(offset);

    if (cursor.position() != offset)
        return QRect();

    QRect r = edit->cursorRect(cursor);
    if (cursor.movePosition(QTextCursor::NextCharacter)) {
        r.setWidth(edit->cursorRect(cursor).x() - r.x());
    } else {
        // we don't know the width of the character - maybe because we're at document end
        // in that case, IAccessible2 tells us to return the width of a default character
        int averageCharWidth = QFontMetrics(cursor.charFormat().font()).averageCharWidth();
        if (edit->layoutDirection() == Qt::RightToLeft)
            averageCharWidth *= -1;
        r.setWidth(averageCharWidth);
    }

    switch (coordType) {
    case RelativeToScreen:
        r.moveTo(edit->viewport()->mapToGlobal(r.topLeft()));
        break;
    case RelativeToParent:
        break;
    }

    return r;
}

int QAccessibleTextEdit::selectionCount()
{
    return textEdit()->textCursor().hasSelection() ? 1 : 0;
}

int QAccessibleTextEdit::offsetAtPoint(const QPoint &point, CoordinateType coordType)
{
    QTextEdit *edit = textEdit();

    QPoint p = point;
    if (coordType == RelativeToScreen)
        p = edit->viewport()->mapFromGlobal(p);
    // convert to document coordinates
    p += QPoint(edit->horizontalScrollBar()->value(), edit->verticalScrollBar()->value());

    return edit->document()->documentLayout()->hitTest(p, Qt::ExactHit);
}

void QAccessibleTextEdit::selection(int selectionIndex, int *startOffset, int *endOffset)
{
    *startOffset = *endOffset = 0;
    QTextCursor cursor = textEdit()->textCursor();

    if (selectionIndex != 0 || !cursor.hasSelection())
        return;

    *startOffset = cursor.selectionStart();
    *endOffset = cursor.selectionEnd();
}

QString QAccessibleTextEdit::text(int startOffset, int endOffset)
{
    QTextCursor cursor(textEdit()->document());

    cursor.setPosition(startOffset, QTextCursor::MoveAnchor);
    cursor.setPosition(endOffset, QTextCursor::KeepAnchor);

    return cursor.selectedText();
}

QString QAccessibleTextEdit::textBeforeOffset (int offset, BoundaryType boundaryType,
        int *startOffset, int *endOffset)
{
    // TODO - what exactly is before?
    Q_UNUSED(offset);
    Q_UNUSED(boundaryType);
    Q_UNUSED(startOffset);
    Q_UNUSED(endOffset);
    return QString();
}

QString QAccessibleTextEdit::textAfterOffset(int offset, BoundaryType boundaryType,
        int *startOffset, int *endOffset)
{
    // TODO - what exactly is after?
    Q_UNUSED(offset);
    Q_UNUSED(boundaryType);
    Q_UNUSED(startOffset);
    Q_UNUSED(endOffset);
    return QString();
}

QString QAccessibleTextEdit::textAtOffset(int offset, BoundaryType boundaryType,
                                          int *startOffset, int *endOffset)
{
    Q_ASSERT(startOffset);
    Q_ASSERT(endOffset);

    *startOffset = *endOffset = -1;
    QTextEdit *edit = textEdit();

    QTextCursor cursor(edit->document());
    if (offset >= characterCount())
        return QString();

    cursor.setPosition(offset);
    switch (boundaryType) {
    case CharBoundary:
        *startOffset = cursor.position();
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        *endOffset = cursor.position();
        break;
    case WordBoundary:
        cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
        *startOffset = cursor.position();
        cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
        *endOffset = cursor.position();
        break;
    case SentenceBoundary:
        // TODO - what's a sentence?
        return QString();
    case LineBoundary:
        cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
        *startOffset = cursor.position();
        cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        *endOffset = cursor.position();
        break;
    case ParagraphBoundary:
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        *startOffset = cursor.position();
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        *endOffset = cursor.position();
        break;
    case NoBoundary: {
        *startOffset = 0;
        const QString txt = edit->toPlainText();
        *endOffset = txt.count();
        return txt; }
    default:
        qDebug("AccessibleTextAdaptor::textAtOffset: Unknown boundary type %d", boundaryType);
        return QString();
    }

    return cursor.selectedText();
}

void QAccessibleTextEdit::removeSelection(int selectionIndex)
{
    if (selectionIndex != 0)
        return;

    QTextCursor cursor = textEdit()->textCursor();
    cursor.clearSelection();
    textEdit()->setTextCursor(cursor);
}

void QAccessibleTextEdit::setCursorPosition(int position)
{
    QTextCursor cursor = textEdit()->textCursor();
    cursor.setPosition(position);
    textEdit()->setTextCursor(cursor);
}

void QAccessibleTextEdit::setSelection(int selectionIndex, int startOffset, int endOffset)
{
    if (selectionIndex != 0)
        return;

    QTextCursor cursor = textEdit()->textCursor();
    cursor.setPosition(startOffset, QTextCursor::MoveAnchor);
    cursor.setPosition(endOffset, QTextCursor::KeepAnchor);
    textEdit()->setTextCursor(cursor);
}

int QAccessibleTextEdit::characterCount()
{
    return textEdit()->toPlainText().count();
}

void QAccessibleTextEdit::scrollToSubstring(int startIndex, int endIndex)
{
    QTextEdit *edit = textEdit();

    QTextCursor cursor(edit->document());
    cursor.setPosition(startIndex);
    QRect r = edit->cursorRect(cursor);

    cursor.setPosition(endIndex);
    r.setBottomRight(edit->cursorRect(cursor).bottomRight());

    r.moveTo(r.x() + edit->horizontalScrollBar()->value(),
             r.y() + edit->verticalScrollBar()->value());

    // E V I L, but ensureVisible is not public
    if (!QMetaObject::invokeMethod(edit, "_q_ensureVisible", Q_ARG(QRectF, r)))
        qWarning("AccessibleTextEdit::scrollToSubstring failed!");
}

static QTextCursor cursorForRange(QTextEdit *textEdit, int startOffset, int endOffset)
{
    QTextCursor cursor(textEdit->document());
    cursor.setPosition(startOffset, QTextCursor::MoveAnchor);
    cursor.setPosition(endOffset, QTextCursor::KeepAnchor);

    return cursor;
}

void QAccessibleTextEdit::copyText(int startOffset, int endOffset)
{
    QTextCursor cursor = cursorForRange(textEdit(), startOffset, endOffset);

    if (!cursor.hasSelection())
        return;

//     QApplication::clipboard()->setMimeData(new QTextEditMimeData(cursor.selection()));
}

void QAccessibleTextEdit::deleteText(int startOffset, int endOffset)
{
    QTextCursor cursor = cursorForRange(textEdit(), startOffset, endOffset);

    cursor.removeSelectedText();
}

void QAccessibleTextEdit::insertText(int offset, const QString &text)
{
    QTextCursor cursor(textEdit()->document());
    cursor.setPosition(offset);

    cursor.insertText(text);
}

void QAccessibleTextEdit::cutText(int startOffset, int endOffset)
{
    QTextCursor cursor = cursorForRange(textEdit(), startOffset, endOffset);

    if (!cursor.hasSelection())
        return;

//     QApplication::clipboard()->setMimeData(new QTextEditMimeData(cursor.selection()));
    cursor.removeSelectedText();
}

void QAccessibleTextEdit::pasteText(int offset)
{
    QTextEdit *edit = textEdit();

    QTextCursor oldCursor = edit->textCursor();
    QTextCursor newCursor = oldCursor;
    newCursor.setPosition(offset);

    edit->setTextCursor(newCursor);
#ifndef QT_NO_CLIPBOARD
    edit->paste();
#endif
    edit->setTextCursor(oldCursor);
}

void QAccessibleTextEdit::replaceText(int startOffset, int endOffset, const QString &text)
{
    QTextCursor cursor = cursorForRange(textEdit(), startOffset, endOffset);

    cursor.removeSelectedText();
    cursor.insertText(text);
}

void QAccessibleTextEdit::setAttributes(int startOffset, int endOffset, const QString &attributes)
{
    // TODO
    Q_UNUSED(startOffset);
    Q_UNUSED(endOffset);
    Q_UNUSED(attributes);
}

#endif // QT_NO_TEXTEDIT

#ifndef QT_NO_MAINWINDOW
QAccessibleMainWindow::QAccessibleMainWindow(QWidget *widget)
    : QAccessibleWidgetEx(widget, Window) { }

QVariant QAccessibleMainWindow::invokeMethodEx(QAccessible::Method /*method*/, int /*child*/, const QVariantList & /*params*/)
{
    return QVariant();
}

int QAccessibleMainWindow::childCount() const
{
    QList<QWidget*> kids = childWidgets(mainWindow(), true);
    return kids.count();
}

int QAccessibleMainWindow::indexOfChild(const QAccessibleInterface *iface) const
{
    QList<QWidget*> kids = childWidgets(mainWindow(), true);
    int childIndex = kids.indexOf(static_cast<QWidget*>(iface->object()));
    return childIndex == -1 ? -1 : ++childIndex;
}

int QAccessibleMainWindow::navigate(RelationFlag relation, int entry, QAccessibleInterface **iface) const
{
    if (relation == Child && entry >= 1) {
        QList<QWidget*> kids = childWidgets(mainWindow(), true);
        if (entry <= kids.count()) {
            *iface = QAccessible::queryAccessibleInterface(kids.at(entry - 1));
            return *iface ? 0 : -1;
        }
    }
    return QAccessibleWidgetEx::navigate(relation, entry, iface);
}

int QAccessibleMainWindow::childAt(int x, int y) const
{
    QWidget *w = widget();
    if (!w->isVisible())
        return -1;
    QPoint gp = w->mapToGlobal(QPoint(0, 0));
    if (!QRect(gp.x(), gp.y(), w->width(), w->height()).contains(x, y))
        return -1;

    QWidgetList kids = childWidgets(mainWindow(), true);
    QPoint rp = mainWindow()->mapFromGlobal(QPoint(x, y));
    for (int i = 0; i < kids.size(); ++i) {
        QWidget *child = kids.at(i);
        if (!child->isWindow() && !child->isHidden() && child->geometry().contains(rp)) {
            return i + 1;
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
