/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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
#ifndef QIMAGESCALE_P_H
#define QIMAGESCALE_P_H

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

#include <qimage.h>

QT_BEGIN_NAMESPACE

/*
  This version accepts only supported formats.
*/
QImage qSmoothScaleImage(const QImage &img, int w, int h);

namespace QImageScale {
    struct QImageScaleInfo {
        int *xpoints{nullptr};
        const unsigned int **ypoints{nullptr};
        int *xapoints{nullptr};
        int *yapoints{nullptr};
        int xup_yup{0};
        int sh = 0;
        int sw = 0;
    };
}

QT_END_NAMESPACE

#endif
