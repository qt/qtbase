/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QFONTENGINEMULTIFONTCONFIG_H
#define QFONTENGINEMULTIFONTCONFIG_H

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

#include <QtGui/private/qfontengine_p.h>
#include <fontconfig/fontconfig.h>

QT_BEGIN_NAMESPACE

class QFontEngineMultiFontConfig : public QFontEngineMulti
{
public:
    explicit QFontEngineMultiFontConfig(QFontEngine *fe, int script);

    ~QFontEngineMultiFontConfig();

    bool shouldLoadFontEngineForCharacter(int at, uint ucs4) const override;
private:
    FcPattern* getMatchPatternForFallback(int at) const;

    mutable QVector<FcPattern*> cachedMatchPatterns;
};

QT_END_NAMESPACE

#endif // QFONTENGINEMULTIFONTCONFIG_H
