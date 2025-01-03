// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtWidgets/qtwidgetsglobal.h>
#if QT_CONFIG(colordialog)
#include "qcolordialog.h"
#endif
#if QT_CONFIG(fontdialog)
#include "qfontdialog.h"
#endif
#if QT_CONFIG(filedialog)
#include "qfiledialog.h"
#endif

#include "qevent.h"
#include "qapplication.h"
#include "qlayout.h"
#if QT_CONFIG(sizegrip)
#include "qsizegrip.h"
#endif
#if QT_CONFIG(whatsthis)
#include "qwhatsthis.h"
#endif
#if QT_CONFIG(menu)
#include "qmenu.h"
#endif
#include "qcursor.h"
#if QT_CONFIG(messagebox)
#include "qmessagebox.h"
#endif
#if QT_CONFIG(errormessage)
#include "qerrormessage.h"
#endif
#include <qpa/qplatformtheme.h>
#include "private/qdialog_p.h"
#include "private/qguiapplication_p.h"
#if QT_CONFIG(accessibility)
#include "qaccessible.h"
#endif

QT_BEGIN_NAMESPACE

static inline int themeDialogType(const QDialog *dialog)
{
#if QT_CONFIG(filedialog)
    if (qobject_cast<const QFileDialog *>(dialog))
        return QPlatformTheme::FileDialog;
#endif
#if QT_CONFIG(colordialog)
    if (qobject_cast<const QColorDialog *>(dialog))
        return QPlatformTheme::ColorDialog;
#endif
#if QT_CONFIG(fontdialog)
    if (qobject_cast<const QFontDialog *>(dialog))
        return QPlatformTheme::FontDialog;
#endif
#if QT_CONFIG(messagebox)
    if (qobject_cast<const QMessageBox *>(dialog))
        return QPlatformTheme::MessageDialog;
#endif
#if QT_CONFIG(errormessage)
    if (qobject_cast<const QErrorMessage *>(dialog))
        return QPlatformTheme::MessageDialog;
#endif
#if !QT_CONFIG(filedialog) && !QT_CONFIG(colordialog) && !QT_CONFIG(fontdialog) && \
    !QT_CONFIG(messagebox) && !QT_CONFIG(errormessage)
    Q_UNUSED(dialog);
#endif
    return -1;
}

QDialogPrivate::~QDialogPrivate()
{
    delete m_platformHelper;
}

QPlatformDialogHelper *QDialogPrivate::platformHelper() const
{
    // Delayed creation of the platform, ensuring that
    // that qobject_cast<> on the dialog works in the plugin.
    if (!m_platformHelperCreated && canBeNativeDialog()) {
        m_platformHelperCreated = true;
        QDialogPrivate *ncThis = const_cast<QDialogPrivate *>(this);
        QDialog *dialog = ncThis->q_func();
        const int type = themeDialogType(dialog);
        if (type >= 0) {
            m_platformHelper = QGuiApplicationPrivate::platformTheme()
                    ->createPlatformDialogHelper(static_cast<QPlatformTheme::DialogType>(type));
            if (m_platformHelper) {
                QObject::connect(m_platformHelper, SIGNAL(accept()), dialog, SLOT(accept()));
                QObject::connect(m_platformHelper, SIGNAL(reject()), dialog, SLOT(reject()));
                ncThis->initHelper(m_platformHelper);
            }
        }
    }
    return m_platformHelper;
}

bool QDialogPrivate::canBeNativeDialog() const
{
    if (QCoreApplication::testAttribute(Qt::AA_DontUseNativeDialogs))
        return false;

    QDialogPrivate *ncThis = const_cast<QDialogPrivate *>(this);
    QDialog *dialog = ncThis->q_func();
    const int type = themeDialogType(dialog);
    if (type >= 0)
        return QGuiApplicationPrivate::platformTheme()
                ->usePlatformNativeDialog(static_cast<QPlatformTheme::DialogType>(type));
    return false;
}

/*!
    \internal

    Properly closes dialog and sets the \a resultCode.
 */
