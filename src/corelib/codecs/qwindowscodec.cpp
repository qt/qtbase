/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qwindowscodec_p.h"
#include <qvarlengtharray.h>
#include <qstring.h>
#include <qbytearray.h>
#include <qt_windows.h>

QT_BEGIN_NAMESPACE

QWindowsLocalCodec::QWindowsLocalCodec()
{
}

QWindowsLocalCodec::~QWindowsLocalCodec()
{
}

QString QWindowsLocalCodec::convertToUnicode(const char *chars, int length, ConverterState *state) const
{
    const char *mb = chars;
    int mblen = length;

    if (!mb || !mblen)
        return QString();

    QVarLengthArray<wchar_t, 4096> wc(4096);
    int len;
    QString sp;
    bool prepend = false;
    char state_data = 0;
    int remainingChars = 0;

    //save the current state information
    if (state) {
        state_data = (char)state->state_data[0];
        remainingChars = state->remainingChars;
    }

    //convert the pending charcter (if available)
    if (state && remainingChars) {
        char prev[3] = {0};
        prev[0] = state_data;
        prev[1] = mb[0];
        remainingChars = 0;
        len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                                    prev, 2, wc.data(), wc.length());
        if (len) {
            prepend = true;
            sp.append(QChar(wc[0]));
            mb++;
            mblen--;
            wc[0] = 0;
        }
    }

    while (!(len=MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED|MB_ERR_INVALID_CHARS,
                mb, mblen, wc.data(), wc.length()))) {
        int r = GetLastError();
        if (r == ERROR_INSUFFICIENT_BUFFER) {
                const int wclen = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                                    mb, mblen, 0, 0);
                wc.resize(wclen);
        } else if (r == ERROR_NO_UNICODE_TRANSLATION) {
            //find the last non NULL character
            while (mblen > 1  && !(mb[mblen-1]))
                mblen--;
            //check whether,  we hit an invalid character in the middle
            if ((mblen <= 1) || (remainingChars && state_data))
                return convertToUnicodeCharByChar(chars, length, state);
            //Remove the last character and try again...
            state_data = mb[mblen-1];
            remainingChars = 1;
            mblen--;
        } else {
            // Fail.
            qWarning("MultiByteToWideChar: Cannot convert multibyte text");
            break;
        }
    }

    if (len <= 0)
        return QString();

    if (wc[len-1] == 0) // len - 1: we don't want terminator
        --len;

    //save the new state information
    if (state) {
        state->state_data[0] = (char)state_data;
        state->remainingChars = remainingChars;
    }
    QString s((QChar*)wc.data(), len);
    if (prepend) {
        return sp+s;
    }
    return s;
}

QString QWindowsLocalCodec::convertToUnicodeCharByChar(const char *chars, int length, ConverterState *state) const
{
    if (!chars || !length)
        return QString();

    int copyLocation = 0;
    int extra = 2;
    if (state && state->remainingChars) {
        copyLocation = state->remainingChars;
        extra += copyLocation;
    }
    int newLength = length + extra;
    char *mbcs = new char[newLength];
    //ensure that we have a NULL terminated string
    mbcs[newLength-1] = 0;
    mbcs[newLength-2] = 0;
    memcpy(&(mbcs[copyLocation]), chars, length);
    if (copyLocation) {
        //copy the last character from the state
        mbcs[0] = (char)state->state_data[0];
        state->remainingChars = 0;
    }
    const char *mb = mbcs;
#if !defined(Q_OS_WINRT)
    const char *next = 0;
    QString s;
    while ((next = CharNextExA(CP_ACP, mb, 0)) != mb) {
        wchar_t wc[2] ={0};
        int charlength = next - mb;
        int len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED|MB_ERR_INVALID_CHARS, mb, charlength, wc, 2);
        if (len>0) {
            s.append(QChar(wc[0]));
        } else {
            int r = GetLastError();
            //check if the character being dropped is the last character
            if (r == ERROR_NO_UNICODE_TRANSLATION && mb == (mbcs+newLength -3) && state) {
                state->remainingChars = 1;
                state->state_data[0] = (char)*mb;
            }
        }
        mb = next;
    }
#else
    QString s;
    size_t size = mbstowcs(NULL, mb, length);
    if (size < 0) {
        Q_ASSERT("Error in CE TextCodec");
        return QString();
    }
    wchar_t* ws = new wchar_t[size + 2];
    ws[size +1] = 0;
    ws[size] = 0;
    size = mbstowcs(ws, mb, length);
    for (size_t i = 0; i < size; i++)
        s.append(QChar(ws[i]));
    delete [] ws;
#endif
    delete [] mbcs;
    return s;
}

QByteArray QWindowsLocalCodec::convertFromUnicode(const QChar *ch, int uclen, ConverterState *) const
{
    if (!ch)
        return QByteArray();
    if (uclen == 0)
        return QByteArray("");
    BOOL used_def;
    QByteArray mb(4096, 0);
    int len;
    while (!(len=WideCharToMultiByte(CP_ACP, 0, (const wchar_t*)ch, uclen,
                mb.data(), mb.size()-1, 0, &used_def)))
    {
        int r = GetLastError();
        if (r == ERROR_INSUFFICIENT_BUFFER) {
            mb.resize(1+WideCharToMultiByte(CP_ACP, 0,
                                (const wchar_t*)ch, uclen,
                                0, 0, 0, &used_def));
                // and try again...
        } else {
            // Fail.  Probably can't happen in fact (dwFlags is 0).
#ifndef QT_NO_DEBUG
            // Can't use qWarning(), as it'll recurse to handle %ls
            fprintf(stderr,
                    "WideCharToMultiByte: Cannot convert multibyte text (error %d): %ls\n",
                    r, reinterpret_cast<const wchar_t*>(QString(ch, uclen).utf16()));
#endif
            break;
        }
    }
    mb.resize(len);
    return mb;
}


QByteArray QWindowsLocalCodec::name() const
{
    return "System";
}

int QWindowsLocalCodec::mibEnum() const
{
    return 0;
}

QT_END_NAMESPACE
