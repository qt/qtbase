/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qlineedit.h"
#include "qlineedit_p.h"

#include "qvariant.h"
#if QT_CONFIG(itemviews)
#include "qabstractitemview.h"
#endif
#if QT_CONFIG(draganddrop)
#include "qdrag.h"
#endif
#include "qwidgetaction.h"
#include "qclipboard.h"
#ifndef QT_NO_ACCESSIBILITY
#include "qaccessible.h"
#endif
#ifndef QT_NO_IM
#include "qinputmethod.h"
#include "qlist.h"
#endif
#include <qpainter.h>
#if QT_CONFIG(animation)
#include <qpropertyanimation.h>
#endif
#include <qstylehints.h>
#include <qvalidator.h>

QT_BEGIN_NAMESPACE

const int QLineEditPrivate::verticalMargin(1);
const int QLineEditPrivate::horizontalMargin(2);

QRect QLineEditPrivate::adjustedControlRect(const QRect &rect) const
{
    QRect widgetRect = !rect.isEmpty() ? rect : q_func()->rect();
    QRect cr = adjustedContentsRect();
    int cix = cr.x() - hscroll + horizontalMargin;
    return widgetRect.translated(QPoint(cix, vscroll));
}

int QLineEditPrivate::xToPos(int x, QTextLine::CursorPosition betweenOrOn) const
{
    QRect cr = adjustedContentsRect();
    x-= cr.x() - hscroll + horizontalMargin;
    return control->xToPos(x, betweenOrOn);
}

bool QLineEditPrivate::inSelection(int x) const
{
    x -= adjustedContentsRect().x() - hscroll + horizontalMargin;
    return control->inSelection(x);
}

QRect QLineEditPrivate::cursorRect() const
{
    return adjustedControlRect(control->cursorRect());
}

#if QT_CONFIG(completer)

void QLineEditPrivate::_q_completionHighlighted(const QString &newText)
{
    Q_Q(QLineEdit);
    if (control->completer()->completionMode() != QCompleter::InlineCompletion) {
        q->setText(newText);
    } else {
        int c = control->cursor();
        QString text = control->text();
        q->setText(text.leftRef(c) + newText.midRef(c));
        control->moveCursor(control->end(), false);
#ifndef Q_OS_ANDROID
        const bool mark = true;
#else
        const bool mark = (imHints & Qt::ImhNoPredictiveText);
#endif
        control->moveCursor(c, mark);
    }
}

#endif // QT_CONFIG(completer)

void QLineEditPrivate::_q_handleWindowActivate()
{
    Q_Q(QLineEdit);
    if (!q->hasFocus() && control->hasSelectedText())
        control->deselect();
}

void QLineEditPrivate::_q_textEdited(const QString &text)
{
    Q_Q(QLineEdit);
    emit q->textEdited(text);
#if QT_CONFIG(completer)
    if (control->completer()
        && control->completer()->completionMode() != QCompleter::InlineCompletion)
        control->complete(-1); // update the popup on cut/paste/del
#endif
}

void QLineEditPrivate::_q_cursorPositionChanged(int from, int to)
{
    Q_Q(QLineEdit);
    q->update();
    emit q->cursorPositionChanged(from, to);
}

#ifdef QT_KEYPAD_NAVIGATION
void QLineEditPrivate::_q_editFocusChange(bool e)
{
    Q_Q(QLineEdit);
    q->setEditFocus(e);
}
#endif

void QLineEditPrivate::_q_selectionChanged()
{
    Q_Q(QLineEdit);
    if (control->preeditAreaText().isEmpty()) {
        QStyleOptionFrame opt;
        q->initStyleOption(&opt);
        bool showCursor = control->hasSelectedText() ?
                          q->style()->styleHint(QStyle::SH_BlinkCursorWhenTextSelected, &opt, q):
                          q->hasFocus();
        setCursorVisible(showCursor);
    }

    emit q->selectionChanged();
#ifndef QT_NO_ACCESSIBILITY
    QAccessibleTextSelectionEvent ev(q, control->selectionStart(), control->selectionEnd());
    ev.setCursorPosition(control->cursorPosition());
    QAccessible::updateAccessibility(&ev);
#endif
}

void QLineEditPrivate::_q_updateNeeded(const QRect &rect)
{
    q_func()->update(adjustedControlRect(rect));
}

