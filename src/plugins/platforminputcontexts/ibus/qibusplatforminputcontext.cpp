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
#include "qibusplatforminputcontext.h"

#include <QtDebug>
#include <QTextCharFormat>
#include <QGuiApplication>
#include <qwindow.h>
#include <qevent.h>

#include "qibusproxy.h"
#include "qibusinputcontextproxy.h"
#include "qibustypes.h"

#include <sys/types.h>
#include <signal.h>

#include <QtDBus>

enum { debug = 0 };

class QIBusPlatformInputContextPrivate
{
public:
    QIBusPlatformInputContextPrivate();
    ~QIBusPlatformInputContextPrivate()
    {
        delete context;
        delete bus;
        delete connection;
    }

    static QDBusConnection *createConnection();

    QDBusConnection *connection;
    QIBusProxy *bus;
    QIBusInputContextProxy *context;

    bool valid;
};


QIBusPlatformInputContext::QIBusPlatformInputContext ()
    : d(new QIBusPlatformInputContextPrivate())
{
    if (d->context) {
        connect(d->context, SIGNAL(CommitText(QDBusVariant)), SLOT(commitText(QDBusVariant)));
        connect(d->context, SIGNAL(UpdatePreeditText(QDBusVariant,uint,bool)), this, SLOT(updatePreeditText(QDBusVariant,uint,bool)));
    }
    QInputPanel *p = qApp->inputPanel();
    connect(p, SIGNAL(inputItemChanged()), this, SLOT(inputItemChanged()));
    connect(p, SIGNAL(cursorRectangleChanged()), this, SLOT(cursorRectChanged()));
}

QIBusPlatformInputContext::~QIBusPlatformInputContext (void)
{
    delete d;
}

bool QIBusPlatformInputContext::isValid() const
{
    return d->valid;
}

void QIBusPlatformInputContext::invokeAction(QInputPanel::Action a, int x)
{
    QPlatformInputContext::invokeAction(a, x);

    if (!d->valid)
        return;
}

void QIBusPlatformInputContext::reset()
{
    QPlatformInputContext::reset();

    if (!d->valid)
        return;

    d->context->Reset();
}

void QIBusPlatformInputContext::update(Qt::InputMethodQueries q)
{
    QPlatformInputContext::update(q);
}

void QIBusPlatformInputContext::cursorRectChanged()
{
    if (!d->valid)
        return;

    QRect r = qApp->inputPanel()->cursorRectangle().toRect();
    if(!r.isValid())
        return;

    QWindow *inputWindow = qApp->inputPanel()->inputWindow();
    r.moveTopLeft(inputWindow->mapToGlobal(r.topLeft()));
    if (debug)
        qDebug() << "microFocus" << r;
    d->context->SetCursorLocation(r.x(), r.y(), r.width(), r.height());
}

void QIBusPlatformInputContext::inputItemChanged()
{
    if (!d->valid)
        return;

    QObject *input = qApp->inputPanel()->inputItem();
    if (debug)
        qDebug() << "setFocusObject" << input;
    if (input)
        d->context->FocusIn();
    else
        d->context->FocusOut();
}


void QIBusPlatformInputContext::commitText(const QDBusVariant &text)
{
    QObject *input = qApp->inputPanel()->inputItem();
    if (!input)
        return;

    const QDBusArgument arg = text.variant().value<QDBusArgument>();

    QIBusText t;
    if (debug)
        qDebug() << arg.currentSignature();
    t.fromDBusArgument(arg);
    if (debug)
        qDebug() << "commit text:" << t.text;

    QInputMethodEvent event;
    event.setCommitString(t.text);
    QCoreApplication::sendEvent(input, &event);
}

void QIBusPlatformInputContext::updatePreeditText(const QDBusVariant &text, uint cursorPos, bool visible)
{
    QObject *input = qApp->inputPanel()->inputItem();
    if (!input)
        return;

    const QDBusArgument arg = text.variant().value<QDBusArgument>();

    QIBusText t;
    t.fromDBusArgument(arg);
    if (debug)
        qDebug() << "preedit text:" << t.text;

    QList<QInputMethodEvent::Attribute> attributes = t.attributes.imAttributes();
    attributes += QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, cursorPos, visible ? 1 : 0, QVariant());

    QInputMethodEvent event(t.text, attributes);
    QCoreApplication::sendEvent(input, &event);
}


