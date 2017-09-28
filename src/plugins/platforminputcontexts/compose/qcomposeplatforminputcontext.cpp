/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qcomposeplatforminputcontext.h"

#include <QtCore/QCoreApplication>
#include <QtGui/QKeyEvent>
#include <QtCore/QDebug>

#include <algorithm>

QT_BEGIN_NAMESPACE

//#define DEBUG_COMPOSING

static const int ignoreKeys[] = {
    Qt::Key_Shift,
    Qt::Key_Control,
    Qt::Key_Meta,
    Qt::Key_Alt,
    Qt::Key_CapsLock,
    Qt::Key_Super_L,
    Qt::Key_Super_R,
    Qt::Key_Hyper_L,
    Qt::Key_Hyper_R,
    Qt::Key_Mode_switch
};

static const int composingKeys[] = {
    Qt::Key_Multi_key,
    Qt::Key_Dead_Grave,
    Qt::Key_Dead_Acute,
    Qt::Key_Dead_Circumflex,
    Qt::Key_Dead_Tilde,
    Qt::Key_Dead_Macron,
    Qt::Key_Dead_Breve,
    Qt::Key_Dead_Abovedot,
    Qt::Key_Dead_Diaeresis,
    Qt::Key_Dead_Abovering,
    Qt::Key_Dead_Doubleacute,
    Qt::Key_Dead_Caron,
    Qt::Key_Dead_Cedilla,
    Qt::Key_Dead_Ogonek,
    Qt::Key_Dead_Iota,
    Qt::Key_Dead_Voiced_Sound,
    Qt::Key_Dead_Semivoiced_Sound,
    Qt::Key_Dead_Belowdot,
    Qt::Key_Dead_Hook,
    Qt::Key_Dead_Horn,
    Qt::Key_Dead_Stroke,
    Qt::Key_Dead_Abovecomma,
    Qt::Key_Dead_Abovereversedcomma,
    Qt::Key_Dead_Doublegrave,
    Qt::Key_Dead_Belowring,
    Qt::Key_Dead_Belowmacron,
    Qt::Key_Dead_Belowcircumflex,
    Qt::Key_Dead_Belowtilde,
    Qt::Key_Dead_Belowbreve,
    Qt::Key_Dead_Belowdiaeresis,
    Qt::Key_Dead_Invertedbreve,
    Qt::Key_Dead_Belowcomma,
    Qt::Key_Dead_Currency,
    Qt::Key_Dead_a,
    Qt::Key_Dead_A,
    Qt::Key_Dead_e,
    Qt::Key_Dead_E,
    Qt::Key_Dead_i,
    Qt::Key_Dead_I,
    Qt::Key_Dead_o,
    Qt::Key_Dead_O,
    Qt::Key_Dead_u,
    Qt::Key_Dead_U,
    Qt::Key_Dead_Small_Schwa,
    Qt::Key_Dead_Capital_Schwa,
    Qt::Key_Dead_Greek,
    Qt::Key_Dead_Lowline,
    Qt::Key_Dead_Aboveverticalline,
    Qt::Key_Dead_Belowverticalline,
    Qt::Key_Dead_Longsolidusoverlay
};

QComposeInputContext::QComposeInputContext()
    : m_tableState(TableGenerator::EmptyTable)
    , m_compositionTableInitialized(false)
{
    clearComposeBuffer();
}

bool QComposeInputContext::filterEvent(const QEvent *event)
{
    const QKeyEvent *keyEvent = (const QKeyEvent *)event;
    // should pass only the key presses
    if (keyEvent->type() != QEvent::KeyPress) {
        return false;
    }

    // if there were errors when generating the compose table input
    // context should not try to filter anything, simply return false
    if (m_compositionTableInitialized && (m_tableState & TableGenerator::NoErrors) != TableGenerator::NoErrors)
        return false;

    int keyval = keyEvent->key();
    int keysym = 0;

    if (ignoreKey(keyval))
        return false;

    if (!composeKey(keyval) && keyEvent->text().isEmpty())
        return false;

    keysym = keyEvent->nativeVirtualKey();

    int nCompose = 0;
    while (nCompose < QT_KEYSEQUENCE_MAX_LEN && m_composeBuffer[nCompose] != 0)
        nCompose++;

    if (nCompose == QT_KEYSEQUENCE_MAX_LEN) {
        reset();
        nCompose = 0;
    }

    m_composeBuffer[nCompose] = keysym;
    // check sequence
    if (checkComposeTable())
        return true;

    return false;
}

