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
    QCupsPrintEngine(QPrinter::PrinterMode m, const QString &deviceId);
    virtual ~QCupsPrintEngine();

    // reimplementations QPdfPrintEngine
    void setProperty(PrintEnginePropertyKey key, const QVariant &value) override;
    QVariant property(PrintEnginePropertyKey key) const override;
    // end reimplementations QPdfPrintEngine

private:
    Q_DISABLE_COPY_MOVE(QCupsPrintEngine)
};

class QCupsPrintEnginePrivate : public QPdfPrintEnginePrivate
{
    Q_DECLARE_PUBLIC(QCupsPrintEngine)
public:
    QCupsPrintEnginePrivate(QPrinter::PrinterMode m);
    ~QCupsPrintEnginePrivate();

    bool openPrintDevice() override;
    void closePrintDevice() override;

private:
    Q_DISABLE_COPY_MOVE(QCupsPrintEnginePrivate)

    void changePrinter(const QString &newPrinter);
    void setPageSize(const QPageSize &pageSize);

    QPrintDevice m_printDevice;
    QStringList cupsOptions;
    QString cupsTempFile;
    QPrint::DuplexMode duplex;
};

QT_END_NAMESPACE

#endif // QCUPSPRINTENGINE_P_H
