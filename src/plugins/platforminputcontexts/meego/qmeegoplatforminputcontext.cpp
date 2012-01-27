/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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
#include "qmeegoplatforminputcontext.h"

#include <QtDebug>
#include <QTextCharFormat>
#include <QGuiApplication>
#include <qwindow.h>
#include <qevent.h>
#include <qscreen.h>

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

class QMeeGoPlatformInputContextPrivate
{
public:
    QMeeGoPlatformInputContextPrivate(QMeeGoPlatformInputContext *qq);
    ~QMeeGoPlatformInputContextPrivate()
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
    QWeakPointer<QWindow> window;
    QMeeGoPlatformInputContext *q;
};


QMeeGoPlatformInputContext::QMeeGoPlatformInputContext()
    : d(new QMeeGoPlatformInputContextPrivate(this))
{
    if (debug)
        qDebug() << "QMeeGoPlatformInputContext::QMeeGoPlatformInputContext()";
    QInputPanel *p = qApp->inputPanel();
    connect(p, SIGNAL(inputItemChanged()), this, SLOT(inputItemChanged()));
}

QMeeGoPlatformInputContext::~QMeeGoPlatformInputContext(void)
{
    delete d;
}

bool QMeeGoPlatformInputContext::isValid() const
{
    return d->valid;
}

