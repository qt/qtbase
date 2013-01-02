/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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
#include "qmaliitplatforminputcontext.h"

#include <QtDebug>
#include <QTextCharFormat>
#include <QGuiApplication>
#include <qwindow.h>
#include <qevent.h>
#include <qscreen.h>

#include "serveraddressproxy.h"
#include "serverproxy.h"
#include "contextadaptor.h"

#include <sys/types.h>
#include <signal.h>

#include <QtDBus>

QT_BEGIN_NAMESPACE

enum { debug = 0 };

enum InputPanelVisibility {
    InputPanelHidden,
    InputPanelShowRequested,
    InputPanelShown
};

enum MaliitOrientationAngle {
    Angle0   =   0,
    Angle90  =  90,
    Angle180 = 180,
    Angle270 = 270
};

static int orientationAngle(Qt::ScreenOrientation orientation)
{
    switch (orientation) {
    case Qt::PrimaryOrientation: // Urgh.
    case Qt::PortraitOrientation:
        return Angle270;
    case Qt::LandscapeOrientation:
        return Angle0;
    case Qt::InvertedPortraitOrientation:
        return Angle90;
    case Qt::InvertedLandscapeOrientation:
        return Angle180;
    }
    return Angle0;
}

// From MTF:
//! Content type for text entries. Used at least with MTextEdit
enum TextContentType {
    //! all characters allowed
    FreeTextContentType,

    //! only integer numbers allowed
    NumberContentType,

    //! allows numbers and certain other characters used in phone numbers
    PhoneNumberContentType,

    //! allows only characters permitted in email address
    EmailContentType,

    //! allows only character permitted in URL address
    UrlContentType,

    //! allows content with user defined format
    CustomContentType
};
static TextContentType contentTypeFromHints(Qt::InputMethodHints hints)
{
    TextContentType type = FreeTextContentType;
    hints &= Qt::ImhExclusiveInputMask;

    if (hints == Qt::ImhFormattedNumbersOnly || hints == Qt::ImhDigitsOnly)
        type = NumberContentType;
    else if (hints == Qt::ImhDialableCharactersOnly)
        type = PhoneNumberContentType;
    else if (hints == Qt::ImhEmailCharactersOnly)
        type = EmailContentType;
    else if (hints == Qt::ImhUrlCharactersOnly)
        type = UrlContentType;

    return type;
}

/// From Maliit's namespace.h
enum MaliitEventRequestType {
    EventRequestBoth,         //!< Both a Qt::KeyEvent and a signal
    EventRequestSignalOnly,   //!< Only a signal
    EventRequestEventOnly     //!< Only a Qt::KeyEvent
};

static QString maliitServerAddress()
{
    org::maliit::Server::Address serverAddress(QStringLiteral("org.maliit.server"), QStringLiteral("/org/maliit/server/address"), QDBusConnection::sessionBus());

    QString address(serverAddress.address());

    // Fallback to old socket when org.maliit.server service is not available
    if (address.isEmpty())
        return QStringLiteral("unix:path=/tmp/meego-im-uiserver/imserver_dbus");

    return address;
}

class QMaliitPlatformInputContextPrivate
{
public:
    QMaliitPlatformInputContextPrivate(QMaliitPlatformInputContext *qq);
    ~QMaliitPlatformInputContextPrivate()
    {
        delete adaptor;
        delete server;
    }

    void sendStateUpdate(bool focusChanged = false);

    QDBusConnection connection;
    ComMeegoInputmethodUiserver1Interface *server;
    Inputcontext1Adaptor *adaptor;

    QMap<QString, QVariant> imState;

    InputPanelVisibility visibility;

    bool valid;
    bool active;
    bool correctionEnabled;
    QRect keyboardRect;
    QString preedit;
    QPointer<QWindow> window;
    QMaliitPlatformInputContext *q;
};


QMaliitPlatformInputContext::QMaliitPlatformInputContext()
    : d(new QMaliitPlatformInputContextPrivate(this))
{
    if (debug)
        qDebug() << "QMaliitPlatformInputContext::QMaliitPlatformInputContext()";
}

QMaliitPlatformInputContext::~QMaliitPlatformInputContext(void)
{
    delete d;
}

