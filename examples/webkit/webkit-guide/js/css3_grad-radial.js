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
window.onload = function() {
    var el = document.querySelector('#main');
    el.addEventListener('mousedown', function(event){

        var colors = [ 'beige', 'crimson', 'darkcyan', 'turquoise',
            'darkgoldenrod', 'darkorange', 'fuchsia',
            'greenyellow', 'lightblue', 'lightcoral',
            'lightgreen', 'mediumorchid', 'pink', 'plum',
            'skyblue', 'springgreen', 'tan', 'tomato',
            'violet', 'yellow', 'teal'];

        var x = event.offsetX;
        var y = event.offsetY;

        var loc = document.querySelector('#localStyles');
        var style = '#main:active {' + 'background: -webkit-gradient(radial, ';
            style += (x + ' ');
            style += (y + ' ');
        style += ',5,';
            style += ((x + 10) + ' ');
            style += ((y + 10) + ' ');
        style += ', 48, ';
            style += 'from(' + colors[r(5)] + '),';
            style += 'color-stop(20%, ' + colors[r(5)] + '),';
            style += 'color-stop(40%, ' + colors[r(5)] + '),';
            style += 'color-stop(60%, ' + colors[r(5)] + '),';
            style += 'color-stop(80%, ' + colors[r(5)] + '),';
            style += 'to(#ffffff) );'
            style += '}'
        loc.innerHTML = style;
    });
}

function r(i) {
    return Math.floor( (Math.random() * i ) );
}