void QLineEditPrivate::init(const QString& txt)
{
    Q_Q(QLineEdit);
    control = new QWidgetLineControl(txt);
    control->setParent(q);
    control->setFont(q->font());
    QObject::connect(control, SIGNAL(textChanged(QString)),
            q, SIGNAL(textChanged(QString)));
    QObject::connect(control, SIGNAL(textEdited(QString)),
            q, SLOT(_q_textEdited(QString)));
    QObject::connect(control, SIGNAL(cursorPositionChanged(int,int)),
            q, SLOT(_q_cursorPositionChanged(int,int)));
    QObject::connect(control, SIGNAL(selectionChanged()),
            q, SLOT(_q_selectionChanged()));
    QObject::connect(control, SIGNAL(accepted()),
            q, SIGNAL(returnPressed()));
    QObject::connect(control, SIGNAL(editingFinished()),
            q, SIGNAL(editingFinished()));
#ifdef QT_KEYPAD_NAVIGATION
    QObject::connect(control, SIGNAL(editFocusChange(bool)),
            q, SLOT(_q_editFocusChange(bool)));
#endif
    QObject::connect(control, SIGNAL(cursorPositionChanged(int,int)),
            q, SLOT(updateMicroFocus()));

    QObject::connect(control, SIGNAL(textChanged(QString)),
            q, SLOT(updateMicroFocus()));

    QObject::connect(control, SIGNAL(updateMicroFocus()),
            q, SLOT(updateMicroFocus()));

    // for now, going completely overboard with updates.
    QObject::connect(control, SIGNAL(selectionChanged()),
            q, SLOT(update()));

    QObject::connect(control, SIGNAL(selectionChanged()),
            q, SLOT(updateMicroFocus()));

    QObject::connect(control, SIGNAL(displayTextChanged(QString)),
            q, SLOT(update()));

    QObject::connect(control, SIGNAL(updateNeeded(QRect)),
            q, SLOT(_q_updateNeeded(QRect)));

    QStyleOptionFrame opt;
    q->initStyleOption(&opt);
    control->setPasswordCharacter(q->style()->styleHint(QStyle::SH_LineEdit_PasswordCharacter, &opt, q));
    control->setPasswordMaskDelay(q->style()->styleHint(QStyle::SH_LineEdit_PasswordMaskDelay, &opt, q));
#ifndef QT_NO_CURSOR
    q->setCursor(Qt::IBeamCursor);
#endif
    q->setFocusPolicy(Qt::StrongFocus);
    q->setAttribute(Qt::WA_InputMethodEnabled);
    //   Specifies that this widget can use more, but is able to survive on
    //   less, horizontal space; and is fixed vertically.
    q->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed, QSizePolicy::LineEdit));
    q->setBackgroundRole(QPalette::Base);
    q->setAttribute(Qt::WA_KeyCompression);
    q->setMouseTracking(true);
    q->setAcceptDrops(true);

    q->setAttribute(Qt::WA_MacShowFocusRect);

    initMouseYThreshold();
}

void QLineEditPrivate::initMouseYThreshold()
{
    mouseYThreshold = QGuiApplication::styleHints()->mouseQuickSelectionThreshold();
}

QRect QLineEditPrivate::adjustedContentsRect() const
{
    Q_Q(const QLineEdit);
    QStyleOptionFrame opt;
    q->initStyleOption(&opt);
    QRect r = q->style()->subElementRect(QStyle::SE_LineEditContents, &opt, q);
    r.setX(r.x() + effectiveLeftTextMargin());
    r.setY(r.y() + topTextMargin);
    r.setRight(r.right() - effectiveRightTextMargin());
    r.setBottom(r.bottom() - bottomTextMargin);
    return r;
}

void QLineEditPrivate::setCursorVisible(bool visible)
{
    Q_Q(QLineEdit);
    if ((bool)cursorVisible == visible)
        return;
    cursorVisible = visible;
    if (control->inputMask().isEmpty())
        q->update(cursorRect());
    else
        q->update();
}

void QLineEditPrivate::updatePasswordEchoEditing(bool editing)
{
    Q_Q(QLineEdit);
    control->updatePasswordEchoEditing(editing);
    q->setAttribute(Qt::WA_InputMethodEnabled, shouldEnableInputMethod());
}

