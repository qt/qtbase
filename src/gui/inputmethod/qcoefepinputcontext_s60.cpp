/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QT_NO_IM

#include "qcoefepinputcontext_p.h"
#include <qapplication.h>
#include <qtextformat.h>
#include <qgraphicsview.h>
#include <qgraphicsscene.h>
#include <qgraphicswidget.h>
#include <qsymbianevent.h>
#include <qlayout.h>
#include <qdesktopwidget.h>
#include <private/qcore_symbian_p.h>

#include <fepitfr.h>
#include <hal.h>

#include <limits.h>
// You only find these enumerations on SDK 5 onwards, so we need to provide our own
// to remain compatible with older releases. They won't be called by pre-5.0 SDKs.

// MAknEdStateObserver::EAknCursorPositionChanged
#define QT_EAknCursorPositionChanged MAknEdStateObserver::EAknEdwinStateEvent(6)
// MAknEdStateObserver::EAknActivatePenInputRequest
#define QT_EAknActivatePenInputRequest MAknEdStateObserver::EAknEdwinStateEvent(7)

// EAknEditorFlagSelectionVisible is only valid from 3.2 onwards.
// Sym^3 AVKON FEP manager expects that this flag is used for FEP-aware editors
// that support text selection.
#define QT_EAknEditorFlagSelectionVisible 0x100000

// EAknEditorFlagEnablePartialScreen is only valid from Sym^3 onwards.
#define QT_EAknEditorFlagEnablePartialScreen 0x200000

QT_BEGIN_NAMESPACE

Q_GUI_EXPORT void qt_s60_setPartialScreenInputMode(bool enable)
{
    S60->partial_keyboard = enable;

    QInputContext *ic = 0;
    if (QApplication::focusWidget()) {
        ic = QApplication::focusWidget()->inputContext();
    } else if (qApp && qApp->inputContext()) {
        ic = qApp->inputContext();
    }
    if (ic)
        ic->update();
}

QCoeFepInputContext::QCoeFepInputContext(QObject *parent)
    : QInputContext(parent),
      m_fepState(q_check_ptr(new CAknEdwinState)),		// CBase derived object needs check on new
      m_lastImHints(Qt::ImhNone),
      m_textCapabilities(TCoeInputCapabilities::EAllText),
      m_inDestruction(false),
      m_pendingInputCapabilitiesChanged(false),
      m_cursorVisibility(1),
      m_inlinePosition(0),
      m_formatRetriever(0),
      m_pointerHandler(0),
      m_hasTempPreeditString(false),
      m_splitViewResizeBy(0),
      m_splitViewPreviousWindowStates(Qt::WindowNoState)
{
    m_fepState->SetObjectProvider(this);
    int defaultFlags = EAknEditorFlagDefault;
    if (QSysInfo::s60Version() > QSysInfo::SV_S60_5_0) {
        if (S60->partial_keyboard) {
            defaultFlags |= QT_EAknEditorFlagEnablePartialScreen;
        }
        defaultFlags |= QT_EAknEditorFlagSelectionVisible;
    }
    m_fepState->SetFlags(defaultFlags);
    m_fepState->SetDefaultInputMode( EAknEditorTextInputMode );
    m_fepState->SetPermittedInputModes( EAknEditorAllInputModes );
    m_fepState->SetDefaultCase( EAknEditorTextCase );
    m_fepState->SetPermittedCases( EAknEditorAllCaseModes );
    m_fepState->SetSpecialCharacterTableResourceId(R_AVKON_SPECIAL_CHARACTER_TABLE_DIALOG);
    m_fepState->SetNumericKeymap(EAknEditorAlphanumericNumberModeKeymap);
}

QCoeFepInputContext::~QCoeFepInputContext()
{
    m_inDestruction = true;

    // This is to make sure that the FEP manager "forgets" about us,
    // otherwise we may get callbacks even after we're destroyed.
    // The call below is essentially equivalent to InputCapabilitiesChanged(),
    // but is synchronous, rather than asynchronous.
    CCoeEnv::Static()->SyncNotifyFocusObserversOfChangeInFocus();

    if (m_fepState)
        delete m_fepState;
}

void QCoeFepInputContext::reset()
{
    commitCurrentString(true);
}

void QCoeFepInputContext::ReportAknEdStateEvent(MAknEdStateObserver::EAknEdwinStateEvent aEventType)
{
    QT_TRAP_THROWING(m_fepState->ReportAknEdStateEventL(aEventType));
}

void QCoeFepInputContext::update()
{
    updateHints(false);

    // For pre-5.0 SDKs, we don't do text updates on S60 side.
    if (QSysInfo::s60Version() < QSysInfo::SV_S60_5_0) {
        return;
    }

    // Don't be fooled (as I was) by the name of this enumeration.
    // What it really does is tell the virtual keyboard UI that the text has been
    // updated and it should be reflected in the internal display of the VK.
    ReportAknEdStateEvent(QT_EAknCursorPositionChanged);
}

void QCoeFepInputContext::setFocusWidget(QWidget *w)
{
    commitCurrentString(true);

    QInputContext::setFocusWidget(w);

    updateHints(true);
}

void QCoeFepInputContext::widgetDestroyed(QWidget *w)
{
    // Make sure that the input capabilities of whatever new widget got focused are queried.
    CCoeControl *ctrl = w->effectiveWinId();
    if (ctrl->IsFocused()) {
        queueInputCapabilitiesChanged();
    }
}

QString QCoeFepInputContext::language()
{
    TLanguage lang = m_fepState->LocalLanguage();
    const QByteArray localeName = qt_symbianLocaleName(lang);
    if (!localeName.isEmpty()) {
        return QString::fromLatin1(localeName);
    } else {
        return QString::fromLatin1("C");
    }
}

