/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef VIRTUALKEYBOARDPPS_H
#define VIRTUALKEYBOARDPPS_H

#include "qqnxabstractvirtualkeyboard.h"

#include <sys/pps.h>

QT_BEGIN_NAMESPACE


class QSocketNotifier;

class QQnxVirtualKeyboardPps : public QQnxAbstractVirtualKeyboard
{
    Q_OBJECT
public:
    QQnxVirtualKeyboardPps();
    ~QQnxVirtualKeyboardPps();

    bool showKeyboard() override;
    bool hideKeyboard() override;

public Q_SLOTS:
    void start();

protected:
    void applyKeyboardOptions() override;

private Q_SLOTS:
    void ppsDataReady();

private:
    // Will be called internally if needed.
    bool connect();
    void close();
    bool queryPPSInfo();
    void handleKeyboardInfoMessage();

    const char* keyboardModeStr() const;
    const char* enterKeyTypeStr() const;

    bool prepareToSend();
    bool writeCurrentPPSEncoder();

    pps_encoder_t  *m_encoder;
    pps_decoder_t  *m_decoder;
    char           *m_buffer;
    int             m_fd;
    QSocketNotifier *m_readNotifier;

    // Path to keyboardManager in PPS.
    static const char *ms_PPSPath;
    static const size_t ms_bufferSize;
};

QT_END_NAMESPACE

#endif // VIRTUALKEYBOARDPPS_H
