/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "signalbug.h"

#include <qcoreapplication.h>
#include <qstring.h>

#include <stdio.h>

static int Step = 0;
Sender RandomSender (0, 0);

void TRACE (int step, const char *name)
{
    for (int t = 0; t < step - 1; t++)
        fprintf (stderr, "\t");
    fprintf (stderr, "Step %d: %s\n", step, name);
    return;
}

Receiver::Receiver ()
    : QObject ()
{
}

void Receiver::received ()
{
    ::Step++;
    const int stepCopy = ::Step;
    TRACE (stepCopy, "Receiver::received()");
    if (::Step != 2 && ::Step != 4)
        qFatal("%s: Incorrect Step: %d (should be 2 or 4)", Q_FUNC_INFO, ::Step);

    if (::Step == 2)
        s->fire ();

    fprintf (stderr, "Receiver<%s>::received() sender=%s\n",
        (const char *) objectName ().toLatin1 (), sender ()->metaObject()->className());

    TRACE (stepCopy, "ends Receiver::received()");
}

Disconnector::Disconnector ()
    : QObject ()
{
}

void Disconnector::received ()
{
    ::Step++;
    const int stepCopy = ::Step;
    TRACE (stepCopy, "Disconnector::received()");
    if (::Step != 5 && ::Step != 6)
        qFatal("%s: Incorrect Step: %d (should be 5 or 6)", Q_FUNC_INFO, ::Step);

    fprintf (stderr, "Disconnector<%s>::received() sender=%s\n",
        (const char *) objectName ().toLatin1 (), sender ()->metaObject()->className());
    if (sender () == 0)
        fprintf (stderr, "WE SHOULD NOT BE RECEIVING THIS SIGNAL\n");

    if (::Step == 5)
    {
        disconnect (s, SIGNAL(fired()), s->d, SLOT(received()));

        connect (&RandomSender, SIGNAL(fired()), s->d, SLOT(received()));
    }

    TRACE (stepCopy, "ends Disconnector::received()");
}


Sender::Sender (Receiver *r, Disconnector *d)
    : QObject ()
{
    this->r = r; this->d = d;
    if (r)
        connect (this, SIGNAL(fired()), r, SLOT(received()));
    if (d)
        connect (this, SIGNAL(fired()), d, SLOT(received()));
};

void Sender::fire ()
{
    ::Step++;
    const int stepCopy = ::Step;
    TRACE (stepCopy, "Sender::fire()");
    if (::Step != 1 && ::Step != 3)
        qFatal("%s: Incorrect Step: %d (should be 1 or 3)", Q_FUNC_INFO, ::Step);

    emit fired ();
    TRACE (stepCopy, "ends Sender::fire()");
}

int main (int argc, char *argv [])
{
    QCoreApplication app (argc, argv);

    Receiver r;
    Disconnector d;
    Sender s (&r, &d);

    r.s = &s;
    d.s = &s;

    ::Step = 0;
    s.fire ();
    return 0;
}