bool QCoeFepInputContext::needsInputPanel()
{
    switch (QSysInfo::s60Version()) {
    case QSysInfo::SV_S60_3_1:
    case QSysInfo::SV_S60_3_2:
        // There are no touch phones for pre-5.0 SDKs.
        return false;
#ifdef Q_CC_NOKIAX86
    default:
        // For emulator we assume that we need an input panel, since we can't
        // separate between phone types.
        return true;
#else
    case QSysInfo::SV_S60_5_0: {
        // For SDK == 5.0, we need phone specific detection, since the HAL API
        // is no good on most phones. However, all phones at the time of writing use the
        // input panel, except N97 in landscape mode, but in this mode it refuses to bring
        // up the panel anyway, so we don't have to care.
        return true;
    }
    default:
        // For unknown/newer types, we try to use the HAL API.
        int keyboardEnabled;
        int keyboardType;
        int err[2];
        err[0] = HAL::Get(HAL::EKeyboard, keyboardType);
        err[1] = HAL::Get(HAL::EKeyboardState, keyboardEnabled);
        if (err[0] == KErrNone && err[1] == KErrNone
                && keyboardType != 0 && keyboardEnabled)
            // Means that we have some sort of keyboard.
            return false;

        // Fall back to using the input panel.
        return true;
#endif // !Q_CC_NOKIAX86
    }
}

bool QCoeFepInputContext::filterEvent(const QEvent *event)
{
    // The CloseSoftwareInputPanel event is not handled here, because the VK will automatically
    // close when it discovers that the underlying widget does not have input capabilities.

    if (!focusWidget())
        return false;

    switch (event->type()) {
    case QEvent::MouseButtonPress:
        // Alphanumeric keypad doesn't like it when we click and text is still getting displayed
        // It ignores the mouse event, so we need to commit and send a selection event (which will get triggered
        // after the commit)
        if (!m_preeditString.isEmpty()) {
            commitCurrentString(true);

            int pos = focusWidget()->inputMethodQuery(Qt::ImCursorPosition).toInt();

            QList<QInputMethodEvent::Attribute> selectAttributes;
            selectAttributes << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, pos, 0, QVariant());
            QInputMethodEvent selectEvent(QLatin1String(""), selectAttributes);
            sendEvent(selectEvent);
    }
        break;
    case QEvent::KeyPress:
        commitTemporaryPreeditString();
        // fall through intended
    case QEvent::KeyRelease:
        const QKeyEvent *keyEvent = static_cast<const QKeyEvent *>(event);
        //If proxy exists, always use hints from proxy.
        QWidget *proxy = focusWidget()->focusProxy();
        Qt::InputMethodHints currentHints = proxy ? proxy->inputMethodHints() : focusWidget()->inputMethodHints();

        switch (keyEvent->key()) {
        case Qt::Key_F20:
            Q_ASSERT(m_lastImHints == currentHints);
            if (m_lastImHints & Qt::ImhHiddenText) {
                // Special case in Symbian. On editors with secret text, F20 is for some reason
                // considered to be a backspace.
                QKeyEvent modifiedEvent(keyEvent->type(), Qt::Key_Backspace, keyEvent->modifiers(),
                        keyEvent->text(), keyEvent->isAutoRepeat(), keyEvent->count());
                QApplication::sendEvent(focusWidget(), &modifiedEvent);
                return true;
            }
            break;
        case Qt::Key_Select:
            if (!m_preeditString.isEmpty()) {
                commitCurrentString(true);
                return true;
            }
            break;
        default:
            break;
        }

        QString widgetText = focusWidget()->inputMethodQuery(Qt::ImSurroundingText).toString();
        bool validLength;
        int maxLength = focusWidget()->inputMethodQuery(Qt::ImMaximumTextLength).toInt(&validLength);
        if (!keyEvent->text().isEmpty() && validLength
                && widgetText.size() + m_preeditString.size() >= maxLength) {
            // Don't send key events with string content if the widget is "full".
            return true;
        }

        if (keyEvent->type() == QEvent::KeyPress
            && currentHints & Qt::ImhHiddenText
            && !keyEvent->text().isEmpty()) {
            // Send some temporary preedit text in order to make text visible for a moment.
            m_preeditString = keyEvent->text();
            QList<QInputMethodEvent::Attribute> attributes;
            QInputMethodEvent imEvent(m_preeditString, attributes);
            sendEvent(imEvent);
            m_tempPreeditStringTimeout.start(1000, this);
            m_hasTempPreeditString = true;
            update();
            return true;
        }
        break;
    }

    if (!needsInputPanel())
        return false;

    if (event->type() == QEvent::RequestSoftwareInputPanel) {
        // Notify S60 that we want the virtual keyboard to show up.
        QSymbianControl *sControl;
        sControl = focusWidget()->effectiveWinId()->MopGetObject(sControl);
        Q_ASSERT(sControl);

        // The FEP UI temporarily steals focus when it shows up the first time, causing
        // all sorts of weird effects on the focused widgets. Since it will immediately give
        // back focus to us, we temporarily disable focus handling until the job's done.
        if (sControl) {
            sControl->setIgnoreFocusChanged(true);
        }

        ensureInputCapabilitiesChanged();
        m_fepState->ReportAknEdStateEventL(MAknEdStateObserver::QT_EAknActivatePenInputRequest);

        if (sControl) {
            sControl->setIgnoreFocusChanged(false);
        }
        return true;
    }

    return false;
}

