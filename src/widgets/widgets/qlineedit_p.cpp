// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qlineedit.h"
#include "qlineedit_p.h"

#include "qvariant.h"
#if QT_CONFIG(itemviews)
#include "qabstractitemview.h"
#endif
#if QT_CONFIG(draganddrop)
#include "qdrag.h"
#endif
#if QT_CONFIG(action)
#  include "qwidgetaction.h"
#endif
#include "qclipboard.h"
#if QT_CONFIG(accessibility)
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

// Needs to be kept in sync with QLineEdit::paintEvent
QRect QLineEditPrivate::adjustedControlRect(const QRect &rect) const
{
    QRect widgetRect = !rect.isEmpty() ? rect : q_func()->rect();
    QRect cr = adjustedContentsRect();
    int cix = cr.x() - hscroll + horizontalMargin;
    return widgetRect.translated(QPoint(cix, vscroll - control->ascent() + q_func()->fontMetrics().ascent()));
}

int QLineEditPrivate::xToPos(int x, QTextLine::CursorPosition betweenOrOn) const
{
    QRect cr = adjustedContentsRect();
    x-= cr.x() - hscroll + horizontalMargin;
    return control->xToPos(x, betweenOrOn);
}

QString QLineEditPrivate::textBeforeCursor(int curPos) const
{
    const QString &text = control->text();
    return text.mid(0, curPos);
}