bool QMaliitPlatformInputContext::isValid() const
{
    return d->valid;
}

void QMaliitPlatformInputContext::invokeAction(QInputMethod::Action action, int x)
{
    if (!inputMethodAccepted())
        return;

    if (action == QInputMethod::Click) {
        if (x < 0 || x >= d->preedit.length()) {
            reset();
            return;
        }

        d->imState["preeditClickPos"] = x;
        d->sendStateUpdate();
        // The first argument is the mouse pos and the second is the
        // preedit rectangle. Both are unused on the server side.
        d->server->mouseClickedOnPreedit(0, 0, 0, 0, 0, 0);
    } else {
        QPlatformInputContext::invokeAction(action, x);
    }
}

void QMaliitPlatformInputContext::reset()
{
    const bool hadPreedit = !d->preedit.isEmpty();
    if (hadPreedit && inputMethodAccepted()) {
        // ### selection
        QInputMethodEvent event;
        event.setCommitString(d->preedit);
        QGuiApplication::sendEvent(qGuiApp->focusObject(), &event);
        d->preedit.clear();
    }

    QDBusPendingReply<void> reply = d->server->reset();
    if (hadPreedit)
        reply.waitForFinished();
}

void QMaliitPlatformInputContext::update(Qt::InputMethodQueries queries)
{
    if (!qGuiApp->focusObject())
        return;

    QInputMethodQueryEvent query(queries);
    QGuiApplication::sendEvent(qGuiApp->focusObject(), &query);

    if (queries & Qt::ImSurroundingText)
        d->imState["surroundingText"] = query.value(Qt::ImSurroundingText);
    if (queries & Qt::ImCursorPosition)
        d->imState["cursorPosition"] = query.value(Qt::ImCursorPosition);
    if (queries & Qt::ImAnchorPosition)
        d->imState["anchorPosition"] = query.value(Qt::ImAnchorPosition);
    if (queries & Qt::ImCursorRectangle) {
        QRect rect = query.value(Qt::ImCursorRectangle).toRect();
        rect = qGuiApp->inputMethod()->inputItemTransform().mapRect(rect);
        QWindow *window = qGuiApp->focusWindow();
        if (window)
            d->imState["cursorRectangle"] = QRect(window->mapToGlobal(rect.topLeft()), rect.size());
    }

    if (queries & Qt::ImCurrentSelection)
        d->imState["hasSelection"] = !query.value(Qt::ImCurrentSelection).toString().isEmpty();

    if (queries & Qt::ImHints) {
        Qt::InputMethodHints hints = Qt::InputMethodHints(query.value(Qt::ImHints).toUInt());

        d->imState["predictionEnabled"] = !(hints & Qt::ImhNoPredictiveText);
        d->imState["autocapitalizationEnabled"] = !(hints & Qt::ImhNoAutoUppercase);
        d->imState["hiddenText"] = (hints & Qt::ImhHiddenText) != 0;

        d->imState["contentType"] = contentTypeFromHints(hints);
    }

    d->sendStateUpdate(/*focusChanged*/true);
}

QRectF QMaliitPlatformInputContext::keyboardRect() const
{
    return d->keyboardRect;
}

void QMaliitPlatformInputContext::activationLostEvent()
{
    d->active = false;
    d->visibility = InputPanelHidden;
}

void QMaliitPlatformInputContext::commitString(const QString &string, int replacementStart, int replacementLength, int /* cursorPos */)
{
    if (!inputMethodAccepted())
        return;

    d->preedit.clear();

    if (debug)
        qWarning() << "CommitString" << string;
    // ### start/cursorPos
    QInputMethodEvent event;
    event.setCommitString(string, replacementStart, replacementLength);
    QCoreApplication::sendEvent(qGuiApp->focusObject(), &event);
}

