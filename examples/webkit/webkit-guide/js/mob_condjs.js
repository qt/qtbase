/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt WebKit module of the Qt Toolkit.
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
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
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
window.onload = function() {
    var msg = 'JavaScript thinks you are viewing this page with a ';
    if ( isDesign('desktop') ) {
        msg += 'full desktop browser';
    }
    else if ( isDesign('touch') ) {
        msg += 'touch-based mobile browser';
    }
    else if ( isDesign('mobile') ) {
        msg += 'non-touch mobile browser';
    }
    else {
        msg = window.styleMedia.matchMedium;
    }
    document.getElementById('js').innerHTML = msg;
};

function isDesign(str) {
    var design;
    if (matchesMedia('only screen and (min-device-width: 481px)')) {
        design = 'desktop';
    }
    else if (matchesMedia('only screen and (max-device-width: 480px)')) {
        design = 'touch';
    }
    else if (matchesMedia('handheld')) {
        design = 'mobile';
    }
    return str == design;
}

function matchesMedia(query) {
    if (!!window.matchMedia)
        return window.matchMedia(query).matches;
    if (!!window.styleMedia && !!window.styleMedia.matchMedium)
        return window.styleMedia.matchMedium(query);
    if (!!window.media && window.media.matchMedium)
        return window.media.matchMedium(query);
    return false;
}