void QDialogPrivate::close(int resultCode)
{
    Q_Q(QDialog);

    q->setResult(resultCode);

    if (!data.is_closing) {
        // Until Qt 6.3 we didn't close dialogs, so they didn't receive a QCloseEvent.
        // It is likely that subclasses implement closeEvent and handle them as rejection
        // (like QMessageBox and QProgressDialog do), so eat those events.
        struct CloseEventEater : QObject
        {
            using QObject::QObject;
        protected:
            bool eventFilter(QObject *o, QEvent *e) override
            {
                if (e->type() == QEvent::Close)
                    return true;
                return QObject::eventFilter(o, e);
            }
        } closeEventEater;
        q->installEventFilter(&closeEventEater);
        QWidgetPrivate::close();
    } else {
        // If the close was initiated outside of QDialog we will end up
        // here via QDialog::closeEvent calling reject(), in which case
        // we need to hide the dialog to ensure QDialog::closeEvent does
        // not ignore the close event. FIXME: Why is QDialog doing this?
        q->hide();
    }

    resetModalitySetByOpen();
}

QWindow *QDialogPrivate::transientParentWindow() const
{
    Q_Q(const QDialog);
    if (const QWidget *parent = q->nativeParentWidget())
        return parent->windowHandle();
    else if (q->windowHandle())
        return q->windowHandle()->transientParent();
    return nullptr;
}

bool QDialogPrivate::setNativeDialogVisible(bool visible)
{
    if (QPlatformDialogHelper *helper = platformHelper()) {
        if (visible) {
            Q_Q(QDialog);
            helperPrepareShow(helper);
            nativeDialogInUse = helper->show(q->windowFlags(), q->windowModality(), transientParentWindow());
        } else if (nativeDialogInUse) {
            helper->hide();
        }
    }
    return nativeDialogInUse;
}

QVariant QDialogPrivate::styleHint(QPlatformDialogHelper::StyleHint hint) const
{
    if (const QPlatformDialogHelper *helper = platformHelper())
        return helper->styleHint(hint);
    return QPlatformDialogHelper::defaultStyleHint(hint);
}

