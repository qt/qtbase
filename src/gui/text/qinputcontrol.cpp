/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qinputcontrol_p.h"
#include <QtGui/qevent.h>

QT_BEGIN_NAMESPACE

QInputControl::QInputControl(Type type, QObject *parent)
    : QObject(parent)
    , m_type(type)
{
}

QInputControl::QInputControl(Type type, QObjectPrivate &dd, QObject *parent)
    : QObject(dd, parent)
    , m_type(type)
{
}

bool QInputControl::isAcceptableInput(const QKeyEvent *event) const
{
    const QString text = event->text();
    if (text.isEmpty())
        return false;

    const QChar c = text.at(0);

    // Formatting characters such as ZWNJ, ZWJ, RLM, etc. This needs to go before the
    // next test, since CTRL+SHIFT is sometimes used to input it on Windows.
    if (c.category() == QChar::Other_Format)
        return true;

    // QTBUG-35734: ignore Ctrl/Ctrl+Shift; accept only AltGr (Alt+Ctrl) on German keyboards
    if (event->modifiers() == Qt::ControlModifier
            || event->modifiers() == (Qt::ShiftModifier | Qt::ControlModifier)) {
        return false;
    }

    if (c.isPrint())
        return true;

    if (c.category() == QChar::Other_PrivateUse)
        return true;

    if (m_type == TextEdit && c == QLatin1Char('\t'))
        return true;

    return false;
}

bool QInputControl::isCommonTextEditShortcut(const QKeyEvent *ke)
{
    if (ke->modifiers() == Qt::NoModifier
        || ke->modifiers() == Qt::ShiftModifier
        || ke->modifiers() == Qt::KeypadModifier) {
        if (ke->key() < Qt::Key_Escape) {
            return true;
        } else {
            switch (ke->key()) {
                case Qt::Key_Return:
                case Qt::Key_Enter:
                case Qt::Key_Delete:
                case Qt::Key_Home:
                case Qt::Key_End:
                case Qt::Key_Backspace:
                case Qt::Key_Left:
                case Qt::Key_Right:
                case Qt::Key_Up:
                case Qt::Key_Down:
                case Qt::Key_Tab:
                return true;
            default:
                break;
            }
        }
#if QT_CONFIG(shortcut)
    } else if (ke->matches(QKeySequence::Copy)
               || ke->matches(QKeySequence::Paste)
               || ke->matches(QKeySequence::Cut)
               || ke->matches(QKeySequence::Redo)
               || ke->matches(QKeySequence::Undo)
               || ke->matches(QKeySequence::MoveToNextWord)
               || ke->matches(QKeySequence::MoveToPreviousWord)
               || ke->matches(QKeySequence::MoveToStartOfDocument)
               || ke->matches(QKeySequence::MoveToEndOfDocument)
               || ke->matches(QKeySequence::SelectNextWord)
               || ke->matches(QKeySequence::SelectPreviousWord)
               || ke->matches(QKeySequence::SelectStartOfLine)
               || ke->matches(QKeySequence::SelectEndOfLine)
               || ke->matches(QKeySequence::SelectStartOfBlock)
               || ke->matches(QKeySequence::SelectEndOfBlock)
               || ke->matches(QKeySequence::SelectStartOfDocument)
               || ke->matches(QKeySequence::SelectEndOfDocument)
               || ke->matches(QKeySequence::SelectAll)
              ) {
        return true;
#endif
    }
    return false;
}

QT_END_NAMESPACE