void QMaliitPlatformInputContext::updatePreedit(const QDBusMessage &message)
{
    if (!inputMethodAccepted())
        return;

    QList<QVariant> arguments = message.arguments();
    if (arguments.count() != 5) {
        qWarning() << "QMaliitPlatformInputContext::updatePreedit: Received message from input method server with wrong parameters.";
        return;
    }

    d->preedit = arguments[0].toString();

    QList<QInputMethodEvent::Attribute> attributes;

    const QDBusArgument formats = arguments[1].value<QDBusArgument>();
    formats.beginArray();
    while (!formats.atEnd()) {
        formats.beginStructure();
        int start, length, preeditFace;
        formats >> start >> length >> preeditFace;
        formats.endStructure();

        QTextCharFormat format;

        enum PreeditFace {
            PreeditDefault,
            PreeditNoCandidates,
            PreeditKeyPress,      //!< Used for displaying the hwkbd key just pressed
            PreeditUnconvertible, //!< Inactive preedit region, not clickable
            PreeditActive,        //!< Preedit region with active suggestions

        };
        switch (PreeditFace(preeditFace)) {
        case PreeditDefault:
        case PreeditKeyPress:
            format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
            format.setUnderlineColor(QColor(0, 0, 0));
            break;
        case PreeditNoCandidates:
            format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
            format.setUnderlineColor(QColor(255, 0, 0));
            break;
        case PreeditUnconvertible:
            format.setForeground(QBrush(QColor(128, 128, 128)));
            break;
        case PreeditActive:
            format.setForeground(QBrush(QColor(153, 50, 204)));
            format.setFontWeight(QFont::Bold);
            break;
        default:
            break;
        }

        attributes << QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat, start, length, format);
    }
    formats.endArray();

    int replacementStart = arguments[2].toInt();
    int replacementLength = arguments[3].toInt();
    int cursorPos = arguments[4].toInt();

    if (debug)
        qWarning() << "updatePreedit" << d->preedit << replacementStart << replacementLength << cursorPos;

    if (cursorPos >= 0)
        attributes << QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, cursorPos, 1, QVariant());

    QInputMethodEvent event(d->preedit, attributes);
    if (replacementStart || replacementLength)
        event.setCommitString(QString(), replacementStart, replacementLength);
    QCoreApplication::sendEvent(qGuiApp->focusObject(), &event);
}

void QMaliitPlatformInputContext::copy()
{
    // Not supported at the moment.
}

void QMaliitPlatformInputContext::imInitiatedHide()
{
    d->visibility = InputPanelHidden;
    emitInputPanelVisibleChanged();
    // ### clear focus
}

void QMaliitPlatformInputContext::keyEvent(int type, int key, int modifiers, const QString &text,
                                          bool autoRepeat, int count, uchar requestType_)
{
    MaliitEventRequestType requestType = MaliitEventRequestType(requestType_);
    if (requestType == EventRequestSignalOnly) {
        qWarning() << "Maliit: Signal emitted key events are not supported.";
        return;
    }

    // HACK: This code relies on QEvent::Type for key events and modifiers to be binary compatible between
    // Qt 4 and 5.
    QEvent::Type eventType = static_cast<QEvent::Type>(type);
    if (type != QEvent::KeyPress && type != QEvent::KeyRelease) {
        qWarning() << "Maliit: Unknown key event type" << type;
        return;
    }

    QKeyEvent event(eventType, key, static_cast<Qt::KeyboardModifiers>(modifiers),
                    text, autoRepeat, count);
    if (d->window)
        QCoreApplication::sendEvent(d->window.data(), &event);
}

void QMaliitPlatformInputContext::paste()
{
    // Not supported at the moment.
}

bool QMaliitPlatformInputContext::preeditRectangle(int &x, int &y, int &width, int &height)
{
    // ###
    QRect r = qApp->inputMethod()->cursorRectangle().toRect();
    if (!r.isValid())
        return false;
    x = r.x();
    y = r.y();
    width = r.width();
    height = r.height();
    return true;
}

bool QMaliitPlatformInputContext::selection(QString &selection)
{
    selection.clear();

    if (!inputMethodAccepted())
        return false;

    QInputMethodQueryEvent query(Qt::ImCurrentSelection);
    QGuiApplication::sendEvent(qGuiApp->focusObject(), &query);
    QVariant value = query.value(Qt::ImCurrentSelection);
    if (!value.isValid())
        return false;

    selection = value.toString();
    return true;
}

void QMaliitPlatformInputContext::setDetectableAutoRepeat(bool)
{
    // Not supported.
}

void QMaliitPlatformInputContext::setGlobalCorrectionEnabled(bool enable)
{
    d->correctionEnabled = enable;
}