/*!
    \class QDialog
    \brief The QDialog class is the base class of dialog windows.

    \ingroup dialog-classes
    \ingroup abstractwidgets
    \inmodule QtWidgets

    A dialog window is a top-level window mostly used for short-term
    tasks and brief communications with the user. QDialogs may be
    modal or modeless. QDialogs can
    provide a \l{#return}{return value}, and they can have \l{#default}{default buttons}. QDialogs can also have a QSizeGrip in their
    lower-right corner, using setSizeGripEnabled().

    Note that QDialog (and any other widget that has type \c Qt::Dialog) uses
    the parent widget slightly differently from other classes in Qt. A dialog is
    always a top-level widget, but if it has a parent, its default location is
    centered on top of the parent's top-level widget (if it is not top-level
    itself). It will also share the parent's taskbar entry.

    Use the overload of the QWidget::setParent() function to change
    the ownership of a QDialog widget. This function allows you to
    explicitly set the window flags of the reparented widget; using
    the overloaded function will clear the window flags specifying the
    window-system properties for the widget (in particular it will
    reset the Qt::Dialog flag).

    \note The parent relationship of the dialog does \e{not} imply
    that the dialog will always be stacked on top of the parent
    window. To ensure that the dialog is always on top, make the
    dialog modal. This also applies for child windows of the dialog
    itself. To ensure that child windows of the dialog stay on top
    of the dialog, make the child windows modal as well.

    \section1 Modal Dialogs

    A \b{modal} dialog is a dialog that blocks input to other
    visible windows in the same application. Dialogs that are used to
    request a file name from the user or that are used to set
    application preferences are usually modal. Dialogs can be
    \l{Qt::ApplicationModal}{application modal} (the default) or
    \l{Qt::WindowModal}{window modal}.

    When an application modal dialog is opened, the user must finish
    interacting with the dialog and close it before they can access
    any other window in the application. Window modal dialogs only
    block access to the window associated with the dialog, allowing
    the user to continue to use other windows in an application.

    The most common way to display a modal dialog is to call its
    exec() function. When the user closes the dialog, exec() will
    provide a useful \l{#return}{return value}. To close the dialog
    and return the appropriate value, you must connect a default button,
    e.g. an \uicontrol OK button to the accept() slot and a
    \uicontrol Cancel button to the reject() slot. Alternatively, you
    can call the done() slot with \c Accepted or \c Rejected.

    An alternative is to call setModal(true) or setWindowModality(),
    then show(). Unlike exec(), show() returns control to the caller
    immediately. Calling setModal(true) is especially useful for
    progress dialogs, where the user must have the ability to interact
    with the dialog, e.g.  to cancel a long running operation. If you
    use show() and setModal(true) together to perform a long operation,
    you must call QCoreApplication::processEvents() periodically during
    processing to enable the user to interact with the dialog. (See
    QProgressDialog.)

    \section1 Modeless Dialogs

    A \b{modeless} dialog is a dialog that operates
    independently of other windows in the same application. Find and
    replace dialogs in word-processors are often modeless to allow the
    user to interact with both the application's main window and with
    the dialog.

    Modeless dialogs are displayed using show(), which returns control
    to the caller immediately.

    If you invoke the \l{QWidget::show()}{show()} function after hiding
    a dialog, the dialog will be displayed in its original position. This is
    because the window manager decides the position for windows that
    have not been explicitly placed by the programmer. To preserve the
    position of a dialog that has been moved by the user, save its position
    in your \l{QWidget::closeEvent()}{closeEvent()}  handler and then
    move the dialog to that position, before showing it again.

    \target default
    \section1 Default Button

    A dialog's \e default button is the button that's pressed when the
    user presses Enter (Return). This button is used to signify that
    the user accepts the dialog's settings and wants to close the
    dialog. Use QPushButton::setDefault(), QPushButton::isDefault()
    and QPushButton::autoDefault() to set and control the dialog's
    default button.

    \target escapekey
    \section1 Escape Key

    If the user presses the Esc key in a dialog, QDialog::reject()
    will be called. This will cause the window to close:
    The \l{QCloseEvent}{close event} cannot be \l{QEvent::ignore()}{ignored}.

    \section1 Extensibility

    Extensibility is the ability to show the dialog in two ways: a
    partial dialog that shows the most commonly used options, and a
    full dialog that shows all the options. Typically an extensible
    dialog will initially appear as a partial dialog, but with a
    \uicontrol More toggle button. If the user presses the
    \uicontrol More button down, the dialog is expanded.

    \target return
    \section1 Return Value (Modal Dialogs)

    Modal dialogs are often used in situations where a return value is
    required, e.g. to indicate whether the user pressed \uicontrol OK or
    \uicontrol Cancel. A dialog can be closed by calling the accept() or the
    reject() slots, and exec() will return \c Accepted or \c Rejected
    as appropriate. The exec() call returns the result of the dialog.
    The result is also available from result() if the dialog has not
    been destroyed.

    In order to modify your dialog's close behavior, you can reimplement
    the functions accept(), reject() or done(). The
    \l{QWidget::closeEvent()}{closeEvent()} function should only be
    reimplemented to preserve the dialog's position or to override the
    standard close or reject behavior.

    \target examples
    \section1 Code Examples

    A modal dialog:

    \snippet dialogs/dialogs.cpp 1

    A modeless dialog:

    \snippet dialogs/dialogs.cpp 0

    A dialog with an extension:

    \snippet dialogs/dialogs.cpp extension

    \sa QDialogButtonBox, QTabWidget, QWidget, QProgressDialog,
        {Standard Dialogs Example}
*/

/*! \enum QDialog::DialogCode

    The value returned by a modal dialog.

    \value Accepted
    \value Rejected
*/

/*!
  \property QDialog::sizeGripEnabled
  \brief whether the size grip is enabled

  A QSizeGrip is placed in the bottom-right corner of the dialog when this
  property is enabled. By default, the size grip is disabled.
*/


/*!
  Constructs a dialog with parent \a parent.

  A dialog is always a top-level widget, but if it has a parent, its
  default location is centered on top of the parent. It will also
  share the parent's taskbar entry.

  The widget flags \a f are passed on to the QWidget constructor.
  If, for example, you don't want a What's This button in the title bar
  of the dialog, pass Qt::WindowTitleHint | Qt::WindowSystemMenuHint in \a f.

  \sa QWidget::setWindowFlags()
*/

QDialog::QDialog(QWidget *parent, Qt::WindowFlags f)
    : QWidget(*new QDialogPrivate, parent,
              f | ((f & Qt::WindowType_Mask) == 0 ? Qt::Dialog : Qt::WindowType(0)))
{
}

/*!
  \overload
  \internal
*/
QDialog::QDialog(QDialogPrivate &dd, QWidget *parent, Qt::WindowFlags f)
    : QWidget(dd, parent, f | ((f & Qt::WindowType_Mask) == 0 ? Qt::Dialog : Qt::WindowType(0)))
{
}

/*!
  Destroys the QDialog, deleting all its children.
*/

