/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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

#include <QtGui>
#include <iostream>
using namespace std;

class Widget : public QWidget
{
public:
    Widget(QWidget *parent = 0);
};

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
//! [0a]
    QStringList fonts = { "Arial", "Helvetica", "Times" };
//! [0a]

//! [0b]
    fonts << "Courier" << "Verdana";
//! [0b]

//! [1]
    for (int i = 0; i < fonts.size(); ++i)
         cout << fonts.at(i).toLocal8Bit().constData() << endl;
//! [1]

//! [2]
    QStringListIterator javaStyleIterator(fonts);
    while (javaStyleIterator.hasNext())
         cout << javaStyleIterator.next().toLocal8Bit().constData() << endl;
//! [2]

//! [3]
    QStringList::const_iterator constIterator;
    for (constIterator = fonts.constBegin(); constIterator != fonts.constEnd();
           ++constIterator)
        cout << (*constIterator).toLocal8Bit().constData() << endl;
//! [3]

//! [4]
    QString str = fonts.join(", ");
     // str == "Arial, Helvetica, Times, Courier"
//! [4]

//! [5] //! [6]
    QStringList list;
//! [5]
    list = str.split(',');
     // list: ["Arial", "Helvetica", "Times", "Courier"]
//! [6]

//! [7]
    QStringList monospacedFonts = fonts.filter(QRegularExpression("Courier|Fixed"));
//! [7]

//! [8]
    QStringList files;
    files << "$QTDIR/src/moc/moc.y"
          << "$QTDIR/src/moc/moc.l"
          << "$QTDIR/include/qconfig.h";

    files.replaceInStrings("$QTDIR", "/usr/lib/qt");
    // files: [ "/usr/lib/qt/src/moc/moc.y", ...]
//! [8]

    QString str1, str2, str3;
//! [9]
    QStringList longerList = (QStringList() << str1 << str2 << str3);
//! [9]

    list.clear();
//! [10]
    list << "Bill Murray" << "John Doe" << "Bill Clinton";

//! [11]
    QStringList result;
//! [11]
    result = list.filter("Bill");
    // result: ["Bill Murray", "Bill Clinton"]
//! [10]

    result.clear();
//! [12]
    foreach (const QString &str, list) {
        if (str.contains("Bill"))
            result += str;
    }
//! [12]

    list.clear();
//! [13]
    list << "alpha" << "beta" << "gamma" << "epsilon";
    list.replaceInStrings("a", "o");
    // list == ["olpho", "beto", "gommo", "epsilon"]
//! [13]

    list.clear();
//! [14]
    list << "alpha" << "beta" << "gamma" << "epsilon";
    list.replaceInStrings(QRegExp("^a"), "o");
    // list == ["olpha", "beta", "gamma", "epsilon"]
//! [14]

    list.clear();
//! [15]
    list << "Bill Clinton" << "Murray, Bill";
    list.replaceInStrings(QRegExp("^(.*), (.*)$"), "\\2 \\1");
    // list == ["Bill Clinton", "Bill Murray"]
//! [15]

    list.clear();
//! [16]
    list << "alpha" << "beta" << "gamma" << "epsilon";
    list.replaceInStrings(QRegularExpression("^a"), "o");
    // list == ["olpha", "beta", "gamma", "epsilon"]
//! [16]

    list.clear();
//! [17]
    list << "Bill Clinton" << "Murray, Bill";
    list.replaceInStrings(QRegularExpression("^(.*), (.*)$"), "\\2 \\1");
    // list == ["Bill Clinton", "Bill Murray"]
//! [17]

}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Widget widget;
    widget.show();
    return app.exec();
}