void QLineEditPrivate::resetInputMethod()
{
    Q_Q(QLineEdit);
    if (q->hasFocus() && qApp) {
        QGuiApplication::inputMethod()->reset();
    }
}

/*!
  This function is not intended as polymorphic usage. Just a shared code
  fragment that calls QInputMethod::invokeAction for this
  class.
*/
bool QLineEditPrivate::sendMouseEventToInputContext( QMouseEvent *e )
{
#if !defined QT_NO_IM
    if ( control->composeMode() ) {
        int tmp_cursor = xToPos(e->pos().x());
        int mousePos = tmp_cursor - control->cursor();
        if ( mousePos < 0 || mousePos > control->preeditAreaText().length() )
            mousePos = -1;

        if (mousePos >= 0) {
            if (e->type() == QEvent::MouseButtonRelease)
                QGuiApplication::inputMethod()->invokeAction(QInputMethod::Click, mousePos);

            return true;
        }
    }
#else
    Q_UNUSED(e);
#endif

    return false;
}

#if QT_CONFIG(draganddrop)
void QLineEditPrivate::drag()
{
    Q_Q(QLineEdit);
    dndTimer.stop();
    QMimeData *data = new QMimeData;
    data->setText(control->selectedText());
    QDrag *drag = new QDrag(q);
    drag->setMimeData(data);
    Qt::DropAction action = drag->start();
    if (action == Qt::MoveAction && !control->isReadOnly() && drag->target() != q)
        control->removeSelection();
}
#endif // QT_CONFIG(draganddrop)


#if QT_CONFIG(toolbutton)
QLineEditIconButton::QLineEditIconButton(QWidget *parent)
    : QToolButton(parent)
    , m_opacity(0)
{
    setFocusPolicy(Qt::NoFocus);
}

QLineEditPrivate *QLineEditIconButton::lineEditPrivate() const
{
    QLineEdit *le = qobject_cast<QLineEdit *>(parentWidget());
    return le ? static_cast<QLineEditPrivate *>(qt_widget_private(le)) : nullptr;
}

void QLineEditIconButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QWindow *window = nullptr;
    if (const QWidget *nativeParent = nativeParentWidget())
        window = nativeParent->windowHandle();
    // Note isDown should really use the active state but in most styles
    // this has no proper feedback
    QIcon::Mode state = QIcon::Disabled;
    if (isEnabled())
        state = isDown() ? QIcon::Selected : QIcon::Normal;
    const QLineEditPrivate *lep = lineEditPrivate();
    const int iconWidth = lep ? lep->sideWidgetParameters().iconSize : 16;
    const QSize iconSize(iconWidth, iconWidth);
    const QPixmap iconPixmap = icon().pixmap(window, iconSize, state, QIcon::Off);
    QRect pixmapRect = QRect(QPoint(0, 0), iconSize);
    pixmapRect.moveCenter(rect().center());
    painter.setOpacity(m_opacity);
    painter.drawPixmap(pixmapRect, iconPixmap);
}

void QLineEditIconButton::actionEvent(QActionEvent *e)
{
    switch (e->type()) {
    case QEvent::ActionChanged: {
        const QAction *action = e->action();
        if (isVisibleTo(parentWidget()) != action->isVisible()) {
            setVisible(action->isVisible());
            if (QLineEditPrivate *lep = lineEditPrivate())
                lep->positionSideWidgets();
        }
    }
        break;
    default:
        break;
    }
    QToolButton::actionEvent(e);
}

void QLineEditIconButton::setOpacity(qreal value)
{
    if (!qFuzzyCompare(m_opacity, value)) {
        m_opacity = value;
        updateCursor();
        update();
    }
}

#if QT_CONFIG(animation)
void QLineEditIconButton::startOpacityAnimation(qreal endValue)
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, QByteArrayLiteral("opacity"));
    animation->setDuration(160);
    animation->setEndValue(endValue);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}
#endif

void QLineEditIconButton::updateCursor()
{
#ifndef QT_NO_CURSOR
    setCursor(qFuzzyCompare(m_opacity, qreal(1.0)) || !parentWidget() ? QCursor(Qt::ArrowCursor) : parentWidget()->cursor());
#endif
}
#endif // QT_CONFIG(toolbutton)