QString QLineEditPrivate::textAfterCursor(int curPos) const
{
    const QString &text = control->text();
    return text.mid(curPos);
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
void QLineEditPrivate::connectCompleter()
{
    Q_Q(const QLineEdit);
    QObject::connect(control->completer(), qOverload<const QString &>(&QCompleter::activated),
                     q, &QLineEdit::setText);
    QObjectPrivate::connect(control->completer(), qOverload<const QString &>(&QCompleter::highlighted),
                            this, &QLineEditPrivate::completionHighlighted);
}

void QLineEditPrivate::disconnectCompleter()
{
    Q_Q(const QLineEdit);
    QObject::disconnect(control->completer(), qOverload<const QString &>(&QCompleter::activated),
                        q, &QLineEdit::setText);
    QObjectPrivate::disconnect(control->completer(), qOverload<const QString &>(&QCompleter::highlighted),
                               this, &QLineEditPrivate::completionHighlighted);
}

void QLineEditPrivate::completionHighlighted(const QString &newText)
{
    Q_Q(QLineEdit);
    if (control->completer()->completionMode() != QCompleter::InlineCompletion) {
        q->setText(newText);
    } else {
        int c = control->cursor();
        QString text = control->text();
        q->setText(QStringView{text}.left(c) + QStringView{newText}.mid(c));
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

void QLineEditPrivate::handleWindowActivate()
{
    Q_Q(QLineEdit);
    if (!q->hasFocus() && control->hasSelectedText())
        control->deselect();
}

void QLineEditPrivate::textEdited(const QString &text)
{
    Q_Q(QLineEdit);
    edited = true;
    emit q->textEdited(text);
#if QT_CONFIG(completer)
    if (control->completer()
        && control->completer()->completionMode() != QCompleter::InlineCompletion)
        control->complete(-1); // update the popup on cut/paste/del
#endif
}

void QLineEditPrivate::cursorPositionChanged(int from, int to)
{
    Q_Q(QLineEdit);
    q->update();
    emit q->cursorPositionChanged(from, to);
}

#ifdef QT_KEYPAD_NAVIGATION
void QLineEditPrivate::editFocusChange(bool e)
{
    Q_Q(QLineEdit);
    q->setEditFocus(e);
}
#endif

void QLineEditPrivate::selectionChanged()
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
#if QT_CONFIG(accessibility)
    QAccessibleTextSelectionEvent ev(q, control->selectionStart(), control->selectionEnd());
    ev.setCursorPosition(control->cursorPosition());
    QAccessible::updateAccessibility(&ev);
#endif
}

void QLineEditPrivate::updateNeeded(const QRect &rect)
{
    q_func()->update(adjustedControlRect(rect));
}

void QLineEditPrivate::init(const QString& txt)
{
    Q_Q(QLineEdit);

    const auto qUpdateMicroFocus = [q]()
    {
        q->updateMicroFocus();
    };
    control = new QWidgetLineControl(txt);
    control->setParent(q);
    control->setFont(q->font());
    QObject::connect(control, &QWidgetLineControl::textChanged,
                     q, &QLineEdit::textChanged);
    QObjectPrivate::connect(control, &QWidgetLineControl::textEdited,
                            this, &QLineEditPrivate::textEdited);
    QObjectPrivate::connect(control, &QWidgetLineControl::cursorPositionChanged,
                            this, &QLineEditPrivate::cursorPositionChanged);
    QObjectPrivate::connect(control, &QWidgetLineControl::selectionChanged,
                            this, &QLineEditPrivate::selectionChanged);
    QObjectPrivate::connect(control, &QWidgetLineControl::editingFinished,
                            this, &QLineEditPrivate::controlEditingFinished);
#ifdef QT_KEYPAD_NAVIGATION
    QObject::connect(control, &QWidgetLineControl::editFocusChange,
                     this, &QLineEditPrivate::editFocusChange);
#endif
    QObject::connect(control, &QWidgetLineControl::cursorPositionChanged,
                     q, qUpdateMicroFocus);

    QObject::connect(control, &QWidgetLineControl::textChanged,
                     q, qUpdateMicroFocus);

    QObject::connect(control, &QWidgetLineControl::updateMicroFocus,
                     q, qUpdateMicroFocus);

    // for now, going completely overboard with updates.
    QObject::connect(control, &QWidgetLineControl::selectionChanged,
                     q, qOverload<>(&QLineEdit::update));

    QObject::connect(control, &QWidgetLineControl::selectionChanged,
                     q, qUpdateMicroFocus);

    QObject::connect(control, &QWidgetLineControl::displayTextChanged,
                     q, qOverload<>(&QLineEdit::update));

    QObjectPrivate::connect(control, &QWidgetLineControl::updateNeeded,
                            this, &QLineEditPrivate::updateNeeded);
    QObject::connect(control, &QWidgetLineControl::inputRejected,
                     q, &QLineEdit::inputRejected);

    QStyleOptionFrame opt;
    q->initStyleOption(&opt);
    control->setPasswordCharacter(char16_t(q->style()->styleHint(QStyle::SH_LineEdit_PasswordCharacter, &opt, q)));
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
    r = r.marginsRemoved(effectiveTextMargins());
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

void QLineEditPrivate::setText(const QString& text)
{
    edited = true;
    control->setText(text);
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
        int tmp_cursor = xToPos(e->position().toPoint().x());
        int mousePos = tmp_cursor - control->cursor();
        if ( mousePos < 0 || mousePos > control->preeditAreaText().size() )
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
    Qt::DropAction action = drag->exec(Qt::CopyAction);
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
    QIcon::Mode state = QIcon::Disabled;
    if (isEnabled())
        state = isDown() ? QIcon::Active : QIcon::Normal;
    const QLineEditPrivate *lep = lineEditPrivate();
    const int iconWidth = lep ? lep->sideWidgetParameters().iconSize : 16;
    const QSize iconSize(iconWidth, iconWidth);
    const QPixmap iconPixmap = icon().pixmap(iconSize, devicePixelRatio(), state, QIcon::Off);
    QRect pixmapRect = QRect(QPoint(0, 0), iconSize);
    pixmapRect.moveCenter(rect().center());
    painter.setOpacity(m_opacity);
    painter.drawPixmap(pixmapRect, iconPixmap);
}

void QLineEditIconButton::actionEvent(QActionEvent *e)
{
    switch (e->type()) {
    case QEvent::ActionChanged: {
        const auto *action = e->action();
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
bool QLineEditIconButton::shouldHideWithText() const
{
    return m_hideWithText;
}

void QLineEditIconButton::setHideWithText(bool hide)
{
    m_hideWithText = hide;
}

void QLineEditIconButton::onAnimationFinished()
{
    if (shouldHideWithText() && isVisible() && m_fadingOut) {
        hide();
        m_fadingOut = false;

        // Invalidate previous geometry to take into account new size of side widgets
        if (auto le = lineEditPrivate())
            le->updateGeometry_helper(true);
    }
}

void QLineEditIconButton::animateShow(bool visible)
{
    m_fadingOut = !visible;

    if (shouldHideWithText() && !isVisible()) {
        show();

        // Invalidate previous geometry to take into account new size of side widgets
        if (auto le = lineEditPrivate())
            le->updateGeometry_helper(true);
    }

    startOpacityAnimation(visible ? 1.0 : 0.0);
}

void QLineEditIconButton::startOpacityAnimation(qreal endValue)
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, QByteArrayLiteral("opacity"), this);
    connect(animation, &QPropertyAnimation::finished, this, &QLineEditIconButton::onAnimationFinished);

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

#if QT_CONFIG(animation) && QT_CONFIG(toolbutton)
static void displayWidgets(const QLineEditPrivate::SideWidgetEntryList &widgets, bool display)
{
    for (const auto &e : widgets) {
        if (e.flags & QLineEditPrivate::SideWidgetFadeInWithText)
            static_cast<QLineEditIconButton *>(e.widget)->animateShow(display);
    }
}
#endif

void QLineEditPrivate::textChanged(const QString &text)
{
    if (hasSideWidgets()) {
        const int newTextSize = text.size();
        if (!newTextSize || !lastTextSize) {
            lastTextSize = newTextSize;
#if QT_CONFIG(animation) && QT_CONFIG(toolbutton)
            const bool display = newTextSize > 0;
            displayWidgets(leadingSideWidgets, display);
            displayWidgets(trailingSideWidgets, display);
#endif
        }
    }
}

void QLineEditPrivate::clearButtonClicked()
{
    Q_Q(QLineEdit);
    if (!q->text().isEmpty()) {
        q->clear();
        textEdited(QString());
    }
}

void QLineEditPrivate::controlEditingFinished()
{
    Q_Q(QLineEdit);
    edited = false;
    emit q->returnPressed();
    emit q->editingFinished();
}

QLineEditPrivate::SideWidgetParameters QLineEditPrivate::sideWidgetParameters() const
{
    Q_Q(const QLineEdit);
    SideWidgetParameters result;
    result.iconSize = q->style()->pixelMetric(QStyle::PM_LineEditIconSize, nullptr, q);
    result.margin = q->style()->pixelMetric(QStyle::PM_LineEditIconMargin, nullptr, q);
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
#else
    Q_UNUSED(enabled);
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
#else
            Q_UNUSED(delta);
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

#if QT_CONFIG(action)
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
        return nullptr;
    if (!hasSideWidgets()) { // initial setup.
        QObjectPrivate::connect(q, &QLineEdit::textChanged,
                                this, &QLineEditPrivate::textChanged);
        lastTextSize = q->text().size();
    }
    QWidget *w = nullptr;
    // Store flags about QWidgetAction here since removeAction() may be called from ~QAction,
    // in which a qobject_cast<> no longer works.
    if (QWidgetAction *widgetAction = qobject_cast<QWidgetAction *>(newAction)) {
        if ((w = widgetAction->requestWidget(q)))
            flags |= SideWidgetCreatedByWidgetAction;
    }
    if (!w) {
#if QT_CONFIG(toolbutton)
        QLineEditIconButton *toolButton = new QLineEditIconButton(q);
        toolButton->setIcon(newAction->icon());
        toolButton->setOpacity(lastTextSize > 0 || !(flags & SideWidgetFadeInWithText) ? 1 : 0);
        if (flags & SideWidgetClearButton) {
            QObjectPrivate::connect(toolButton, &QToolButton::clicked,
                                    this, &QLineEditPrivate::clearButtonClicked);

#if QT_CONFIG(animation)
            // The clear button is handled only by this widget. The button should be really
            // shown/hidden in order to calculate size hints correctly.
            toolButton->setHideWithText(true);
#endif
        }
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
    Q_Q(QLineEdit);
    const auto location = findSideWidget(action);
    if (!location.isValid())
        return;
    SideWidgetEntryList &list = location.position == QLineEdit::TrailingPosition ? trailingSideWidgets : leadingSideWidgets;
    SideWidgetEntry entry = list[location.index];
    list.erase(list.begin() + location.index);
    if (entry.flags & SideWidgetCreatedByWidgetAction) {
        // If the cast fails, the QAction is in the process of being deleted
        // and has already ceased to be a QWidgetAction; in the process, it
        // will release its widget itself, and calling releaseWidget() here
        // would be UB, so don't:
        if (const auto a = qobject_cast<QWidgetAction*>(entry.action))
            a->releaseWidget(entry.widget);
    } else {
         delete entry.widget;
    }
     positionSideWidgets();
     if (!hasSideWidgets()) // Last widget, remove connection
         QObjectPrivate::connect(q, &QLineEdit::textChanged,
                                 this, &QLineEditPrivate::textChanged);
     q->update();
}
#endif // QT_CONFIG(action)

static int effectiveTextMargin(int defaultMargin, const QLineEditPrivate::SideWidgetEntryList &widgets,
                               const QLineEditPrivate::SideWidgetParameters &parameters)
{
    if (widgets.empty())
        return defaultMargin;

    const auto visibleSideWidgetCount = std::count_if(widgets.begin(), widgets.end(),
                             [](const QLineEditPrivate::SideWidgetEntry &e) {
#if QT_CONFIG(toolbutton) && QT_CONFIG(animation)
        // a button that's fading out doesn't get any space
        if (auto* iconButton = qobject_cast<QLineEditIconButton*>(e.widget))
            return iconButton->needsSpace();

#endif
        return e.widget->isVisibleTo(e.widget->parentWidget());
    });

    return defaultMargin + (parameters.margin + parameters.widgetWidth) * visibleSideWidgetCount;
}

QMargins QLineEditPrivate::effectiveTextMargins() const
{
    return {effectiveTextMargin(textMargins.left(), leftSideWidgetList(), sideWidgetParameters()),
            textMargins.top(),
            effectiveTextMargin(textMargins.right(), rightSideWidgetList(), sideWidgetParameters()),
            textMargins.bottom()};
}


QT_END_NAMESPACE

#include "moc_qlineedit_p.cpp"
