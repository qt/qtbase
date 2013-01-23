/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "thread.h"

/*
 * QThread derived class with additional capability to move a QObject to the
 * new thread, to stop the thread and move the QObject back to the thread where
 *it came from.
 */

Thread::Thread( QObject *parent)
    : QThread (parent)
{
    //we need a class that receives signals from other threads and emits a signal in response
    shutDownHelper=new QSignalMapper;
    shutDownHelper->setMapping(this,0);
    connect(this, SIGNAL(started()), this, SLOT(setReadyStatus() ), Qt::DirectConnection);
    connect(this, SIGNAL(aboutToStop()), shutDownHelper, SLOT(map()) );
}

//------------------------------------------------------
Thread::~Thread()
{
    delete shutDownHelper;
}

//------------------------------------------------------
// starts thread, moves worker to this thread and blocks
void Thread::launchWorker(QObject *worker)
{
    this->worker = worker;
    start();
    worker->moveToThread(this);
    shutDownHelper->moveToThread(this);
    connect(shutDownHelper, SIGNAL(mapped(int) ), this, SLOT(stopExecutor()), Qt::DirectConnection );
    mutex.lock();
    waitCondition.wait(&mutex);
}

//------------------------------------------------------
// puts a command to stop processing in the event queue of worker thread
void Thread::stop()
{
    emit aboutToStop();
}

//------------------------------------------------------

// methods above this line should be called in  gui thread context
// methods below this line are private and will be run in  secondary thread context

//------------------------------------------------------
void Thread::stopExecutor()  //secondary thread context
{
    exit();
}

//------------------------------------------------------
void Thread::setReadyStatus()
{
    waitCondition.wakeAll();
}