/* Kernel keycode -> X keycode table */
static const unsigned int keycode_table[256] = {
      0,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21, 22,  23,
     24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37, 38,  39,
     40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53, 54,  55,
     56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69, 70,  71,
     72,  73,  74,  75,  76,  77,  76,  79,  80,  81,  82,  83,  84,  85, 86,  87,
     88,  89,  90,  91, 111,  221, 94,  95,  96, 211, 128, 127, 129, 208, 131, 126,
    108, 109, 112, 111, 113, 181,  97,  98,  99, 100, 102, 103, 104, 105, 106, 107,
    239, 160, 174, 176, 222, 157, 123, 110, 139, 134, 209, 210, 133, 115, 116, 117,
    232, 133, 134, 135, 140, 248, 191, 192, 122, 188, 245, 158, 161, 193, 223, 227,
    198, 199, 200, 147, 159, 151, 178, 201, 146, 203, 166, 236, 230, 235, 234, 233,
    163, 204, 253, 153, 162, 144, 164, 177, 152, 190, 208, 129, 130, 231, 209, 210,
    136, 220, 143, 246, 251, 137, 138, 182, 183, 184,  93, 184, 247, 132, 170, 219,
    249, 205, 207, 149, 150, 154, 155, 167, 168, 169, 171, 172, 173, 165, 175, 179,
    180,   0, 185, 186, 187, 118, 119, 120, 121, 229, 194, 195, 196, 197, 148, 202,
    101, 212, 237, 214, 215, 216, 217, 218, 228, 142, 213, 240, 241, 242, 243, 244,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

bool
QIBusPlatformInputContext::x11FilterEvent(uint keyval, uint keycode, uint state, bool press)
{
    if (!d->valid)
        return false;

    if (!press)
        return false;

    keycode -= 8; // ###
    QDBusReply<bool> reply = d->context->ProcessKeyEvent(keyval, keycode, state);

//    qDebug() << "x11FilterEvent return" << reply.value();

    return reply.value();
}

QIBusPlatformInputContextPrivate::QIBusPlatformInputContextPrivate()
    : connection(createConnection()),
      bus(0),
      context(0),
      valid(false)
{
    if (!connection || !connection->isConnected()) {
        qDebug("QIBusPlatformInputContext: not connected.");
        return;
    }

    bus = new QIBusProxy(QLatin1String("org.freedesktop.IBus"),
                         QLatin1String("/org/freedesktop/IBus"),
                         *connection);
    if (!bus->isValid()) {
        qWarning("QIBusPlatformInputContext: invalid bus.");
        return;
    }

    QDBusReply<QDBusObjectPath> ic = bus->CreateInputContext(QLatin1String("QIBusInputContext"));
    if (!ic.isValid()) {
        qWarning("QIBusPlatformInputContext: CreateInputContext failed.");
        return;
    }

    context = new QIBusInputContextProxy(QLatin1String("org.freedesktop.IBus"), ic.value().path(), *connection);

    if (!context->isValid()) {
        qWarning("QIBusPlatformInputContext: invalid input context.");
        return;
    }

    enum Capabilities {
        IBUS_CAP_PREEDIT_TEXT       = 1 << 0,
        IBUS_CAP_AUXILIARY_TEXT     = 1 << 1,
        IBUS_CAP_LOOKUP_TABLE       = 1 << 2,
        IBUS_CAP_FOCUS              = 1 << 3,
        IBUS_CAP_PROPERTY           = 1 << 4,
        IBUS_CAP_SURROUNDING_TEXT   = 1 << 5
    };
    context->SetCapabilities(IBUS_CAP_PREEDIT_TEXT|IBUS_CAP_FOCUS);

    if (debug)
        qDebug(">>>> valid!");
    valid = true;
}

QDBusConnection *QIBusPlatformInputContextPrivate::createConnection()
{
    QByteArray display(getenv("DISPLAY"));
    QByteArray host = "unix";
    QByteArray displayNumber = "0";

    int pos = display.indexOf(':');
    if (pos > 0)
        host = display.left(pos);
    ++pos;
    int pos2 = display.indexOf('.', pos);
    if (pos2 > 0)
        displayNumber = display.mid(pos, pos2 - pos);
    if (debug)
        qDebug() << "host=" << host << "displayNumber" << displayNumber;

    QFile file(QDir::homePath() + QLatin1String("/.config/ibus/bus/") +
               QLatin1String(QDBusConnection::localMachineId()) +
               QLatin1Char('-') + QString::fromLocal8Bit(host) + QLatin1Char('-') + QString::fromLocal8Bit(displayNumber));

    if (!file.exists()) {
        qWarning("QIBusPlatformInputContext: ibus config file '%s' does not exist.", qPrintable(file.fileName()));
        return 0;
    }

    file.open(QFile::ReadOnly);

    QByteArray address;
    int pid = -1;

    while (!file.atEnd()) {
        QByteArray line = file.readLine().trimmed();
        if (line.startsWith('#'))
            continue;

        if (line.startsWith("IBUS_ADDRESS="))
            address = line.mid(sizeof("IBUS_ADDRESS=") - 1);
        if (line.startsWith("IBUS_DAEMON_PID="))
            pid = line.mid(sizeof("IBUS_DAEMON_PID=") - 1).toInt();
    }

    if (debug)
        qDebug() << "IBUS_ADDRESS=" << address << "PID=" << pid;
    if (address.isEmpty() || pid < 0 || kill(pid, 0) != 0)
        return 0;

    return new QDBusConnection(QDBusConnection::connectToBus(QString::fromLatin1(address), QLatin1String("QIBusProxy")));
}
