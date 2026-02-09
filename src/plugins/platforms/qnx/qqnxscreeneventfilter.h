// Copyright (C) 2013 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQNXSCREENEVENTFILTER_H
#define QQNXSCREENEVENTFILTER_H

QT_BEGIN_NAMESPACE

class QQnxScreenEventFilter
{
protected:
    ~QQnxScreenEventFilter() {}

public:
    virtual bool handleKeyboardEvent(int flags, int sym, int mod, int scan, int cap, int sequenceId) = 0;
};

QT_END_NAMESPACE

#endif
