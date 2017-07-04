/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Rick Stockton <rickstockton@reno-computerhelp.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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

#include "buttontester.h"

#include <QDebug>

void ButtonTester::mousePressEvent(QMouseEvent *e)
{
    int j = ButtonTester::buttonByNumber (e->button());
    QString result = "Mouse Press: raw button=" + QString::number(j)
                + "  Qt=" + enumNameFromValue(e->button());
    QString buttonsString = ButtonTester::enumNamesFromMouseButtons(e->buttons());
    result += "\n heldbuttons " + buttonsString;
    qDebug() << result;
    this->setText(result);
    this->repaint();
}

void ButtonTester::mouseReleaseEvent(QMouseEvent *e)
{
    int j = ButtonTester::buttonByNumber (e->button());
    QString result = "Mouse Release: raw button=" + QString::number(j)
                + "  Qt=" + enumNameFromValue(e->button());
    QString buttonsString = ButtonTester::enumNamesFromMouseButtons(e->buttons());
    result += "\n heldbuttons " + buttonsString;
    qDebug() << result;
    this->setText(result);
    this->repaint();

}

void ButtonTester::mouseDoubleClickEvent(QMouseEvent *e)
{
    int j = ButtonTester::buttonByNumber (e->button());
    QString result = "Mouse DoubleClick: raw button=" + QString::number(j)
                + "  Qt=" + enumNameFromValue(e->button());
    QString buttonsString = ButtonTester::enumNamesFromMouseButtons(e->buttons());
    result += "\n heldbuttons" + buttonsString;
    qDebug() << result;
    this->setText(result);
}

#if QT_CONFIG(wheelevent)
void ButtonTester::wheelEvent (QWheelEvent *e)
{
    QString result;
    if (e->delta() > 0) {

        if (e->orientation() == Qt::Vertical) {
            result = "Mouse Wheel Event: UP";
        } else {
            result = "Mouse Wheel Event: LEFT";
        }
    } else if (e->delta() < 0) {
        if (e->orientation() == Qt::Vertical) {
            result = "Mouse Wheel Event: DOWN";
        } else {
            result = "Mouse Wheel Event: RIGHT";
        }
    }
    qDebug() << result;
    this->setText(result);
}
#endif

int ButtonTester::buttonByNumber(const Qt::MouseButton button)
{
    if (button == Qt::NoButton)      return 0;
    if (button == Qt::LeftButton)    return 1;
    if (button == Qt::RightButton)   return 2;
    if (button == Qt::MiddleButton)  return 3;

/* Please note that Qt Button #4 corresponds to button #8 on all
 * platforms which EMULATE wheel events by creating button events
 * (Button #4 = Scroll Up; Button #5 = Scroll Down; Button #6 = Scroll
 * Left; and Button #7 = Scroll Right.) This includes X11, with both
 * Xlib and XCB.  So, the "raw button" for "Qt::BackButton" is
 * usually described as "Button #8".

 * If your platform supports "smooth scrolling", then, for the cases of
 * Qt::BackButton and higher, this program will show "raw button" with a
 * value which is too large. Subtract 4 to get the correct button ID for
 * your platform.
 */

    if (button == Qt::BackButton)    return 8;
    if (button == Qt::ForwardButton) return 9;
    if (button == Qt::TaskButton)    return 10;
    if (button == Qt::ExtraButton4)  return 11;
    if (button == Qt::ExtraButton5)  return 12;
    if (button == Qt::ExtraButton6)  return 13;
    if (button == Qt::ExtraButton7)  return 14;
    if (button == Qt::ExtraButton8)  return 15;
    if (button == Qt::ExtraButton9)  return 16;
    if (button == Qt::ExtraButton10) return 17;
    if (button == Qt::ExtraButton11) return 18;
    if (button == Qt::ExtraButton12) return 19;
    if (button == Qt::ExtraButton13) return 20;
    if (button == Qt::ExtraButton14) return 21;
    if (button == Qt::ExtraButton15) return 22;
    if (button == Qt::ExtraButton16) return 23;
    if (button == Qt::ExtraButton17) return 24;
    if (button == Qt::ExtraButton18) return 25;
    if (button == Qt::ExtraButton19) return 26;
    if (button == Qt::ExtraButton20) return 27;
    if (button == Qt::ExtraButton21) return 28;
    if (button == Qt::ExtraButton22) return 29;
    if (button == Qt::ExtraButton23) return 30;
    if (button == Qt::ExtraButton24) return 31;
    qDebug("QMouseShortcutEntry::addShortcut contained Invalid Qt::MouseButton value");
    return 0;
}

