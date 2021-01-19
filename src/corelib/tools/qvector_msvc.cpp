/****************************************************************************
**
** Copyright (C) 2016 Intel Corporation.
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

// ### Qt6: verify if we can remove this, somehow.
// First, try to see if the extern template from qvector.h is necessary.
// If it still is, check if removing the copy constructors in qarraydata.h
// make the calling convention of both sets of begin() and end() functions
// match, as it does for the IA-64 C++ ABI.

#ifdef QVECTOR_H
#  error "This file must be compiled with no precompiled headers"
#endif

// invert the setting of QT_STRICT_ITERATORS, whichever it was
#ifdef QT_STRICT_ITERATORS
#  undef QT_STRICT_ITERATORS
#else
#  define QT_STRICT_ITERATORS
#endif

// the Q_TEMPLATE_EXTERN at the bottom of qvector.h will do the trick
#include <QtCore/qvector.h>