void QLineEditPrivate::_q_textChanged(const QString &text)
{
    if (hasSideWidgets()) {
        const int newTextSize = text.size();
        if (!newTextSize || !lastTextSize) {
            lastTextSize = newTextSize;
#if QT_CONFIG(animation) && QT_CONFIG(toolbutton)
            const bool fadeIn = newTextSize > 0;
            for (const SideWidgetEntry &e : leadingSideWidgets) {
                if (e.flags & SideWidgetFadeInWithText)
                   static_cast<QLineEditIconButton *>(e.widget)->animateShow(fadeIn);
            }
            for (const SideWidgetEntry &e : trailingSideWidgets) {
                if (e.flags & SideWidgetFadeInWithText)
                   static_cast<QLineEditIconButton *>(e.widget)->animateShow(fadeIn);
            }
#endif
        }
    }
}

void QLineEditPrivate::_q_clearButtonClicked()
{
    Q_Q(QLineEdit);
    if (!q->text().isEmpty()) {
        q->clear();
        emit q->textEdited(QString());
    }
}

QLineEditPrivate::SideWidgetParameters QLineEditPrivate::sideWidgetParameters() const
{
    Q_Q(const QLineEdit);
    SideWidgetParameters result;
    result.iconSize = q->height() < 34 ? 16 : 32;
    result.margin = result.iconSize / 4;
    result.widgetWidth = result.iconSize + 6;
    result.widgetHeight = result.iconSize + 2;
    return result;
}

QIcon QLineEditPrivate::clearButtonIcon() const
{
    Q_Q(const QLineEdit);
    QStyleOptionFrame styleOption;
    q->initStyleOption(&styleOption);
    return q->style()->standardIcon(QStyle::SP_LineEditClearButton, &styleOption, q);
}

void QLineEditPrivate::setClearButtonEnabled(bool enabled)
{
#if QT_CONFIG(action)
    for (const SideWidgetEntry &e : trailingSideWidgets) {
        if (e.flags & SideWidgetClearButton) {
            e.action->setEnabled(enabled);
            break;
        }
    }
#endif
}

void QLineEditPrivate::positionSideWidgets()
{
    Q_Q(QLineEdit);
    if (hasSideWidgets()) {
        const QRect contentRect = q->rect();
        const SideWidgetParameters p = sideWidgetParameters();
        const int delta = p.margin + p.widgetWidth;
        QRect widgetGeometry(QPoint(p.margin, (contentRect.height() - p.widgetHeight) / 2),
                             QSize(p.widgetWidth, p.widgetHeight));
        for (const SideWidgetEntry &e : leftSideWidgetList()) {
            e.widget->setGeometry(widgetGeometry);
#if QT_CONFIG(action)
            if (e.action->isVisible())
                widgetGeometry.moveLeft(widgetGeometry.left() + delta);
#endif
        }
        widgetGeometry.moveLeft(contentRect.width() - p.widgetWidth - p.margin);
        for (const SideWidgetEntry &e : rightSideWidgetList()) {
            e.widget->setGeometry(widgetGeometry);
#if QT_CONFIG(action)
            if (e.action->isVisible())
                widgetGeometry.moveLeft(widgetGeometry.left() - delta);
#endif
        }
    }
}

QLineEditPrivate::SideWidgetLocation QLineEditPrivate::findSideWidget(const QAction *a) const
{
    int i = 0;
    for (const auto &e : leadingSideWidgets) {
        if (a == e.action)
            return {QLineEdit::LeadingPosition, i};
        ++i;
    }
    i = 0;
    for (const auto &e : trailingSideWidgets) {
        if (a == e.action)
            return {QLineEdit::TrailingPosition, i};
        ++i;
    }
    return {QLineEdit::LeadingPosition, -1};
}