bool QCoeFepInputContext::symbianFilterEvent(QWidget *keyWidget, const QSymbianEvent *event)
{
    Q_UNUSED(keyWidget);
    if (event->type() == QSymbianEvent::CommandEvent)
        // A command basically means the same as a button being pushed. With Qt buttons
        // that would normally result in a reset of the input method due to the focus change.
        // This should also happen for commands.
        reset();

    if (event->type() == QSymbianEvent::WindowServerEvent
        && event->windowServerEvent()
        && event->windowServerEvent()->Type() == EEventWindowVisibilityChanged
        && S60->splitViewLastWidget) {

        QGraphicsView *gv = qobject_cast<QGraphicsView*>(S60->splitViewLastWidget);
        const bool alwaysResize = (gv && gv->verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOff);

        if (alwaysResize) {
            TUint visibleFlags = event->windowServerEvent()->VisibilityChanged()->iFlags;
            if (visibleFlags & TWsVisibilityChangedEvent::EPartiallyVisible)
                ensureFocusWidgetVisible(S60->splitViewLastWidget);
            if (visibleFlags & TWsVisibilityChangedEvent::ENotVisible)
                resetSplitViewWidget(true);
        }
    }

    return false;
}

void QCoeFepInputContext::timerEvent(QTimerEvent *timerEvent)
{
    if (timerEvent->timerId() == m_tempPreeditStringTimeout.timerId())
        commitTemporaryPreeditString();
}

void QCoeFepInputContext::commitTemporaryPreeditString()
{
    if (m_tempPreeditStringTimeout.isActive())
        m_tempPreeditStringTimeout.stop();

    if (!m_hasTempPreeditString)
        return;

    commitCurrentString(false);
}

void QCoeFepInputContext::mouseHandler( int x, QMouseEvent *event)
{
    Q_ASSERT(focusWidget());

    if (event->type() == QEvent::MouseButtonPress && event->button() == Qt::LeftButton) {
        commitCurrentString(true);
        int pos = focusWidget()->inputMethodQuery(Qt::ImCursorPosition).toInt();

        QList<QInputMethodEvent::Attribute> attributes;
        attributes << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, pos + x, 0, QVariant());
        QInputMethodEvent event(QLatin1String(""), attributes);
        sendEvent(event);
    }
}

TCoeInputCapabilities QCoeFepInputContext::inputCapabilities()
{
    if (m_inDestruction || !focusWidget()) {
        return TCoeInputCapabilities(TCoeInputCapabilities::ENone, 0, 0);
    }

    return TCoeInputCapabilities(m_textCapabilities, this, 0);
}

void QCoeFepInputContext::resetSplitViewWidget(bool keepInputWidget)
{
    QGraphicsView *gv = qobject_cast<QGraphicsView*>(S60->splitViewLastWidget);

    if (!gv) {
        return;
    }

    QSymbianControl *symControl = static_cast<QSymbianControl*>(gv->effectiveWinId());
    symControl->CancelLongTapTimer();

    const bool alwaysResize = (gv->verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOff);
    QWidget *windowToMove = gv->window();

    bool userResize = gv->testAttribute(Qt::WA_Resized);

    windowToMove->setUpdatesEnabled(false);

    if (!alwaysResize) {
        if (gv->scene()) {
            if (gv->scene()->focusItem()) {
                // Check if the widget contains cursorPositionChanged signal and disconnect from it.
                QByteArray signal = QMetaObject::normalizedSignature(SIGNAL(cursorPositionChanged()));
                int index = gv->scene()->focusItem()->toGraphicsObject()->metaObject()->indexOfSignal(signal.right(signal.length() - 1));
                if (index != -1)
                    disconnect(gv->scene()->focusItem()->toGraphicsObject(), SIGNAL(cursorPositionChanged()), this, SLOT(translateInputWidget()));
            }

            QGraphicsItem *rootItem = 0;
            foreach (QGraphicsItem *item, gv->scene()->items()) {
                if (!item->parentItem()) {
                    rootItem = item;
                    break;
                }
            }
            if (rootItem)
                rootItem->resetTransform();
        }
    } else {
        if (m_splitViewResizeBy)
            gv->resize(gv->rect().width(), m_splitViewResizeBy);
    }
    // Resizing might have led to widget losing its original windowstate.
    // Restore previous window state.

    if (m_splitViewPreviousWindowStates != windowToMove->windowState())
        windowToMove->setWindowState(m_splitViewPreviousWindowStates);

    windowToMove->setUpdatesEnabled(true);

    gv->setAttribute(Qt::WA_Resized, userResize); //not a user resize

    m_splitViewResizeBy = 0;
    if (!keepInputWidget) {
        m_splitViewPreviousWindowStates = Qt::WindowNoState;
        S60->splitViewLastWidget = 0;
    }
}

// Checks if a given widget is visible in the splitview rect. The offset
// parameter can be used to validate if moving widget upwards or downwards
// by the offset would make a difference for the visibility.

bool QCoeFepInputContext::isWidgetVisible(QWidget *widget, int offset)
{
    bool visible = false;
    if (widget) {
        QRect splitViewRect = qt_TRect2QRect(static_cast<CEikAppUi*>(S60->appUi())->ClientRect());
        QWidget *window = QApplication::activeWindow();
        QGraphicsView *gv = qobject_cast<QGraphicsView*>(widget);
        if (gv && window) {
            if (QGraphicsScene *scene = gv->scene()) {
                if (QGraphicsItem *focusItem = scene->focusItem()) {
                    QPoint cursorPos = window->mapToGlobal(focusItem->cursor().pos());
                    cursorPos.setY(cursorPos.y() + offset);
                    if (splitViewRect.contains(cursorPos)) {
                        visible = true;
                    }
                }
            }
        }
    }
    return visible;
}

// Ensure that the input widget is visible in the splitview rect.