bool QComposeInputContext::isValid() const
{
    return true;
}

void QComposeInputContext::setFocusObject(QObject *object)
{
    m_focusObject = object;
}

void QComposeInputContext::reset()
{
    clearComposeBuffer();
}

void QComposeInputContext::update(Qt::InputMethodQueries q)
{
    QPlatformInputContext::update(q);
}

static bool isDuplicate(const QComposeTableElement &lhs, const QComposeTableElement &rhs)
{
    return std::equal(lhs.keys, lhs.keys + QT_KEYSEQUENCE_MAX_LEN,
                      QT_MAKE_CHECKED_ARRAY_ITERATOR(rhs.keys, QT_KEYSEQUENCE_MAX_LEN));
}

bool QComposeInputContext::checkComposeTable()
{
    if (!m_compositionTableInitialized) {
        TableGenerator reader;
        m_tableState = reader.tableState();

        m_compositionTableInitialized = true;
        if ((m_tableState & TableGenerator::NoErrors) == TableGenerator::NoErrors) {
            m_composeTable = reader.composeTable();
        } else {
#ifdef DEBUG_COMPOSING
            qDebug( "### FAILED_PARSING ###" );
#endif
            // if we have errors, don' try to look things up anyways.
            reset();
            return false;
        }
    }
    Q_ASSERT(!m_composeTable.isEmpty());
    QVector<QComposeTableElement>::const_iterator it =
            std::lower_bound(m_composeTable.constBegin(), m_composeTable.constEnd(), m_composeBuffer, ByKeys());

    // prevent dereferencing an 'end' iterator, which would result in a crash
    if (it == m_composeTable.constEnd())
        it -= 1;

    QComposeTableElement elem = *it;
    // would be nicer if qLowerBound had API that tells if the item was actually found
    if (m_composeBuffer[0] != elem.keys[0]) {
#ifdef DEBUG_COMPOSING
        qDebug( "### no match ###" );
#endif
        reset();
        return false;
    }
    // check if compose buffer is matched
    for (int i=0; i < QT_KEYSEQUENCE_MAX_LEN; i++) {

        // check if partial match
        if (m_composeBuffer[i] == 0 && elem.keys[i]) {
#ifdef DEBUG_COMPOSING
            qDebug("### partial match ###");
#endif
            return true;
        }

        if (m_composeBuffer[i] != elem.keys[i]) {
#ifdef DEBUG_COMPOSING
            qDebug("### different entry ###");
#endif
            reset();
            return i != 0;
        }
    }
#ifdef DEBUG_COMPOSING
    qDebug("### match exactly ###");
#endif

    // check if the key sequence is overwriten - see the comment in
    // TableGenerator::orderComposeTable()
    int next = 1;
    do {
        // if we are at the end of the table, then we have nothing to do here
        if (it + next != m_composeTable.constEnd()) {
            QComposeTableElement nextElem = *(it + next);
            if (isDuplicate(elem, nextElem)) {
                elem = nextElem;
                next++;
                continue;
            } else {
                break;
            }
        }
        break;
    } while (true);

    commitText(elem.value);
    reset();

    return true;
}

void QComposeInputContext::commitText(uint character) const
{
    QInputMethodEvent event;
    event.setCommitString(QChar(character));
    QCoreApplication::sendEvent(m_focusObject, &event);
}

bool QComposeInputContext::ignoreKey(int keyval) const
{
    for (uint i = 0; i < (sizeof(ignoreKeys) / sizeof(ignoreKeys[0])); i++)
        if (keyval == ignoreKeys[i])
            return true;

    return false;
}

bool QComposeInputContext::composeKey(int keyval) const
{
    for (uint i = 0; i < (sizeof(composingKeys) / sizeof(composingKeys[0])); i++)
        if (keyval == composingKeys[i])
            return true;

    return false;
}

void QComposeInputContext::clearComposeBuffer()
{
    for (uint i=0; i < (sizeof(m_composeBuffer) / sizeof(int)); i++)
        m_composeBuffer[i] = 0;
}

QComposeInputContext::~QComposeInputContext() {}

QT_END_NAMESPACE
