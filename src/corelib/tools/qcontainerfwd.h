/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <QtCore/qglobal.h>

#ifndef QCONTAINERFWD_H
#define QCONTAINERFWD_H

// std headers can unfortunately not be forward declared
#include <tuple>
#include <variant>

QT_BEGIN_NAMESPACE

template <class Key, class T> class QCache;
template <class Key, class T> class QHash;
template <class Key, class T> class QMap;
template <class Key, class T> class QMultiHash;
template <class Key, class T> class QMultiMap;
template <typename T1, typename T2>
using QPair = std::pair<T1, T2>;
template <class T> class QQueue;
template <class T> class QSet;
template <class T> class QStack;
template<class T, qsizetype Prealloc = 256> class QVarLengthArray;
template <class T> class QList;
#ifndef Q_CLANG_QDOC
template<typename T> using QVector = QList<T>;
using QStringList = QList<QString>;
using QByteArrayList = QList<QByteArray>;
#else
template<typename T> class QVector;
class QStringList;
class QByteArrayList;
#endif
class QMetaType;
class QVariant;

QT_END_NAMESPACE

#endif // QCONTAINERFWD_H