QDialog::~QDialog()
{
    QT_TRY {
        // Need to hide() here, as our (to-be) overridden hide()
        // will not be called in ~QWidget.
        hide();
    } QT_CATCH(...) {
        // we're in the destructor - just swallow the exception
    }
}

/*!
  \internal
  This function is called by the push button \a pushButton when it
  becomes the default button. If \a pushButton is \nullptr, the dialogs
  default default button becomes the default button. This is what a
  push button calls when it loses focus.
*/
#if QT_CONFIG(pushbutton)
void QDialogPrivate::setDefault(QPushButton *pushButton)
{
    Q_Q(QDialog);
    bool hasMain = false;
    QList<QPushButton*> list = q->findChildren<QPushButton*>();
    for (int i=0; i<list.size(); ++i) {
        QPushButton *pb = list.at(i);
        if (pb->window() == q) {
            if (pb == mainDef)
                hasMain = true;
            if (pb != pushButton)
                pb->setDefault(false);
        }
    }
    if (!pushButton && hasMain)
        mainDef->setDefault(true);
    if (!hasMain)
        mainDef = pushButton;
}

/*!
  \internal
  This function sets the default default push button to \a pushButton.
  This function is called by QPushButton::setDefault().
*/
void QDialogPrivate::setMainDefault(QPushButton *pushButton)
{
    mainDef = nullptr;
    setDefault(pushButton);
}

/*!
  \internal
  Hides the default button indicator. Called when non auto-default
  push button get focus.
 */
void QDialogPrivate::hideDefault()
{
    Q_Q(QDialog);
    QList<QPushButton*> list = q->findChildren<QPushButton*>();
    for (int i=0; i<list.size(); ++i) {
        list.at(i)->setDefault(false);
    }
}
#endif

void QDialogPrivate::resetModalitySetByOpen()
{
    Q_Q(QDialog);
    if (resetModalityTo != -1 && !q->testAttribute(Qt::WA_SetWindowModality)) {
        // open() changed the window modality and the user didn't touch it afterwards; restore it
        q->setWindowModality(Qt::WindowModality(resetModalityTo));
        q->setAttribute(Qt::WA_SetWindowModality, wasModalitySet);
#ifdef Q_OS_MAC
        Q_ASSERT(resetModalityTo != Qt::WindowModal);
        q->setParent(q->parentWidget(), Qt::Dialog);
#endif
    }
    resetModalityTo = -1;
}

/*!
  In general returns the modal dialog's result code, \c Accepted or
  \c Rejected.

  \note When called on a QMessageBox instance, the returned value is a
  value of the \l QMessageBox::StandardButton enum.

  Do not call this function if the dialog was constructed with the
  Qt::WA_DeleteOnClose attribute.
*/
int QDialog::result() const
{
    Q_D(const QDialog);
    return d->rescode;
}

/*!
  \fn void QDialog::setResult(int i)

  Sets the modal dialog's result code to \a i.

  \note We recommend that you use one of the values defined by
  QDialog::DialogCode.
*/
void QDialog::setResult(int r)
{
    Q_D(QDialog);
    d->rescode = r;
}

/*!
    \since 4.5

    Shows the dialog as a \l{QDialog#Modal Dialogs}{window modal dialog},
    returning immediately.

    \sa exec(), show(), result(), setWindowModality()
*/
void QDialog::open()
{
    Q_D(QDialog);

    Qt::WindowModality modality = windowModality();
    if (modality != Qt::WindowModal) {
        d->resetModalityTo = modality;
        d->wasModalitySet = testAttribute(Qt::WA_SetWindowModality);
        setWindowModality(Qt::WindowModal);
        setAttribute(Qt::WA_SetWindowModality, false);
#ifdef Q_OS_MAC
        setParent(parentWidget(), Qt::Sheet);
#endif
    }

    setResult(0);
    show();
}

/*!
    Shows the dialog as a \l{QDialog#Modal Dialogs}{modal dialog},
    blocking until the user closes it. The function returns a \l
    DialogCode result.

    If the dialog is \l{Qt::ApplicationModal}{application modal}, users cannot
    interact with any other window in the same application until they close
    the dialog. If the dialog is \l{Qt::ApplicationModal}{window modal}, only
    interaction with the parent window is blocked while the dialog is open.
    By default, the dialog is application modal.

    \note Avoid using this function; instead, use \c{open()}. Unlike exec(),
    open() is asynchronous, and does not spin an additional event loop. This
    prevents a series of dangerous bugs from happening (e.g. deleting the
    dialog's parent while the dialog is open via exec()). When using open() you
    can connect to the finished() signal of QDialog to be notified when the
    dialog is closed.

    \sa open(), show(), result(), setWindowModality()
*/

