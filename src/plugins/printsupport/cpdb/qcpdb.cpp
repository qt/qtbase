// Copyright (C) 2022-2023 Gaurav Guleria <tinytrebuchet@protonmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcpdb_p.h"

QT_BEGIN_NAMESPACE

const char *QCPDBSupport::translateOption(cpdb_printer_obj_t * printerObj, const char *optionName)
{

    QByteArray locale = QLocale().bcp47Name().toLocal8Bit().replace('-', '_');
    return cpdbGetOptionTranslation(printerObj, optionName, locale.constData());
}

const char *QCPDBSupport::translateChoice(cpdb_printer_obj_t *printerObj, const char *optionName, const char *choiceName)
{
    QByteArray locale = QLocale().bcp47Name().toLocal8Bit().replace('-', '_');
    return cpdbGetChoiceTranslation(printerObj, optionName, choiceName, locale.constData());
}

const char *QCPDBSupport::translateGroup(cpdb_printer_obj_t *printerObj, const char *groupName)
{
    QByteArray locale = QLocale().bcp47Name().toLocal8Bit().replace('-', '_');
    return cpdbGetOptionTranslation(printerObj, groupName, locale.constData());
}

QT_END_NAMESPACE