void QCoeFepInputContext::ensureFocusWidgetVisible(QWidget *widget)
{
    // Native side opening and closing its virtual keyboard when it changes the keyboard layout,
    // has an adverse impact on long tap timer. Cancel the timer when splitview opens to avoid this.
    QSymbianControl *symControl = static_cast<QSymbianControl*>(widget->effectiveWinId());
    symControl->CancelLongTapTimer();

    // Graphicsviews that have vertical scrollbars should always be resized to the splitview area.
    // Graphicsviews without scrollbars should be translated.

    QGraphicsView *gv = qobject_cast<QGraphicsView*>(widget);
    if (!gv)
        return;

    const bool alwaysResize = (gv && gv->verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOff);
    const bool moveWithinVisibleArea = (S60->splitViewLastWidget != 0);

    QWidget *windowToMove = gv ? gv : symControl->widget();
    if (!windowToMove->isWindow())
        windowToMove = windowToMove->window();
    if (!windowToMove) {
        return;
    }

    // When opening the keyboard (not moving within the splitview area), save the original
    // window state. In some cases, ensuring input widget visibility might lead to window
    // states getting changed.

    if (!moveWithinVisibleArea) {
        // Check if the widget contains cursorPositionChanged signal and connect to it.
        QByteArray signal = QMetaObject::normalizedSignature(SIGNAL(cursorPositionChanged()));
        if (gv->scene() && gv->scene()->focusItem()) {
            int index = gv->scene()->focusItem()->toGraphicsObject()->metaObject()->indexOfSignal(signal.right(signal.length() - 1));
            if (index != -1)
                connect(gv->scene()->focusItem()->toGraphicsObject(), SIGNAL(cursorPositionChanged()), this, SLOT(translateInputWidget()));
        }
        S60->splitViewLastWidget = widget;
        m_splitViewPreviousWindowStates = windowToMove->windowState();
    }

    int windowTop = widget->window()->pos().y();

    const bool userResize = widget->testAttribute(Qt::WA_Resized);

    QRect splitViewRect = qt_TRect2QRect(static_cast<CEikAppUi*>(S60->appUi())->ClientRect());


    // When resizing a window widget, it will lose its maximized window state.
    // Native applications hide statuspane in splitview state, so lets move to
    // fullscreen mode. This makes available area slightly bigger, which helps usability
    // and greatly reduces event passing in orientation switch cases,
    // as the statuspane size is not changing.

    if (alwaysResize)
        windowToMove->setUpdatesEnabled(false);

    if (!(windowToMove->windowState() & Qt::WindowFullScreen)) {
        windowToMove->setWindowState(
            (windowToMove->windowState() & ~(Qt::WindowMinimized | Qt::WindowFullScreen)) | Qt::WindowFullScreen);
    }

    if (alwaysResize) {
        if (!moveWithinVisibleArea) {
            m_splitViewResizeBy = widget->height();
            windowTop = widget->geometry().top();
            widget->resize(widget->width(), splitViewRect.height() - windowTop);
        }

        if (gv->scene()) {
            const QRectF microFocusRect = gv->scene()->inputMethodQuery(Qt::ImMicroFocus).toRectF();
            gv->ensureVisible(microFocusRect);
        }
    } else {
        translateInputWidget();
    }

    if (alwaysResize)
        windowToMove->setUpdatesEnabled(true);

    widget->setAttribute(Qt::WA_Resized, userResize); //not a user resize
}

static QTextCharFormat qt_TCharFormat2QTextCharFormat(const TCharFormat &cFormat, bool validStyleColor)
{
    QTextCharFormat qFormat;

    if (validStyleColor) {
        QBrush foreground(QColor(cFormat.iFontPresentation.iTextColor.Internal()));
        qFormat.setForeground(foreground);
    }

    qFormat.setFontStrikeOut(cFormat.iFontPresentation.iStrikethrough == EStrikethroughOn);
    qFormat.setFontUnderline(cFormat.iFontPresentation.iUnderline == EUnderlineOn);

    return qFormat;
}

void QCoeFepInputContext::updateHints(bool mustUpdateInputCapabilities)
{
    QWidget *w = focusWidget();
    if (w) {
        QWidget *proxy = w->focusProxy();
        Qt::InputMethodHints hints = proxy ? proxy->inputMethodHints() : w->inputMethodHints();

        // Since splitview support works like an input method hint, yet it is private flag,
        // we need to update its state separately.
        if (QSysInfo::s60Version() > QSysInfo::SV_S60_5_0) {
            TInt currentFlags = m_fepState->Flags();
            if (S60->partial_keyboard)
                currentFlags |= QT_EAknEditorFlagEnablePartialScreen;
            else
                currentFlags &= ~QT_EAknEditorFlagEnablePartialScreen;
            if (currentFlags != m_fepState->Flags())
                m_fepState->SetFlags(currentFlags);
        }

        if (hints != m_lastImHints) {
            m_lastImHints = hints;
            applyHints(hints);
        } else if (!mustUpdateInputCapabilities) {
            // Optimization. Return immediately if there was no change.
            return;
        }
    }
    queueInputCapabilitiesChanged();
}