int QDialog::exec()
{
    Q_D(QDialog);

    if (Q_UNLIKELY(d->eventLoop)) {
        qWarning("QDialog::exec: Recursive call detected");
        return -1;
    }

    bool deleteOnClose = testAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_DeleteOnClose, false);

    d->resetModalitySetByOpen();

    bool wasShowModal = testAttribute(Qt::WA_ShowModal);
    setAttribute(Qt::WA_ShowModal, true);
    setResult(0);

    show();

    QPointer<QDialog> guard = this;
    if (d->nativeDialogInUse) {
        d->platformHelper()->exec();
    } else {
        QEventLoop eventLoop;
        d->eventLoop = &eventLoop;
        (void) eventLoop.exec(QEventLoop::DialogExec);
    }
    if (guard.isNull())
        return QDialog::Rejected;
    d->eventLoop = nullptr;

    setAttribute(Qt::WA_ShowModal, wasShowModal);

    int res = result();
    if (d->nativeDialogInUse)
        d->helperDone(static_cast<QDialog::DialogCode>(res), d->platformHelper());
    if (deleteOnClose)
        delete this;
    return res;
}

/*!
  Closes the dialog and sets its result code to \a r. The finished() signal
  will emit \a r; if \a r is QDialog::Accepted or QDialog::Rejected, the
  accepted() or the rejected() signals will also be emitted, respectively.

  If this dialog is shown with exec(), done() also causes the local event loop
  to finish, and exec() to return \a r.

  As with QWidget::close(), done() deletes the dialog if the
  Qt::WA_DeleteOnClose flag is set. If the dialog is the application's
  main widget, the application terminates. If the dialog is the
  last window closed, the QGuiApplication::lastWindowClosed() signal is
  emitted.

  \sa accept(), reject(), QApplication::activeWindow(), QCoreApplication::quit()
*/

void QDialog::done(int r)
{
    QPointer<QDialog> guard(this);

    Q_D(QDialog);
    d->close(r);

    if (!guard)
        return;

    int dialogCode = d->dialogCode();
    if (dialogCode == QDialog::Accepted)
        emit accepted();
    else if (dialogCode == QDialog::Rejected)
        emit rejected();

    if (guard)
        emit finished(r);
}

/*!
  Hides the modal dialog and sets the result code to \c Accepted.

  \sa reject(), done()
*/

void QDialog::accept()
{
    done(Accepted);
}

/*!
  Hides the modal dialog and sets the result code to \c Rejected.

  \sa accept(), done()
*/

void QDialog::reject()
{
    done(Rejected);
}

/*! \reimp */
bool QDialog::eventFilter(QObject *o, QEvent *e)
{
    return QWidget::eventFilter(o, e);
}

/*****************************************************************************
  Event handlers
 *****************************************************************************/

#ifndef QT_NO_CONTEXTMENU
/*! \reimp */
void QDialog::contextMenuEvent(QContextMenuEvent *e)
{
#if !QT_CONFIG(whatsthis) || !QT_CONFIG(menu)
    Q_UNUSED(e);
#else
    QWidget *w = childAt(e->pos());
    if (!w) {
        w = rect().contains(e->pos()) ? this : nullptr;
        if (!w)
            return;
    }
    while (w && w->whatsThis().size() == 0 && !w->testAttribute(Qt::WA_CustomWhatsThis))
        w = w->isWindow() ? nullptr : w->parentWidget();
    if (w) {
        QPointer<QMenu> p = new QMenu(this);
        QAction *wt = p.data()->addAction(tr("What's This?"));
        if (p.data()->exec(e->globalPos()) == wt) {
            QHelpEvent e(QEvent::WhatsThis, w->rect().center(),
                         w->mapToGlobal(w->rect().center()));
            QCoreApplication::sendEvent(w, &e);
        }
        delete p.data();
    }
#endif
}
#endif // QT_NO_CONTEXTMENU

