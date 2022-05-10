// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef Q_NATIVE_PLAYBACK
#define Q_NATIVE_PLAYBACK

#include <QtCore>
#include "qnativeevents.h"

class NativeEventList : public QObject
{
    Q_OBJECT;

    public:
    enum Playback {ReturnImmediately, WaitUntilFinished};

    NativeEventList(int defaultWaitMs = 20);
    ~NativeEventList();

    void append(QNativeEvent *event);
    void append(int waitMs, QNativeEvent *event = nullptr);

    void play(Playback playback = WaitUntilFinished);
    void stop();
    void setTimeMultiplier(float multiplier);

signals:
    void done();

private slots:
    void sendNextEvent();

private:
    void waitNextEvent();

    QList<QPair<int, QNativeEvent *> > eventList;
    float playbackMultiplier;
    int currIndex;
    bool wait;
    int defaultWaitMs;
    int debug;
};

#endif