void QMeeGoPlatformInputContext::invokeAction(QInputPanel::Action action, int x)
{
    QObject *input = qApp->inputPanel()->inputItem();
    if (!input)
        return;

    if (action == QInputPanel::Click) {
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

void QMeeGoPlatformInputContext::reset()
{
    QObject *input = qApp->inputPanel()->inputItem();

    const bool hadPreedit = !d->preedit.isEmpty();
    if (hadPreedit && input) {
        // ### selection
        QInputMethodEvent event;
        event.setCommitString(d->preedit);
        QGuiApplication::sendEvent(input, &event);
        d->preedit.clear();
    }

    QDBusPendingReply<void> reply = d->server->reset();
    if (hadPreedit)
        reply.waitForFinished();
}

void QMeeGoPlatformInputContext::update(Qt::InputMethodQueries queries)
{
    QInputPanel *panel = qApp->inputPanel();
    QObject *input = panel->inputItem();
    if (!input)
        return;

    QInputMethodQueryEvent query(queries);
    QGuiApplication::sendEvent(input, &query);

    if (queries & Qt::ImSurroundingText)
        d->imState["surroundingText"] = query.value(Qt::ImSurroundingText);
    if (queries & Qt::ImCursorPosition)
        d->imState["cursorPosition"] = query.value(Qt::ImCursorPosition);
    if (queries & Qt::ImAnchorPosition)
        d->imState["anchorPosition"] = query.value(Qt::ImAnchorPosition);
    if (queries & Qt::ImCursorRectangle) {
        QRect rect = query.value(Qt::ImCursorRectangle).toRect();
        rect = panel->inputItemTransform().mapRect(rect);
        QWindow *window = panel->inputWindow();
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

QRectF QMeeGoPlatformInputContext::keyboardRect() const
{
    return d->keyboardRect;
}

void QMeeGoPlatformInputContext::activationLostEvent()
{
    d->active = false;
    d->visibility = InputPanelHidden;
}

void QMeeGoPlatformInputContext::commitString(const QString &string, int replacementStart, int replacementLength, int cursorPos)
{
    QObject *input = qApp->inputPanel()->inputItem();
    if (!input)
        return;

    d->preedit.clear();

    if (debug)
        qWarning() << "CommitString" << string;
    // ### start/cursorPos
    QInputMethodEvent event;
    event.setCommitString(string, replacementStart, replacementLength);
    QCoreApplication::sendEvent(input, &event);
}

void QMeeGoPlatformInputContext::updatePreedit(const QDBusMessage &message)
{
    QObject *input = qApp->inputPanel()->inputItem();
    if (!input)
        return;

    QList<QVariant> arguments = message.arguments();
    if (arguments.count() != 5) {
        qWarning() << "QMeeGoPlatformInputContext::updatePreedit: Received message from input method server with wrong parameters.";
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
    QCoreApplication::sendEvent(input, &event);
}

void QMeeGoPlatformInputContext::copy()
{
    // Not supported at the moment.
}

void QMeeGoPlatformInputContext::imInitiatedHide()
{
    d->visibility = InputPanelHidden;
    emitInputPanelVisibleChanged();
    // ### clear focus
}

void QMeeGoPlatformInputContext::keyEvent(int, int, int, const QString &, bool, int, uchar)
{
    // Not supported at the moment.
}

void QMeeGoPlatformInputContext::paste()
{
    // Not supported at the moment.
}

bool QMeeGoPlatformInputContext::preeditRectangle(int &x, int &y, int &width, int &height)
{
    // ###
    QRect r = qApp->inputPanel()->cursorRectangle().toRect();
    if (!r.isValid())
        return false;
    x = r.x();
    y = r.y();
    width = r.width();
    height = r.height();
    return true;
}

bool QMeeGoPlatformInputContext::selection(QString &selection)
{
    selection.clear();

    QObject *input = qApp->inputPanel()->inputItem();
    if (!input)
        return false;

    QInputMethodQueryEvent query(Qt::ImCurrentSelection);
    QGuiApplication::sendEvent(input, &query);
    QVariant value = query.value(Qt::ImCurrentSelection);
    if (!value.isValid())
        return false;

    selection = value.toString();
    return true;
}

void QMeeGoPlatformInputContext::setDetectableAutoRepeat(bool)
{
    // Not supported.
}

void QMeeGoPlatformInputContext::setGlobalCorrectionEnabled(bool enable)
{
    d->correctionEnabled = enable;
}

void QMeeGoPlatformInputContext::setLanguage(const QString &)
{
    // Unused at the moment.
}

void QMeeGoPlatformInputContext::setRedirectKeys(bool)
{
    // Not supported.
}

void QMeeGoPlatformInputContext::setSelection(int start, int length)
{
    QObject *input = qApp->inputPanel()->inputItem();
    if (!input)
        return;

    QList<QInputMethodEvent::Attribute> attributes;
    attributes << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, start, length, QVariant());
    QInputMethodEvent event(QString(), attributes);
    QGuiApplication::sendEvent(input, &event);
}

void QMeeGoPlatformInputContext::updateInputMethodArea(int x, int y, int width, int height)
{
    d->keyboardRect = QRect(x, y, width, height);
    emitKeyboardRectChanged();
}

void QMeeGoPlatformInputContext::updateServerWindowOrientation(Qt::ScreenOrientation orientation)
{
    d->server->appOrientationChanged(orientationAngle(orientation));
}

void QMeeGoPlatformInputContext::inputItemChanged()
{
    if (!d->valid)
        return;

    QInputPanel *panel = qApp->inputPanel();
    QObject *input = panel->inputItem();
    QWindow *window = panel->inputWindow();
    if (window != d->window.data()) {
       if (d->window)
           disconnect(d->window.data(), SIGNAL(contentOrientationChanged(Qt::ScreenOrientation)),
                      this, SLOT(updateServerWindowOrientation(Qt::ScreenOrientation)));
        d->window = window;
        if (d->window)
            connect(d->window.data(), SIGNAL(contentOrientationChanged(Qt::ScreenOrientation)),
                    this, SLOT(updateServerWindowOrientation(Qt::ScreenOrientation)));
    }

    d->imState["focusState"] = input != 0;
    if (input) {
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
    if (input && window && d->visibility == InputPanelShowRequested)
        showInputPanel();
}

void QMeeGoPlatformInputContext::showInputPanel()
{
    if (debug)
        qDebug() << "showInputPanel";

    QInputPanel *panel = qApp->inputPanel();
    if (!panel->inputItem() || !panel->inputWindow())
        d->visibility = InputPanelShowRequested;
    else {
        d->server->showInputMethod();
        d->visibility = InputPanelShown;
        emitInputPanelVisibleChanged();
    }
}

void QMeeGoPlatformInputContext::hideInputPanel()
{
    d->server->hideInputMethod();
    d->visibility = InputPanelHidden;
    emitInputPanelVisibleChanged();
}

bool QMeeGoPlatformInputContext::isInputPanelVisible() const
{
    return d->visibility == InputPanelShown;
}

QMeeGoPlatformInputContextPrivate::QMeeGoPlatformInputContextPrivate(QMeeGoPlatformInputContext* qq)
    : connection(QDBusConnection::connectToPeer(QStringLiteral("unix:path=/tmp/meego-im-uiserver/imserver_dbus"), QLatin1String("MeeGoIMProxy")))
    , server(0)
    , adaptor(0)
    , visibility(InputPanelHidden)
    , valid(false)
    , active(false)
    , correctionEnabled(false)
    , q(qq)
{
    if (!connection.isConnected()) {
        qDebug("QMeeGoPlatformInputContext: not connected.");
        return;
    }

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

void QMeeGoPlatformInputContextPrivate::sendStateUpdate(bool focusChanged)
{
    server->updateWidgetInformation(imState, focusChanged);
}

QT_END_NAMESPACE