QWidget *QLineEditPrivate::addAction(QAction *newAction, QAction *before, QLineEdit::ActionPosition position, int flags)
{
    Q_Q(QLineEdit);
    if (!newAction)
        return 0;
    if (!hasSideWidgets()) { // initial setup.
        QObject::connect(q, SIGNAL(textChanged(QString)), q, SLOT(_q_textChanged(QString)));
        lastTextSize = q->text().size();
    }
    QWidget *w = 0;
    // Store flags about QWidgetAction here since removeAction() may be called from ~QAction,
    // in which a qobject_cast<> no longer works.
#if QT_CONFIG(action)
    if (QWidgetAction *widgetAction = qobject_cast<QWidgetAction *>(newAction)) {
        if ((w = widgetAction->requestWidget(q)))
            flags |= SideWidgetCreatedByWidgetAction;
    }
#endif
    if (!w) {
#if QT_CONFIG(toolbutton)
        QLineEditIconButton *toolButton = new QLineEditIconButton(q);
        toolButton->setIcon(newAction->icon());
        toolButton->setOpacity(lastTextSize > 0 || !(flags & SideWidgetFadeInWithText) ? 1 : 0);
        if (flags & SideWidgetClearButton)
            QObject::connect(toolButton, SIGNAL(clicked()), q, SLOT(_q_clearButtonClicked()));
        toolButton->setDefaultAction(newAction);
        w = toolButton;
#else
        return nullptr;
#endif
    }

    // QTBUG-59957: clear button should be the leftmost action.
    if (!before && !(flags & SideWidgetClearButton) && position == QLineEdit::TrailingPosition) {
        for (const SideWidgetEntry &e : trailingSideWidgets) {
            if (e.flags & SideWidgetClearButton) {
                before = e.action;
                break;
            }
        }
    }

    // If there is a 'before' action, it takes preference

    // There's a bug in GHS compiler that causes internal error on the following code.
    // The affected GHS compiler versions are 2016.5.4 and 2017.1. GHS internal reference
    // to track the progress of this issue is TOOLS-26637.
    // This temporary workaround allows to compile with GHS toolchain and should be
    // removed when GHS provides a patch to fix the compiler issue.

#if defined(Q_CC_GHS)
    const SideWidgetLocation loc = {position, -1};
    const auto location = before ? findSideWidget(before) : loc;
#else
    const auto location = before ? findSideWidget(before) : SideWidgetLocation{position, -1};
#endif

    SideWidgetEntryList &list = location.position == QLineEdit::TrailingPosition ? trailingSideWidgets : leadingSideWidgets;
    list.insert(location.isValid() ? list.begin() + location.index : list.end(),
                SideWidgetEntry(w, newAction, flags));
    positionSideWidgets();
    w->show();
    return w;
}

void QLineEditPrivate::removeAction(QAction *action)
{
#if QT_CONFIG(action)
    Q_Q(QLineEdit);
    const auto location = findSideWidget(action);
    if (!location.isValid())
        return;
    SideWidgetEntryList &list = location.position == QLineEdit::TrailingPosition ? trailingSideWidgets : leadingSideWidgets;
    SideWidgetEntry entry = list[location.index];
    list.erase(list.begin() + location.index);
     if (entry.flags & SideWidgetCreatedByWidgetAction)
         static_cast<QWidgetAction *>(entry.action)->releaseWidget(entry.widget);
     else
         delete entry.widget;
     positionSideWidgets();
     if (!hasSideWidgets()) // Last widget, remove connection
         QObject::disconnect(q, SIGNAL(textChanged(QString)), q, SLOT(_q_textChanged(QString)));
     q->update();
#endif // QT_CONFIG(action)
}

static bool isSideWidgetVisible(const QLineEditPrivate::SideWidgetEntry &e)
{
   return e.widget->isVisible();
}

int QLineEditPrivate::effectiveLeftTextMargin() const
{
    int result = leftTextMargin;
    if (!leftSideWidgetList().empty()) {
        const SideWidgetParameters p = sideWidgetParameters();
        result += (p.margin + p.widgetWidth)
            * int(std::count_if(leftSideWidgetList().begin(), leftSideWidgetList().end(),
                                isSideWidgetVisible));
    }
    return result;
}

int QLineEditPrivate::effectiveRightTextMargin() const
{
    int result = rightTextMargin;
    if (!rightSideWidgetList().empty()) {
        const SideWidgetParameters p = sideWidgetParameters();
        result += (p.margin + p.widgetWidth)
            * int(std::count_if(rightSideWidgetList().begin(), rightSideWidgetList().end(),
                                isSideWidgetVisible));
    }
    return result;
}


QT_END_NAMESPACE

#include "moc_qlineedit_p.cpp"