/*! \reimp */
void QDialog::keyPressEvent(QKeyEvent *e)
{
#ifndef QT_NO_SHORTCUT
    //   Calls reject() if Escape is pressed. Simulates a button
    //   click for the default button if Enter is pressed. Move focus
    //   for the arrow keys. Ignore the rest.
    if (e->matches(QKeySequence::Cancel)) {
        reject();
    } else
#endif
    if (!e->modifiers() || (e->modifiers() & Qt::KeypadModifier && e->key() == Qt::Key_Enter)) {
        switch (e->key()) {
#if QT_CONFIG(pushbutton)
        case Qt::Key_Enter:
        case Qt::Key_Return: {
            QList<QPushButton*> list = findChildren<QPushButton*>();
            for (int i=0; i<list.size(); ++i) {
                QPushButton *pb = list.at(i);
                if (pb->isDefault() && pb->isVisible()) {
                    if (pb->isEnabled())
                        pb->click();
                    return;
                }
            }
        }
        break;
#endif
        default:
            e->ignore();
            return;
        }
    } else {
        e->ignore();
    }
}

/*! \reimp */
void QDialog::closeEvent(QCloseEvent *e)
{
#if QT_CONFIG(whatsthis)
    if (isModal() && QWhatsThis::inWhatsThisMode())
        QWhatsThis::leaveWhatsThisMode();
#endif
    if (isVisible()) {
        QPointer<QObject> that = this;
        reject();
        if (that && isVisible())
            e->ignore();
    } else {
        e->accept();
    }
}

/*****************************************************************************
  Geometry management.
 *****************************************************************************/

/*! \reimp
*/

void QDialog::setVisible(bool visible)
{
    Q_D(QDialog);
    d->setVisible(visible);
}

void QDialogPrivate::setVisible(bool visible)
{
    Q_Q(QDialog);
    if (!q->testAttribute(Qt::WA_DontShowOnScreen) && canBeNativeDialog() && setNativeDialogVisible(visible))
        return;

    // We should not block windows by the invisible modal dialog
    // if a platform-specific dialog is implemented as an in-process
    // Qt window, because in this case it will also be blocked.
    const bool dontBlockWindows = q->testAttribute(Qt::WA_DontShowOnScreen)
            && styleHint(QPlatformDialogHelper::DialogIsQtWindow).toBool();
    Qt::WindowModality oldModality;
    bool wasModalitySet;

    if (dontBlockWindows) {
        oldModality = q->windowModality();
        wasModalitySet = q->testAttribute(Qt::WA_SetWindowModality);
        q->setWindowModality(Qt::NonModal);
    }

    if (visible) {
        if (q->testAttribute(Qt::WA_WState_ExplicitShowHide) && !q->testAttribute(Qt::WA_WState_Hidden))
            return;

        q->QWidget::setVisible(visible);

        // Window activation might be prevented. We can't test isActiveWindow here,
        // as the window will be activated asynchronously by the window manager.
        if (!q->testAttribute(Qt::WA_ShowWithoutActivating)) {
            QWidget *fw = q->window()->focusWidget();
            if (!fw)
                fw = q;

            /*
            The following block is to handle a special case, and does not
            really follow proper logic in concern of autoDefault and TAB
            order. However, it's here to ease usage for the users. If a
            dialog has a default QPushButton, and first widget in the TAB
            order also is a QPushButton, then we give focus to the main
            default QPushButton. This simplifies code for the developers,
            and actually catches most cases... If not, then they simply
            have to use [widget*]->setFocus() themselves...
            */
#if QT_CONFIG(pushbutton)
            if (mainDef && fw->focusPolicy() == Qt::NoFocus) {
                QWidget *first = fw;
                while ((first = first->nextInFocusChain()) != fw && first->focusPolicy() == Qt::NoFocus)
                    ;
                if (first != mainDef && qobject_cast<QPushButton*>(first))
                    mainDef->setFocus();
            }
            if (!mainDef && q->isWindow()) {
                QWidget *w = fw;
                while ((w = w->nextInFocusChain()) != fw) {
                    QPushButton *pb = qobject_cast<QPushButton *>(w);
                    if (pb && pb->autoDefault() && pb->focusPolicy() != Qt::NoFocus) {
                        pb->setDefault(true);
                        break;
                    }
                }
            }
#endif
            if (fw && !fw->hasFocus()) {
                QFocusEvent e(QEvent::FocusIn, Qt::TabFocusReason);
                QCoreApplication::sendEvent(fw, &e);
            }
        }

#if QT_CONFIG(accessibility)
        QAccessibleEvent event(q, QAccessible::DialogStart);
        QAccessible::updateAccessibility(&event);
#endif

    } else {
        if (q->testAttribute(Qt::WA_WState_ExplicitShowHide) && q->testAttribute(Qt::WA_WState_Hidden))
            return;

#if QT_CONFIG(accessibility)
        if (q->isVisible()) {
            QAccessibleEvent event(q, QAccessible::DialogEnd);
            QAccessible::updateAccessibility(&event);
        }
#endif

        // Reimplemented to exit a modal event loop when the dialog is hidden.
        q->QWidget::setVisible(visible);
        if (eventLoop)
            eventLoop->exit();
    }

    if (dontBlockWindows) {
        q->setWindowModality(oldModality);
        q->setAttribute(Qt::WA_SetWindowModality, wasModalitySet);
    }

#if QT_CONFIG(pushbutton)
    const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme();
    if (mainDef && q->isActiveWindow()
        && theme->themeHint(QPlatformTheme::DialogSnapToDefaultButton).toBool())
        QCursor::setPos(mainDef->mapToGlobal(mainDef->rect().center()));
#endif
}

