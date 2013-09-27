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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <QCoreApplication>
#include <QLoggingCategory>


//![1]
QLoggingCategory theFooArea("foo");
QLoggingCategory theBarArea("bar");
QLoggingCategory theBazArea("baz");
//![1]

// Note: These locations are Ubuntu specific.

// Note: To make the example work with user permissions, make sure
// the files are user-writable and the path leading there accessible.

const char traceSwitch[] = "/sys/kernel/debug/tracing/tracing_on";
const char traceSink[] = "/sys/kernel/debug/tracing/trace_marker";

// The base class only serves as a facility to share code
// between the "single line" data logging aspect and the
// scoped "measuring" aspect.

// Both aspects and the base class could be combined into
// a single tracer serving both purposes, but are split
// here for clarity.

// Error handling is left as an exercise.

//![2]
class MyTracerBase : public QTracer
{
public:
    MyTracerBase() {
        enable = ::open(traceSwitch, O_WRONLY);
        marker = ::open(traceSink, O_WRONLY);
    }

    ~MyTracerBase() {
        ::close(enable);
        ::close(marker);
    }

protected:
    int enable;
    int marker;
};
//![2]


//![2]
class MyTracer : public MyTracerBase
{
public:
    void start() { ::write(marker, "B", 1); }
    void end() { ::write(marker, "E", 1); }
};
//![2]


//![3]
class MyDataLogger : public MyTracerBase
{
public:
    MyDataLogger() {
        buf[0] = 0;
        pos = 0;
    }

    void record(int i) { pos += sprintf(buf + pos, "%d", i); }
    void record(const char *msg) { pos += sprintf(buf + pos, "%s", msg); }
    void end() { ::write(marker, buf, pos); pos = 0; }

private:
    char buf[100];
    int pos;
};
//![3]

// Simplest possible example for "measuring".
//![4]
int foo(int i)
{
    qCTraceGuard(theFooArea);
    // Here could be some lengthy code.
    return i * i;
}
//![4]

// We can switch on/off tracing dynamically.
// The tracer will be temporarily switched off at the third call
// and re-enabled at the eighth.
//![5]
int bar(int i)
{
    static int n = 0;
    ++n;
    if (n == 3)
        theBarArea.setEnabled(QtTraceMsg, false);
    if (n == 8)
        theBarArea.setEnabled(QtTraceMsg, true);

    qCTraceGuard(theBarArea);
    return i * i;
}
//![5]

// An example to create "complex" log messages.
//![6]
int baz(int i)
{
    qCTrace(theBazArea) << 32 << "some stuff";

    return i * i;
}
//![6]



//![7]
namespace {
static struct Init
{
    Init() {
        tracer.addToCategory(theFooArea);
        tracer.addToCategory(theBarArea);
        logger.addToCategory(theBazArea);
    }

    MyTracer tracer;
    MyDataLogger logger;

} initializer;
}
//![7]