void QCoeFepInputContext::applyHints(Qt::InputMethodHints hints)
{
    using namespace Qt;

    commitTemporaryPreeditString();

    const bool anynumbermodes = hints & (ImhDigitsOnly | ImhFormattedNumbersOnly | ImhDialableCharactersOnly);
    const bool anytextmodes = hints & (ImhUppercaseOnly | ImhLowercaseOnly | ImhEmailCharactersOnly | ImhUrlCharactersOnly);
    const bool numbersOnly = anynumbermodes && !anytextmodes;
    const bool noOnlys = !(hints & ImhExclusiveInputMask);
    // if alphanumeric input, or if multiple incompatible number modes are selected;
    // then make all symbols available in numeric mode too.
    const bool needsCharMap= !numbersOnly || ((hints & ImhFormattedNumbersOnly) && (hints & ImhDialableCharactersOnly));
    TInt flags;
    Qt::InputMethodHints oldHints = hints;

    // Some sanity checking. Make sure that only one preference is set.
    InputMethodHints prefs = ImhPreferNumbers | ImhPreferUppercase | ImhPreferLowercase;
    prefs &= hints;
    if (prefs != ImhPreferNumbers && prefs != ImhPreferUppercase && prefs != ImhPreferLowercase) {
        hints &= ~prefs;
    }
    if (!noOnlys) {
        // Make sure that the preference is within the permitted set.
        if (hints & ImhPreferNumbers && !anynumbermodes) {
            hints &= ~ImhPreferNumbers;
        } else if (hints & ImhPreferUppercase && !(hints & ImhUppercaseOnly)) {
            hints &= ~ImhPreferUppercase;
        } else if (hints & ImhPreferLowercase && !(hints & ImhLowercaseOnly)) {
            hints &= ~ImhPreferLowercase;
        }
        // If there is no preference, set it to something within the permitted set.
        if (!(hints & ImhPreferNumbers || hints & ImhPreferUppercase || hints & ImhPreferLowercase)) {
            if (hints & ImhLowercaseOnly) {
                hints |= ImhPreferLowercase;
            } else if (hints & ImhUppercaseOnly) {
                hints |= ImhPreferUppercase;
            } else if (numbersOnly) {
                hints |= ImhPreferNumbers;
            }
        }
    }

    if (hints & ImhPreferNumbers) {
        m_fepState->SetDefaultInputMode(EAknEditorNumericInputMode);
        m_fepState->SetCurrentInputMode(EAknEditorNumericInputMode);
    } else {
        m_fepState->SetDefaultInputMode(EAknEditorTextInputMode);
        m_fepState->SetCurrentInputMode(EAknEditorTextInputMode);
    }
    flags = 0;
    if (noOnlys || (anynumbermodes && anytextmodes)) {
        flags = EAknEditorAllInputModes;
    }
    else if (anynumbermodes) {
        flags |= EAknEditorNumericInputMode;
        if (QSysInfo::s60Version() > QSysInfo::SV_S60_5_0
            && ((hints & ImhFormattedNumbersOnly) || (hints & ImhDialableCharactersOnly))) {
            //workaround - the * key does not launch the symbols menu, making it impossible to use these modes unless text mode is enabled.
            flags |= EAknEditorTextInputMode;
        }
    }
    else if (anytextmodes) {
        flags |= EAknEditorTextInputMode;
    }
    else {
        flags = EAknEditorAllInputModes;
    }
    m_fepState->SetPermittedInputModes(flags);
    ReportAknEdStateEvent(MAknEdStateObserver::EAknEdwinStateInputModeUpdate);

    if (hints & ImhPreferLowercase) {
        m_fepState->SetDefaultCase(EAknEditorLowerCase);
        m_fepState->SetCurrentCase(EAknEditorLowerCase);
    } else if (hints & ImhPreferUppercase) {
        m_fepState->SetDefaultCase(EAknEditorUpperCase);
        m_fepState->SetCurrentCase(EAknEditorUpperCase);
    } else if (hints & ImhNoAutoUppercase) {
        m_fepState->SetDefaultCase(EAknEditorLowerCase);
        m_fepState->SetCurrentCase(EAknEditorLowerCase);
    } else {
        m_fepState->SetDefaultCase(EAknEditorTextCase);
        m_fepState->SetCurrentCase(EAknEditorTextCase);
    }
    flags = 0;
    if (hints & ImhUppercaseOnly) {
        flags |= EAknEditorUpperCase;
    }
    if (hints & ImhLowercaseOnly) {
        flags |= EAknEditorLowerCase;
    }
    if (flags == 0) {
        flags = EAknEditorAllCaseModes;
        if (hints & ImhNoAutoUppercase) {
            flags &= ~EAknEditorTextCase;
        }
    }
    m_fepState->SetPermittedCases(flags);
    ReportAknEdStateEvent(MAknEdStateObserver::EAknEdwinStateCaseModeUpdate);

    flags = 0;
    if (QSysInfo::s60Version() > QSysInfo::SV_S60_5_0) {
        if (S60->partial_keyboard)
            flags |= QT_EAknEditorFlagEnablePartialScreen;
        flags |= QT_EAknEditorFlagSelectionVisible;
    }
    if (hints & ImhUppercaseOnly && !(hints & ImhLowercaseOnly)
            || hints & ImhLowercaseOnly && !(hints & ImhUppercaseOnly)) {
        flags |= EAknEditorFlagFixedCase;
    }
    // Using T9 and hidden text together may actually crash the FEP, so check for hidden text too.
    if (hints & ImhNoPredictiveText || hints & ImhHiddenText) {
        flags |= EAknEditorFlagNoT9;
    }
    if (needsCharMap)
        flags |= EAknEditorFlagUseSCTNumericCharmap;
    m_fepState->SetFlags(flags);
    ReportAknEdStateEvent(MAknEdStateObserver::EAknEdwinStateFlagsUpdate);

    if (hints & ImhDialableCharactersOnly) {
        // This is first, because if (ImhDialableCharactersOnly | ImhFormattedNumbersOnly)
        // is specified, this one is more natural (# key enters a #)
        flags = EAknEditorStandardNumberModeKeymap;
    } else if (hints & ImhFormattedNumbersOnly) {
        // # key enters decimal point
        flags = EAknEditorCalculatorNumberModeKeymap;
    } else if (hints & ImhDigitsOnly) {
        // This is last, because it is most restrictive (# key is inactive)
        flags = EAknEditorPlainNumberModeKeymap;
    } else {
        flags = EAknEditorStandardNumberModeKeymap;
    }
    m_fepState->SetNumericKeymap(static_cast<TAknEditorNumericKeymap>(flags));

    if (hints & ImhUrlCharactersOnly) {
        // URL characters is everything except space, so a superset of the other restrictions
        m_fepState->SetSpecialCharacterTableResourceId(R_AVKON_URL_SPECIAL_CHARACTER_TABLE_DIALOG);
    } else if (hints & ImhEmailCharactersOnly) {
        m_fepState->SetSpecialCharacterTableResourceId(R_AVKON_EMAIL_ADDR_SPECIAL_CHARACTER_TABLE_DIALOG);
    } else if (needsCharMap) {
        m_fepState->SetSpecialCharacterTableResourceId(R_AVKON_SPECIAL_CHARACTER_TABLE_DIALOG);
    } else if ((hints & ImhFormattedNumbersOnly) || (hints & ImhDialableCharactersOnly)) {
        m_fepState->SetSpecialCharacterTableResourceId(R_AVKON_SPECIAL_CHARACTER_TABLE_DIALOG);
    } else {
        m_fepState->SetSpecialCharacterTableResourceId(0);
    }

    if (hints & ImhHiddenText) {
        m_textCapabilities = TCoeInputCapabilities::EAllText | TCoeInputCapabilities::ESecretText;
    } else {
        m_textCapabilities = TCoeInputCapabilities::EAllText;
    }
}

