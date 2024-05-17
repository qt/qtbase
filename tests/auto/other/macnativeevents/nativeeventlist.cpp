// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "nativeeventlist.h"

NativeEventList::NativeEventList(int defaultWaitMs)
    : playbackMultiplier(1.0)
    , currIndex(-1)
    , wait(false)
    , defaultWaitMs(defaultWaitMs)
{
    debug = qgetenv("NATIVEDEBUG").toInt();
    QString multiplier = qgetenv("NATIVEDEBUGSPEED");
    if (!multiplier.isEmpty())
        setTimeMultiplier(multiplier.toFloat());
}

NativeEventList::~NativeEventList()
{
    for (int i=0; i<eventList.size(); i++)
        delete eventList.takeAt(i).second;
}

void NativeEventList::sendNextEvent()
{
    QNativeEvent *e = eventList.at(currIndex).second;
    if (e) {
        if (debug > 0)
            qDebug() << "Sending:" << *e;
        QNativeInput::sendNativeEvent(*e);
    }
    waitNextEvent();
}

void NativeEventList::waitNextEvent()
{
    if (++currIndex >= eventList.size()){
        emit done();
        stop();
        return;
    }

    int interval = eventList.at(currIndex).first;
    QTimer::singleShot(interval * playbackMultiplier, this, SLOT(sendNextEvent()));
}

void NativeEventList::append(QNativeEvent *event)
{
    eventList.append(QPair<int, QNativeEvent *>(defaultWaitMs, event));
}

void NativeEventList::append(int waitMs, QNativeEvent *event)
{
    eventList.append(QPair<int, QNativeEvent *>(waitMs, event));
}

void NativeEventList::play(Playback playback)
{
    waitNextEvent();

    wait = (playback == WaitUntilFinished);
    while (wait)
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
}

void NativeEventList::stop()
{
    wait = false;
    QAbstractEventDispatcher::instance()->interrupt();
}

void NativeEventList::setTimeMultiplier(float multiplier)
{
    playbackMultiplier = multiplier;
}

