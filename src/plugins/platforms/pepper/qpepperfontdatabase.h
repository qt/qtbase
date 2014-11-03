/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Native Client platform plugin.
**
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#ifndef QPEPPERFONTDATABASE_H
#define QPEPPERFONTDATABASE_H

#include "../platformsupport/fontdatabases/basic/qbasicfontdatabase_p.h"

class QPepperFontDatabase : public QBasicFontDatabase
{
public:
    void populateFontDatabase();
    QFontEngine *fontEngine(const QFontDef &fontDef, void *handle) Q_DECL_OVERRIDE;
    QStringList fallbacksForFamily(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QChar::Script script) const Q_DECL_OVERRIDE;
    QStringList addApplicationFont(const QByteArray &fontData, const QString &fileName);
    void releaseHandle(void *handle);
};

#endif