void QCoeFepInputContext::applyFormat(QList<QInputMethodEvent::Attribute> *attributes)
{
    TCharFormat cFormat;
    QColor styleTextColor;
    if (QWidget *focused = focusWidget()) {
        QGraphicsView *gv = qobject_cast<QGraphicsView*>(focused);
        if (!gv) // could be either the QGV or its viewport that has focus
            gv = qobject_cast<QGraphicsView*>(focused->parentWidget());
        if (gv) {
            if (QGraphicsScene *scene = gv->scene()) {
                if (QGraphicsItem *focusItem = scene->focusItem()) {
                    if (focusItem->isWidget()) {
                        styleTextColor = static_cast<QGraphicsWidget*>(focusItem)->palette().text().color();
                    }
                }
            }
        } else {
            styleTextColor = focused->palette().text().color();
        }
    } else {
        styleTextColor = QApplication::palette("QLineEdit").text().color();
    }

    if (styleTextColor.isValid()) {
        const TLogicalRgb fontColor(TRgb(styleTextColor.red(), styleTextColor.green(), styleTextColor.blue(), styleTextColor.alpha()));
        cFormat.iFontPresentation.iTextColor = fontColor;
    }

    TInt numChars = 0;
    TInt charPos = 0;
    int oldSize = attributes->size();
    while (m_formatRetriever) {
        m_formatRetriever->GetFormatOfFepInlineText(cFormat, numChars, charPos);
        if (numChars <= 0) {
            // This shouldn't happen according to S60 docs, but apparently does sometimes.
            break;
        }
        attributes->append(QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat,
                                                        charPos,
                                                        numChars,
                                                        QVariant(qt_TCharFormat2QTextCharFormat(cFormat, styleTextColor.isValid()))));
        charPos += numChars;
        if (charPos >= m_preeditString.size()) {
            break;
        }
    }

    if (attributes->size() == oldSize) {
        // S60 didn't provide any format, so let's give our own instead.
        attributes->append(QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat,
                                                        0,
                                                        m_preeditString.size(),
                                                        standardFormat(PreeditFormat)));
    }
}

void QCoeFepInputContext::queueInputCapabilitiesChanged()
{
    if (m_pendingInputCapabilitiesChanged)
        return;

    // Call ensureInputCapabilitiesChanged asynchronously. This is done to improve performance
    // by not updating input capabilities too often. The reason we don't call the Symbian
    // asynchronous version of InputCapabilitiesChanged is because we need to ensure that it
    // is synchronous in some specific cases. Those will call ensureInputCapabilitesChanged.
    QMetaObject::invokeMethod(this, "ensureInputCapabilitiesChanged", Qt::QueuedConnection);
    m_pendingInputCapabilitiesChanged = true;
}

void QCoeFepInputContext::ensureInputCapabilitiesChanged()
{
    if (!m_pendingInputCapabilitiesChanged)
        return;

    // The call below is essentially equivalent to InputCapabilitiesChanged(),
    // but is synchronous, rather than asynchronous.
    CCoeEnv::Static()->SyncNotifyFocusObserversOfChangeInFocus();
    m_pendingInputCapabilitiesChanged = false;
}

void QCoeFepInputContext::translateInputWidget()
{
    QGraphicsView *gv = qobject_cast<QGraphicsView *>(S60->splitViewLastWidget);
    QRect splitViewRect = qt_TRect2QRect(static_cast<CEikAppUi*>(S60->appUi())->ClientRect());

    QRectF cursor = gv->scene()->inputMethodQuery(Qt::ImMicroFocus).toRectF();
    QPolygon cursorP = gv->mapFromScene(cursor);
    QRectF vkbRect = QRectF(splitViewRect.bottomLeft(), qApp->desktop()->rect().bottomRight());
    if (cursor.isEmpty() || vkbRect.isEmpty())
        return;

    // Fetch root item (i.e. graphicsitem with no parent)
    QGraphicsItem *rootItem = 0;
    foreach (QGraphicsItem *item, gv->scene()->items()) {
        if (!item->parentItem()) {
            rootItem = item;
            break;
        }
    }
    if (!rootItem)
        return;

    m_transformation = (rootItem->transform().isTranslating()) ? QRectF(0,0, gv->width(), rootItem->transform().dy()) : QRectF();

    // Do nothing if the cursor is visible in the splitview area.
    if (splitViewRect.contains(cursorP.boundingRect()))
        return;

    // New Y position should be ideally at the center of the splitview area.
    // If that would expose unpainted canvas, limit the tranformation to the visible scene bottom.

    const qreal maxY = gv->sceneRect().bottom() - splitViewRect.bottom() + m_transformation.height();
    qreal dy = -(qMin(maxY, (cursor.bottom() - vkbRect.top() / 2)));

    // Do not allow transform above screen top.
    if (m_transformation.height() + dy > 0)
        return;

    rootItem->setTransform(QTransform::fromTranslate(0, dy), true);
}