QString ButtonTester::enumNameFromValue(const Qt::MouseButton button)
{
    if (button == Qt::NoButton)      return "NoButton";
    if (button == Qt::LeftButton)    return "LeftButton";
    if (button == Qt::RightButton)   return "RightButton";
    if (button == Qt::MiddleButton)  return "MiddleButton";
    if (button == Qt::BackButton)    return "BackButton";
    if (button == Qt::ForwardButton) return "ForwardButton";
    if (button == Qt::TaskButton)    return "TaskButton";
    if (button == Qt::ExtraButton4)  return "ExtraButton4";
    if (button == Qt::ExtraButton5)  return "ExtraButton5";
    if (button == Qt::ExtraButton6)  return "ExtraButton6";
    if (button == Qt::ExtraButton7)  return "ExtraButton7";
    if (button == Qt::ExtraButton8)  return "ExtraButton8";
    if (button == Qt::ExtraButton9)  return "ExtraButton9";
    if (button == Qt::ExtraButton10) return "ExtraButton10";
    if (button == Qt::ExtraButton11) return "ExtraButton11";
    if (button == Qt::ExtraButton12) return "ExtraButton12";
    if (button == Qt::ExtraButton13) return "ExtraButton13";
    if (button == Qt::ExtraButton14) return "ExtraButton14";
    if (button == Qt::ExtraButton15) return "ExtraButton15";
    if (button == Qt::ExtraButton16) return "ExtraButton16";
    if (button == Qt::ExtraButton17) return "ExtraButton17";
    if (button == Qt::ExtraButton18) return "ExtraButton18";
    if (button == Qt::ExtraButton19) return "ExtraButton19";
    if (button == Qt::ExtraButton20) return "ExtraButton20";
    if (button == Qt::ExtraButton21) return "ExtraButton21";
    if (button == Qt::ExtraButton22) return "ExtraButton22";
    if (button == Qt::ExtraButton23) return "ExtraButton23";
    if (button == Qt::ExtraButton24) return "ExtraButton24";
    qDebug("QMouseShortcutEntry::addShortcut contained Invalid Qt::MouseButton value");
    return "NoButton";
}

QString ButtonTester::enumNamesFromMouseButtons(const Qt::MouseButtons buttons)
{
    QString returnText = "";
    if (buttons == Qt::NoButton)      return "NoButton";
    if (buttons & Qt::LeftButton)    returnText += "LeftButton ";
    if (buttons & Qt::RightButton)   returnText +=  "RightButton ";
    if (buttons & Qt::MiddleButton)  returnText +=  "MiddleButton ";
    if (buttons & Qt::BackButton)    returnText +=  "BackButton ";
    if (buttons & Qt::ForwardButton) returnText +=  "ForwardButton ";
    if (buttons & Qt::TaskButton)    returnText +=  "TaskButton ";
    if (buttons & Qt::ExtraButton4)  returnText +=  "ExtraButton4 ";
    if (buttons & Qt::ExtraButton5)  returnText +=  "ExtraButton5 ";
    if (buttons & Qt::ExtraButton6)  returnText +=  "ExtraButton6 ";
    if (buttons & Qt::ExtraButton7)  returnText +=  "ExtraButton7 ";
    if (buttons & Qt::ExtraButton8)  returnText +=  "ExtraButton8 ";
    if (buttons & Qt::ExtraButton9)  returnText +=  "ExtraButton9 ";
    if (buttons & Qt::ExtraButton10) returnText +=  "ExtraButton10 ";
    if (buttons & Qt::ExtraButton11) returnText +=  "ExtraButton11 ";
    if (buttons & Qt::ExtraButton12) returnText +=  "ExtraButton12 ";
    if (buttons & Qt::ExtraButton13) returnText +=  "ExtraButton13 ";
    if (buttons & Qt::ExtraButton14) returnText +=  "ExtraButton14 ";
    if (buttons & Qt::ExtraButton15) returnText +=  "ExtraButton15 ";
    if (buttons & Qt::ExtraButton16) returnText +=  "ExtraButton16 ";
    if (buttons & Qt::ExtraButton17) returnText +=  "ExtraButton17 ";
    if (buttons & Qt::ExtraButton18) returnText +=  "ExtraButton18 ";
    if (buttons & Qt::ExtraButton19) returnText +=  "ExtraButton19 ";
    if (buttons & Qt::ExtraButton20) returnText +=  "ExtraButton20 ";
    if (buttons & Qt::ExtraButton21) returnText +=  "ExtraButton21 ";
    if (buttons & Qt::ExtraButton22) returnText +=  "ExtraButton22 ";
    if (buttons & Qt::ExtraButton23) returnText +=  "ExtraButton23 ";
    if (buttons & Qt::ExtraButton24) returnText +=  "ExtraButton24 ";
    return returnText;
}

