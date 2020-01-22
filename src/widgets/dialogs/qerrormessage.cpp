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

#include "qerrormessage.h"

#include "qapplication.h"
#include "qcheckbox.h"
#include "qlabel.h"
#include "qlayout.h"
#if QT_CONFIG(messagebox)
#include "qmessagebox.h"
#endif
#include "qpushbutton.h"
#include "qstringlist.h"
#include "qtextedit.h"
#include "qdialog_p.h"
#include "qpixmap.h"
#include "qmetaobject.h"
#include "qthread.h"
#include "qset.h"

#include <queue>

#include <stdio.h>
#include <stdlib.h>

QT_BEGIN_NAMESPACE

namespace {
struct Message {
    QString content;
    QString type;
};
}

class QErrorMessagePrivate : public QDialogPrivate
{
    Q_DECLARE_PUBLIC(QErrorMessage)
public:
    QPushButton * ok;
    QCheckBox * again;
    QTextEdit * errors;
    QLabel * icon;
    std::queue<Message> pending;
    QSet<QString> doNotShow;
    QSet<QString> doNotShowType;
    QString currentMessage;
    QString currentType;

    bool isMessageToBeShown(const QString &message, const QString &type) const;
    bool nextPending();
    void retranslateStrings();
};

namespace {
class QErrorMessageTextView : public QTextEdit
{
public:
    QErrorMessageTextView(QWidget *parent)
        : QTextEdit(parent) { setReadOnly(true); }

    virtual QSize minimumSizeHint() const override;
    virtual QSize sizeHint() const override;
};
} // unnamed namespace

QSize QErrorMessageTextView::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize QErrorMessageTextView::sizeHint() const
{
    return QSize(250, 75);
}

/*!
    \class QErrorMessage

    \brief The QErrorMessage class provides an error message display dialog.

    \ingroup standard-dialog
    \inmodule QtWidgets

    An error message widget consists of a text label and a checkbox. The
    checkbox lets the user control whether the same error message will be
    displayed again in the future, typically displaying the text,
    "Show this message again" translated into the appropriate local
    language.

    For production applications, the class can be used to display messages which
    the user only needs to see once. To use QErrorMessage like this, you create
    the dialog in the usual way, and show it by calling the showMessage() slot or
    connecting signals to it.

    The static qtHandler() function installs a message handler
    using qInstallMessageHandler() and creates a QErrorMessage that displays
    qDebug(), qWarning() and qFatal() messages. This is most useful in
    environments where no console is available to display warnings and
    error messages.

    In both cases QErrorMessage will queue pending messages and display
    them in order, with each new message being shown as soon as the user
    has accepted the previous message. Once the user has specified that a
    message is not to be shown again it is automatically skipped, and the
    dialog will show the next appropriate message in the queue.

    The \l{dialogs/standarddialogs}{Standard Dialogs} example shows
    how to use QErrorMessage as well as other built-in Qt dialogs.

    \image qerrormessage.png

    \sa QMessageBox, QStatusBar::showMessage(), {Standard Dialogs Example}
*/

static QErrorMessage * qtMessageHandler = nullptr;

static void deleteStaticcQErrorMessage() // post-routine
{
    if (qtMessageHandler) {
        delete qtMessageHandler;
        qtMessageHandler = nullptr;
    }
}

static bool metFatal = false;

static QString msgType2i18nString(QtMsgType t)
{
    Q_STATIC_ASSERT(QtDebugMsg == 0);
    Q_STATIC_ASSERT(QtWarningMsg == 1);
    Q_STATIC_ASSERT(QtCriticalMsg == 2);
    Q_STATIC_ASSERT(QtFatalMsg == 3);
    Q_STATIC_ASSERT(QtInfoMsg == 4);

    // adjust the array below if any of the above fire...

    const char * const messages[] = {
        QT_TRANSLATE_NOOP("QErrorMessage", "Debug Message:"),
        QT_TRANSLATE_NOOP("QErrorMessage", "Warning:"),
        QT_TRANSLATE_NOOP("QErrorMessage", "Critical Error:"),
        QT_TRANSLATE_NOOP("QErrorMessage", "Fatal Error:"),
        QT_TRANSLATE_NOOP("QErrorMessage", "Information:"),
    };
    Q_ASSERT(size_t(t) < sizeof messages / sizeof *messages);

    return QCoreApplication::translate("QErrorMessage", messages[t]);
}

static void jump(QtMsgType t, const QMessageLogContext & /*context*/, const QString &m)
{
    if (!qtMessageHandler)
        return;

    QString rich = QLatin1String("<p><b>") + msgType2i18nString(t) + QLatin1String("</b></p>")
                   + Qt::convertFromPlainText(m, Qt::WhiteSpaceNormal);

    // ### work around text engine quirk
    if (rich.endsWith(QLatin1String("</p>")))
        rich.chop(4);

    if (!metFatal) {
        if (QThread::currentThread() == qApp->thread()) {
            qtMessageHandler->showMessage(rich);
        } else {
            QMetaObject::invokeMethod(qtMessageHandler,
                                      "showMessage",
                                      Qt::QueuedConnection,
                                      Q_ARG(QString, rich));
        }
        metFatal = (t == QtFatalMsg);
    }
}