void QCoeFepInputContext::StartFepInlineEditL(const TDesC& aInitialInlineText,
        TInt aPositionOfInsertionPointInInlineText, TBool aCursorVisibility, const MFormCustomDraw* /*aCustomDraw*/,
        MFepInlineTextFormatRetriever& aInlineTextFormatRetriever,
        MFepPointerEventHandlerDuringInlineEdit& aPointerEventHandlerDuringInlineEdit)
{
    QWidget *w = focusWidget();
    if (!w)
        return;

    commitTemporaryPreeditString();

    QList<QInputMethodEvent::Attribute> attributes;

    m_cursorVisibility = aCursorVisibility ? 1 : 0;
    m_inlinePosition = aPositionOfInsertionPointInInlineText;
    m_preeditString = qt_TDesC2QString(aInitialInlineText);

    m_formatRetriever = &aInlineTextFormatRetriever;
    m_pointerHandler = &aPointerEventHandlerDuringInlineEdit;

    // With T9 aInitialInlineText is typically empty when StartFepInlineEditL is called,
    // but FEP requires that selected text is always removed at StartFepInlineEditL.
    // Let's remove the selected text if aInitialInlineText is empty and there is selected text
    if (m_preeditString.isEmpty()) {
        int anchor = w->inputMethodQuery(Qt::ImAnchorPosition).toInt();
        int cursorPos = w->inputMethodQuery(Qt::ImCursorPosition).toInt();
        int replacementLength = qAbs(cursorPos-anchor);
        if (replacementLength > 0) {
            int replacementStart = cursorPos < anchor ? 0 : -replacementLength;
            QList<QInputMethodEvent::Attribute> clearSelectionAttributes;
            QInputMethodEvent clearSelectionEvent(QLatin1String(""), clearSelectionAttributes);
            clearSelectionEvent.setCommitString(QLatin1String(""), replacementStart, replacementLength);
            sendEvent(clearSelectionEvent);
        }
    }

    applyFormat(&attributes);

    attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Cursor,
                                                   m_inlinePosition,
                                                   m_cursorVisibility,
                                                   QVariant()));
    QInputMethodEvent event(m_preeditString, attributes);
    sendEvent(event);
}

void QCoeFepInputContext::UpdateFepInlineTextL(const TDesC& aNewInlineText,
        TInt aPositionOfInsertionPointInInlineText)
{
    QWidget *w = focusWidget();
    if (!w)
        return;

    commitTemporaryPreeditString();

    m_inlinePosition = aPositionOfInsertionPointInInlineText;

    QList<QInputMethodEvent::Attribute> attributes;
    applyFormat(&attributes);
    attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Cursor,
                                                   m_inlinePosition,
                                                   m_cursorVisibility,
                                                   QVariant()));
    QString newPreeditString = qt_TDesC2QString(aNewInlineText);
    QInputMethodEvent event(newPreeditString, attributes);
    if (newPreeditString.isEmpty() && m_preeditString.isEmpty()) {
        // In Symbian world this means "erase last character".
        event.setCommitString(QLatin1String(""), -1, 1);
    }
    m_preeditString = newPreeditString;
    sendEvent(event);
}

void QCoeFepInputContext::SetInlineEditingCursorVisibilityL(TBool aCursorVisibility)
{
    QWidget *w = focusWidget();
    if (!w)
        return;

    m_cursorVisibility = aCursorVisibility ? 1 : 0;

    QList<QInputMethodEvent::Attribute> attributes;
    attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Cursor,
                                                   m_inlinePosition,
                                                   m_cursorVisibility,
                                                   QVariant()));
    QInputMethodEvent event(m_preeditString, attributes);
    sendEvent(event);
}

void QCoeFepInputContext::CancelFepInlineEdit()
{
    // We are not supposed to ever have a tempPreeditString and a real preedit string
    // from S60 at the same time, so it should be safe to rely on this test to determine
    // whether we should honor S60's request to clear the text or not.
    if (m_hasTempPreeditString)
        return;

    QList<QInputMethodEvent::Attribute> attributes;
    QInputMethodEvent event(QLatin1String(""), attributes);
    event.setCommitString(QLatin1String(""), 0, 0);
    m_preeditString.clear();
    m_inlinePosition = 0;
    sendEvent(event);
}

TInt QCoeFepInputContext::DocumentLengthForFep() const
{
    QWidget *w = focusWidget();
    if (!w)
        return 0;

    QVariant variant = w->inputMethodQuery(Qt::ImSurroundingText);
    return variant.value<QString>().size() + m_preeditString.size();
}

TInt QCoeFepInputContext::DocumentMaximumLengthForFep() const
{
    QWidget *w = focusWidget();
    if (!w)
        return 0;

    QVariant variant = w->inputMethodQuery(Qt::ImMaximumTextLength);
    int size;
    if (variant.isValid()) {
        size = variant.toInt();
    } else {
        size = INT_MAX; // Sensible default for S60.
    }
    return size;
}

void QCoeFepInputContext::SetCursorSelectionForFepL(const TCursorSelection& aCursorSelection)
{
    QWidget *w = focusWidget();
    if (!w)
        return;

    commitTemporaryPreeditString();

    int pos = aCursorSelection.iAnchorPos;
    int length = aCursorSelection.iCursorPos - pos;

    QList<QInputMethodEvent::Attribute> attributes;
    attributes << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, pos, length, QVariant());
    QInputMethodEvent event(m_preeditString, attributes);
    sendEvent(event);
}

