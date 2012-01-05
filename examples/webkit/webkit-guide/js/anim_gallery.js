/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
var app = new Function();

app.init = function() {
    var divs = document.querySelectorAll('section > div');
    if ( divs.length < 2 ) return false;
    for (var i = 0, l = divs.length ; i < l ; i++ ) {
        if (i > 1) divs[i].className = 'hideR';
        divs[i].addEventListener('click', app.navigate );
    }
    divs[0].className = 'selected';
    divs[1].className = 'queueR';
};

app.navigate = function(event) {
    var el, n1, n2, p1, p2;
    el = event.currentTarget;
    n1 = el.nextSibling;
    if (n1) n2 = el.nextSibling.nextSibling;
    p1 = el.previousSibling;
    if (p1) p2 = el.previousSibling.previousSibling;
    if ( el.className == 'selected' ) {
        if ( el.id == 'reveal') {
            el.id = '';
        }
        else {
            el.id = 'reveal';
        }
        return false;
    }
    if (n1) { n1.className = 'queueR'; n1.id = ''}
    if (n2) { n2.className = 'hideR'; n2.id = '' }
    if (p1) { p1.className = 'queueL'; p1.id = '' }
    if (p2) { p2.className = 'hideL'; p2.id = '' }
    el.className = 'selected';
};

window.onload = function() {
    // alert(app.init);
     app.init();
};
