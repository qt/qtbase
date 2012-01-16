/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the documentation of the Qt Toolkit.
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
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
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
** $QT_END_LICENSE$
**
****************************************************************************/

//! [0]
bool MyScreenSaver::save( int level )
{
    switch ( level ) {
        case 0:
            if ( dim_enabled ) {
                // dim the screen
            }
            return true;
        case 1:
            if ( screenoff_enabled ) {
                // turn off the screen
            }
            return true;
        case 2:
            if ( suspend_enabled ) {
                // suspend
            }
            return true;
        default:
            return false;
    }
}

...

int timings[4];
timings[0] = 5000;  // dim after 5 seconds
timings[1] = 10000; // light off after 15 seconds
timings[2] = 45000; // suspend after 60 seconds
timings[3] = 0;
QWSServer::setScreenSaverIntervals( timings );

// ignore the key/mouse event that turns on the screen
int blocklevel = 1;
if ( !screenoff_enabled ) {
    // screenoff is disabled, ignore the key/mouse event that wakes from suspend
    blocklevel = 2;
    if ( !suspend_enabled ) {
        // suspend is disabled, never ignore events
        blocklevel = -1;
    }
}
QWSServer::setScreenSaverBlockLevel( blocklevel );
//! [0]