void QCoeFepInputContext::GetCursorSelectionForFep(TCursorSelection& aCursorSelection) const
{
    QWidget *w = focusWidget();
    if (!w) {
        aCursorSelection.SetSelection(0,0);
        return;
    }

    int cursor = w->inputMethodQuery(Qt::ImCursorPosition).toInt() + m_preeditString.size();
    int anchor = w->inputMethodQuery(Qt::ImAnchorPosition).toInt() + m_preeditString.size();
    QString text = w->inputMethodQuery(Qt::ImSurroundingText).value<QString>();
    int combinedSize = text.size() + m_preeditString.size();
    if (combinedSize < anchor || combinedSize < cursor) {
        // ### TODO! FIXME! QTBUG-5050
        // This is a hack to prevent crashing in 4.6 with QLineEdits that use input masks.
        // The root problem is that cursor position is relative to displayed text instead of the
        // actual text we get.
        //
        // To properly fix this we would need to know the displayText of QLineEdits instead
        // of just the text, which on itself should be a trivial change. The difficulties start
        // when we need to commit the changes back to the QLineEdit, which would have to be somehow
        // able to handle displayText, too.
        //
        // Until properly fixed, the cursor and anchor positions will not reflect correct positions
        // for masked QLineEdits, unless all the masked positions are filled in order so that
        // cursor position relative to the displayed text matches position relative to actual text.
        aCursorSelection.iAnchorPos = combinedSize;
        aCursorSelection.iCursorPos = combinedSize;
    } else {
        aCursorSelection.iAnchorPos = anchor;
        aCursorSelection.iCursorPos = cursor;
    }
}

void QCoeFepInputContext::GetEditorContentForFep(TDes& aEditorContent, TInt aDocumentPosition,
        TInt aLengthToRetrieve) const
{
    QWidget *w = focusWidget();
    if (!w) {
        aEditorContent.FillZ(aLengthToRetrieve);
        return;
    }

    QString text = w->inputMethodQuery(Qt::ImSurroundingText).value<QString>();
    // FEP expects the preedit string to be part of the editor content, so let's mix it in.
    int cursor = w->inputMethodQuery(Qt::ImCursorPosition).toInt();
    text.insert(cursor, m_preeditString);
    aEditorContent.Copy(qt_QString2TPtrC(text.mid(aDocumentPosition, aLengthToRetrieve)));
}

void QCoeFepInputContext::GetFormatForFep(TCharFormat& aFormat, TInt /* aDocumentPosition */) const
{
    QWidget *w = focusWidget();
    if (!w) {
        aFormat = TCharFormat();
        return;
    }

    QFont font = w->inputMethodQuery(Qt::ImFont).value<QFont>();
    QFontMetrics metrics(font);
    //QString name = font.rawName();
    QString name = font.defaultFamily(); // TODO! FIXME! Should be the above.
    QHBufC hBufC(name);
    aFormat = TCharFormat(hBufC->Des(), metrics.height());
}

void QCoeFepInputContext::GetScreenCoordinatesForFepL(TPoint& aLeftSideOfBaseLine, TInt& aHeight,
        TInt& aAscent, TInt /* aDocumentPosition */) const
{
    QWidget *w = focusWidget();
    if (!w) {
        aLeftSideOfBaseLine = TPoint(0,0);
        aHeight = 0;
        aAscent = 0;
        return;
    }

    QRect rect = w->inputMethodQuery(Qt::ImMicroFocus).value<QRect>();
    aLeftSideOfBaseLine.iX = rect.left();
    aLeftSideOfBaseLine.iY = rect.bottom();

    QFont font = w->inputMethodQuery(Qt::ImFont).value<QFont>();
    QFontMetrics metrics(font);
    aHeight = metrics.height();
    aAscent = metrics.ascent();
}

void QCoeFepInputContext::DoCommitFepInlineEditL()
{
    commitCurrentString(false);
    if (QSysInfo::s60Version() > QSysInfo::SV_S60_5_0)
        ReportAknEdStateEvent(QT_EAknCursorPositionChanged);

}

void QCoeFepInputContext::commitCurrentString(bool cancelFepTransaction)
{
    QList<QInputMethodEvent::Attribute> attributes;
    QInputMethodEvent event(QLatin1String(""), attributes);
    event.setCommitString(m_preeditString, 0, 0);
    m_preeditString.clear();
    m_inlinePosition = 0;
    sendEvent(event);

    m_hasTempPreeditString = false;

    if (cancelFepTransaction) {
        CCoeFep* fep = CCoeEnv::Static()->Fep();
        if (fep)
            fep->CancelTransaction();
    }
}

MCoeFepAwareTextEditor_Extension1* QCoeFepInputContext::Extension1(TBool& aSetToTrue)
{
    aSetToTrue = ETrue;
    return this;
}

void QCoeFepInputContext::SetStateTransferingOwnershipL(MCoeFepAwareTextEditor_Extension1::CState* aState,
        TUid /*aTypeSafetyUid*/)
{
    // Note: The S60 docs are wrong! See the State() function.
    if (m_fepState)
        delete m_fepState;
    m_fepState = static_cast<CAknEdwinState *>(aState);
}

MCoeFepAwareTextEditor_Extension1::CState* QCoeFepInputContext::State(TUid /*aTypeSafetyUid*/)
{
    // Note: The S60 docs are horribly wrong when describing the
    // SetStateTransferingOwnershipL function and this function. They say that the former
    // sets a CState object identified by the TUid, and the latter retrieves it.
    // In reality, the CState is expected to always be a CAknEdwinState (even if it was not
    // previously set), and the TUid is ignored. All in all, there is a single CAknEdwinState
    // per QCoeFepInputContext, which should be deleted if the SetStateTransferingOwnershipL
    // function is used to set a new one.
    return m_fepState;
}

TTypeUid::Ptr QCoeFepInputContext::MopSupplyObject(TTypeUid /*id*/)
{
    return TTypeUid::Null();
}

MObjectProvider *QCoeFepInputContext::MopNext()
{
    QWidget *w = focusWidget();
    if (w)
        return w->effectiveWinId();
    return 0;
}

QT_END_NAMESPACE

#endif // QT_NO_IM
