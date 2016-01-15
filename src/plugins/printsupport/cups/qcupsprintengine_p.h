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

#ifndef QCUPSPRINTENGINE_P_H
#define QCUPSPRINTENGINE_P_H

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

#include "QtPrintSupport/qprintengine.h"

#include <QtCore/qstring.h>
#include <QtGui/qpaintengine.h>

#include <private/qpaintengine_p.h>
#include <private/qprintdevice_p.h>
#include <private/qprintengine_pdf_p.h>

QT_BEGIN_NAMESPACE

class QCupsPrintEnginePrivate;

class QCupsPrintEngine : public QPdfPrintEngine
{
    Q_DECLARE_PRIVATE(QCupsPrintEngine)
public:
    QCupsPrintEngine(QPrinter::PrinterMode m);
    virtual ~QCupsPrintEngine();

    // reimplementations QPdfPrintEngine
    void setProperty(PrintEnginePropertyKey key, const QVariant &value) Q_DECL_OVERRIDE;
    QVariant property(PrintEnginePropertyKey key) const Q_DECL_OVERRIDE;
    // end reimplementations QPdfPrintEngine

private:
    Q_DISABLE_COPY(QCupsPrintEngine)
};

class QCupsPrintEnginePrivate : public QPdfPrintEnginePrivate
{
    Q_DECLARE_PUBLIC(QCupsPrintEngine)
public:
    QCupsPrintEnginePrivate(QPrinter::PrinterMode m);
    ~QCupsPrintEnginePrivate();

    bool openPrintDevice() Q_DECL_OVERRIDE;
    void closePrintDevice() Q_DECL_OVERRIDE;

private:
    Q_DISABLE_COPY(QCupsPrintEnginePrivate)

    void setupDefaultPrinter();
    void changePrinter(const QString &newPrinter);
    void setPageSize(const QPageSize &pageSize);

    QPrintDevice m_printDevice;
    QStringList cupsOptions;
    QString cupsTempFile;
};

QT_END_NAMESPACE

#endif // QCUPSPRINTENGINE_P_H