/*!\reimp */
void QDialog::showEvent(QShowEvent *event)
{
    if (!event->spontaneous() && !testAttribute(Qt::WA_Moved)) {
        Qt::WindowStates  state = windowState();
        adjustPosition(parentWidget());
        setAttribute(Qt::WA_Moved, false); // not really an explicit position
        if (state != windowState())
            setWindowState(state);
    }
}

/*! \internal */
void QDialog::adjustPosition(QWidget* w)
{
    Q_D(QDialog);

    if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme())
        if (theme->themeHint(QPlatformTheme::WindowAutoPlacement).toBool())
            return;
    QPoint p(0, 0);
    int extraw = 0, extrah = 0;
    const QWindow *parentWindow = nullptr;
    if (w) {
        w = w->window();
    } else {
        parentWindow = d->transientParentWindow();
    }
    QRect desk;
    QScreen *scrn = nullptr;
    if (w)
        scrn = w->screen();
    else if (parentWindow)
        scrn = parentWindow->screen();
    else if (QGuiApplication::primaryScreen()->virtualSiblings().size() > 1)
        scrn = QGuiApplication::screenAt(QCursor::pos());
    else
        scrn = screen();
    if (scrn)
        desk = scrn->availableGeometry();

    QWidgetList list = QApplication::topLevelWidgets();
    for (int i = 0; (extraw == 0 || extrah == 0) && i < list.size(); ++i) {
        QWidget * current = list.at(i);
        if (current->isVisible()) {
            int framew = current->geometry().x() - current->x();
            int frameh = current->geometry().y() - current->y();

            extraw = qMax(extraw, framew);
            extrah = qMax(extrah, frameh);
        }
    }

    // sanity check for decoration frames. With embedding, we
    // might get extraordinary values
    if (extraw == 0 || extrah == 0 || extraw >= 10 || extrah >= 40) {
        extrah = 40;
        extraw = 10;
    }


    if (w) {
        // Use pos() if the widget is embedded into a native window
        QPoint pp;
        if (w->windowHandle() && qvariant_cast<WId>(w->windowHandle()->property("_q_embedded_native_parent_handle")))
            pp = w->pos();
        else
            pp = w->mapToGlobal(QPoint(0,0));
        p = QPoint(pp.x() + w->width()/2,
                    pp.y() + w->height()/ 2);
    } else if (parentWindow) {
        // QTBUG-63406: Widget-based dialog in QML, which has no Widget parent
        // but a transient parent window.
        QPoint pp = parentWindow->mapToGlobal(QPoint(0, 0));
        p = QPoint(pp.x() + parentWindow->width() / 2, pp.y() + parentWindow->height() / 2);
    } else {
        // p = middle of the desktop
        p = QPoint(desk.x() + desk.width()/2, desk.y() + desk.height()/2);
    }

    // p = origin of this
    p = QPoint(p.x()-width()/2 - extraw,
                p.y()-height()/2 - extrah);


    if (p.x() + extraw + width() > desk.x() + desk.width())
        p.setX(desk.x() + desk.width() - width() - extraw);
    if (p.x() < desk.x())
        p.setX(desk.x());

    if (p.y() + extrah + height() > desk.y() + desk.height())
        p.setY(desk.y() + desk.height() - height() - extrah);
    if (p.y() < desk.y())
        p.setY(desk.y());

    // QTBUG-52735: Manually set the correct target screen since scaling in a
    // subsequent call to QWindow::resize() may otherwise use the wrong factor
    // if the screen changed notification is still in an event queue.
    if (scrn) {
        if (QWindow *window = windowHandle())
            window->setScreen(scrn);
    }

    move(p);
}