/*!
    Constructs and installs an error handler window with the given \a
    parent.
*/

QErrorMessage::QErrorMessage(QWidget * parent)
    : QDialog(*new QErrorMessagePrivate, parent)
{
    Q_D(QErrorMessage);

    d->icon = new QLabel(this);
    d->errors = new QErrorMessageTextView(this);
    d->again = new QCheckBox(this);
    d->ok = new QPushButton(this);
    QGridLayout * grid = new QGridLayout(this);

    connect(d->ok, SIGNAL(clicked()), this, SLOT(accept()));

    grid->addWidget(d->icon,   0, 0, Qt::AlignTop);
    grid->addWidget(d->errors, 0, 1);
    grid->addWidget(d->again,  1, 1, Qt::AlignTop);
    grid->addWidget(d->ok,     2, 0, 1, 2, Qt::AlignCenter);
    grid->setColumnStretch(1, 42);
    grid->setRowStretch(0, 42);

#if QT_CONFIG(messagebox)
    d->icon->setPixmap(QMessageBox::standardIcon(QMessageBox::Information));
    d->icon->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
#endif
    d->again->setChecked(true);
    d->ok->setFocus();

    d->retranslateStrings();
}


/*!
    Destroys the error message dialog.
*/

QErrorMessage::~QErrorMessage()
{
    if (this == qtMessageHandler) {
        qtMessageHandler = nullptr;
        QtMessageHandler tmp = qInstallMessageHandler(nullptr);
        // in case someone else has later stuck in another...
        if (tmp != jump)
            qInstallMessageHandler(tmp);
    }
}


/*! \reimp */

void QErrorMessage::done(int a)
{
    Q_D(QErrorMessage);
    if (!d->again->isChecked()) {
        if (d->currentType.isEmpty()) {
            if (!d->currentMessage.isEmpty())
                d->doNotShow.insert(d->currentMessage);
        } else {
            d->doNotShowType.insert(d->currentType);
        }
    }
    d->currentMessage.clear();
    d->currentType.clear();
    if (!d->nextPending()) {
        QDialog::done(a);
        if (this == qtMessageHandler && metFatal)
            exit(1);
    }
}


/*!
    Returns a pointer to a QErrorMessage object that outputs the
    default Qt messages. This function creates such an object, if there
    isn't one already.
*/

QErrorMessage * QErrorMessage::qtHandler()
{
    if (!qtMessageHandler) {
        qtMessageHandler = new QErrorMessage(nullptr);
        qAddPostRoutine(deleteStaticcQErrorMessage); // clean up
        qtMessageHandler->setWindowTitle(QCoreApplication::applicationName());
        qInstallMessageHandler(jump);
    }
    return qtMessageHandler;
}


/*! \internal */

bool QErrorMessagePrivate::isMessageToBeShown(const QString &message, const QString &type) const
{
    return !message.isEmpty()
        && (type.isEmpty() ? !doNotShow.contains(message) : !doNotShowType.contains(type));
}

bool QErrorMessagePrivate::nextPending()
{
    while (!pending.empty()) {
        QString message = std::move(pending.front().content);
        QString type = std::move(pending.front().type);
        pending.pop();
        if (isMessageToBeShown(message, type)) {
#ifndef QT_NO_TEXTHTMLPARSER
            errors->setHtml(message);
#else
            errors->setPlainText(message);
#endif
            currentMessage = std::move(message);
            currentType = std::move(type);
            return true;
        }
    }
    return false;
}


/*!
    Shows the given message, \a message, and returns immediately. If the user
    has requested for the message not to be shown again, this function does
    nothing.

    Normally, the message is displayed immediately. However, if there are
    pending messages, it will be queued to be displayed later.
*/

void QErrorMessage::showMessage(const QString &message)
{
    showMessage(message, QString());
}

/*!
    \since 4.5
    \overload

    Shows the given message, \a message, and returns immediately. If the user
    has requested for messages of type, \a type, not to be shown again, this
    function does nothing.

    Normally, the message is displayed immediately. However, if there are
    pending messages, it will be queued to be displayed later.

    \sa showMessage()
*/

void QErrorMessage::showMessage(const QString &message, const QString &type)
{
    Q_D(QErrorMessage);
    if (!d->isMessageToBeShown(message, type))
        return;
    d->pending.push({message, type});
    if (!isVisible() && d->nextPending())
        show();
}

/*!
    \reimp
*/
void QErrorMessage::changeEvent(QEvent *e)
{
    Q_D(QErrorMessage);
    if (e->type() == QEvent::LanguageChange) {
        d->retranslateStrings();
    }
    QDialog::changeEvent(e);
}

void QErrorMessagePrivate::retranslateStrings()
{
    again->setText(QErrorMessage::tr("&Show this message again"));
    ok->setText(QErrorMessage::tr("&OK"));
}

QT_END_NAMESPACE

#include "moc_qerrormessage.cpp"