void QMaliitPlatformInputContext::setLanguage(const QString &)
{
    // Unused at the moment.
}

void QMaliitPlatformInputContext::setRedirectKeys(bool)
{
    // Not supported.
}

void QMaliitPlatformInputContext::setSelection(int start, int length)
{
    if (!inputMethodAccepted())
        return;

    QList<QInputMethodEvent::Attribute> attributes;
    attributes << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, start, length, QVariant());
    QInputMethodEvent event(QString(), attributes);
    QGuiApplication::sendEvent(qGuiApp->focusObject(), &event);
}

void QMaliitPlatformInputContext::updateInputMethodArea(int x, int y, int width, int height)
{
    d->keyboardRect = QRect(x, y, width, height);
    emitKeyboardRectChanged();
}

void QMaliitPlatformInputContext::updateServerWindowOrientation(Qt::ScreenOrientation orientation)
{
    d->server->appOrientationChanged(orientationAngle(orientation));
}

void QMaliitPlatformInputContext::setFocusObject(QObject *object)
{
    if (!d->valid)
        return;

    QWindow *window = qGuiApp->focusWindow();
    if (window != d->window.data()) {
       if (d->window)
           disconnect(d->window.data(), SIGNAL(contentOrientationChanged(Qt::ScreenOrientation)),
                      this, SLOT(updateServerWindowOrientation(Qt::ScreenOrientation)));
        d->window = window;
        if (d->window)
            connect(d->window.data(), SIGNAL(contentOrientationChanged(Qt::ScreenOrientation)),
                    this, SLOT(updateServerWindowOrientation(Qt::ScreenOrientation)));
    }

    d->imState["focusState"] = (object != 0);
    if (inputMethodAccepted()) {
        if (window)
            d->imState["winId"] = static_cast<qulonglong>(window->winId());

        if (!d->active) {
            d->active = true;
            d->server->activateContext();

            if (window)
                d->server->appOrientationChanged(orientationAngle(window->contentOrientation()));
        }
    }
    d->sendStateUpdate(/*focusChanged*/true);
    if (inputMethodAccepted() && window && d->visibility == InputPanelShowRequested)
        showInputPanel();
}

void QMaliitPlatformInputContext::showInputPanel()
{
    if (debug)
        qDebug() << "showInputPanel";

    if (!inputMethodAccepted())
        d->visibility = InputPanelShowRequested;
    else {
        d->server->showInputMethod();
        d->visibility = InputPanelShown;
        emitInputPanelVisibleChanged();
    }
}

void QMaliitPlatformInputContext::hideInputPanel()
{
    d->server->hideInputMethod();
    d->visibility = InputPanelHidden;
    emitInputPanelVisibleChanged();
}

bool QMaliitPlatformInputContext::isInputPanelVisible() const
{
    return d->visibility == InputPanelShown;
}

QMaliitPlatformInputContextPrivate::QMaliitPlatformInputContextPrivate(QMaliitPlatformInputContext* qq)
    : connection(QDBusConnection::connectToPeer(maliitServerAddress(), QLatin1String("MaliitIMProxy")))
    , server(0)
    , adaptor(0)
    , visibility(InputPanelHidden)
    , valid(false)
    , active(false)
    , correctionEnabled(false)
    , q(qq)
{
    if (!connection.isConnected())
        return;

    server = new ComMeegoInputmethodUiserver1Interface(QStringLiteral(""), QStringLiteral("/com/meego/inputmethod/uiserver1"), connection);
    adaptor = new Inputcontext1Adaptor(qq);
    connection.registerObject("/com/meego/inputmethod/inputcontext", qq);

    enum InputMethodMode {
        //! Normal mode allows to use preedit and error correction
        InputMethodModeNormal,

        //! Virtual keyboard sends QKeyEvent for every key press or release
        InputMethodModeDirect,

        //! Used with proxy widget
        InputMethodModeProxy
    };
    imState["inputMethodMode"] = InputMethodModeNormal;

    imState["correctionEnabled"] = true;

    valid = true;
}

void QMaliitPlatformInputContextPrivate::sendStateUpdate(bool focusChanged)
{
    server->updateWidgetInformation(imState, focusChanged);
}

QT_END_NAMESPACE