/*! \reimp */
QSize QDialog::sizeHint() const
{
    Q_D(const QDialog);
    if (d->extension) {
        if (d->orientation == Qt::Horizontal)
            return QSize(QWidget::sizeHint().width(),
                        qMax(QWidget::sizeHint().height(),d->extension->sizeHint().height()));
        else
            return QSize(qMax(QWidget::sizeHint().width(), d->extension->sizeHint().width()),
                        QWidget::sizeHint().height());
    }
    return QWidget::sizeHint();
}


/*! \reimp */
QSize QDialog::minimumSizeHint() const
{
    Q_D(const QDialog);
    if (d->extension) {
        if (d->orientation == Qt::Horizontal)
            return QSize(QWidget::minimumSizeHint().width(),
                        qMax(QWidget::minimumSizeHint().height(), d->extension->minimumSizeHint().height()));
        else
            return QSize(qMax(QWidget::minimumSizeHint().width(), d->extension->minimumSizeHint().width()),
                        QWidget::minimumSizeHint().height());
    }

    return QWidget::minimumSizeHint();
}

/*!
    \property QDialog::modal
    \brief whether show() should pop up the dialog as modal or modeless

    By default, this property is \c false and show() pops up the dialog
    as modeless. Setting this property to true is equivalent to setting
    QWidget::windowModality to Qt::ApplicationModal.

    exec() ignores the value of this property and always pops up the
    dialog as modal.

    \sa QWidget::windowModality, show(), exec()
*/

void QDialog::setModal(bool modal)
{
    setAttribute(Qt::WA_ShowModal, modal);
}


bool QDialog::isSizeGripEnabled() const
{
#if QT_CONFIG(sizegrip)
    Q_D(const QDialog);
    return !!d->resizer;
#else
    return false;
#endif
}


void QDialog::setSizeGripEnabled(bool enabled)
{
#if !QT_CONFIG(sizegrip)
    Q_UNUSED(enabled);
#else
    Q_D(QDialog);
#if QT_CONFIG(sizegrip)
    d->sizeGripEnabled = enabled;
    if (enabled && d->doShowExtension)
        return;
#endif
    if (!enabled != !d->resizer) {
        if (enabled) {
            d->resizer = new QSizeGrip(this);
            // adjustSize() processes all events, which is suboptimal
            d->resizer->resize(d->resizer->sizeHint());
            if (isRightToLeft())
                d->resizer->move(rect().bottomLeft() -d->resizer->rect().bottomLeft());
            else
                d->resizer->move(rect().bottomRight() -d->resizer->rect().bottomRight());
            d->resizer->raise();
            d->resizer->show();
        } else {
            delete d->resizer;
            d->resizer = nullptr;
        }
    }
#endif // QT_CONFIG(sizegrip)
}



/*! \reimp */
void QDialog::resizeEvent(QResizeEvent *)
{
#if QT_CONFIG(sizegrip)
    Q_D(QDialog);
    if (d->resizer) {
        if (isRightToLeft())
            d->resizer->move(rect().bottomLeft() -d->resizer->rect().bottomLeft());
        else
            d->resizer->move(rect().bottomRight() -d->resizer->rect().bottomRight());
        d->resizer->raise();
    }
#endif
}

/*! \fn void QDialog::finished(int result)
    \since 4.1

    This signal is emitted when the dialog's \a result code has been
    set, either by the user or by calling done(), accept(), or
    reject().

    Note that this signal is \e not emitted when hiding the dialog
    with hide() or setVisible(false). This includes deleting the
    dialog while it is visible.

    \sa accepted(), rejected()
*/

/*! \fn void QDialog::accepted()
    \since 4.1

    This signal is emitted when the dialog has been accepted either by
    the user or by calling accept() or done() with the
    QDialog::Accepted argument.

    Note that this signal is \e not emitted when hiding the dialog
    with hide() or setVisible(false). This includes deleting the
    dialog while it is visible.

    \sa finished(), rejected()
*/

/*! \fn void QDialog::rejected()
    \since 4.1

    This signal is emitted when the dialog has been rejected either by
    the user or by calling reject() or done() with the
    QDialog::Rejected argument.

    Note that this signal is \e not emitted when hiding the dialog
    with hide() or setVisible(false). This includes deleting the
    dialog while it is visible.

    \sa finished(), accepted()
*/

QT_END_NAMESPACE
#include "moc_qdialog.cpp"
