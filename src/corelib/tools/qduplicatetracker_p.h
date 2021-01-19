/****************************************************************************
**
** Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/
#ifndef QDUPLICATETRACKER_P_H
#define QDUPLICATETRACKER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qglobal.h>

#if QT_HAS_INCLUDE(<memory_resource>) && __cplusplus > 201402L
#  include <unordered_set>
#  include <memory_resource>
#else
#  include <qset.h>
#endif

QT_BEGIN_NAMESPACE

template <typename T, size_t Prealloc = 32>
class QDuplicateTracker {
#ifdef __cpp_lib_memory_resource
    char buffer[Prealloc * sizeof(T)];
    std::pmr::monotonic_buffer_resource res{buffer, sizeof buffer};
    std::pmr::unordered_set<T> set{&res};
#else
    QSet<T> set;
    int setSize = 0;
#endif
    Q_DISABLE_COPY_MOVE(QDuplicateTracker);
public:
    QDuplicateTracker() = default;
    void reserve(int n) { set.reserve(n); }
    Q_REQUIRED_RESULT bool hasSeen(const T &s)
    {
        bool inserted;
#ifdef __cpp_lib_memory_resource
        inserted = set.insert(s).second;
#else
        set.insert(s);
        const int n = set.size();
        inserted = qExchange(setSize, n) != n;
#endif
        return !inserted;
    }
};

QT_END_NAMESPACE

#endif /* QDUPLICATETRACKER_P_H */
