// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSPDFPRINTENGINE_H_
#define QOHOSPDFPRINTENGINE_H_

#include <QScopedPointer>
#include <QTemporaryDir>
#include <qprintengine_pdf_p.h>

#if __has_include(<BasicServicesKit/ohprint.h>)
#include <BasicServicesKit/ohprint.h>
#else
#include <ohprint/print_base.h>
#endif

QT_BEGIN_NAMESPACE

class QOhosPdfPrintEnginePrivate;

class QOhosPdfPrintEngine : public QPdfPrintEngine
{
    Q_DISABLE_COPY(QOhosPdfPrintEngine)
    Q_DECLARE_PRIVATE(QOhosPdfPrintEngine)

public:
    QOhosPdfPrintEngine(QPrinter::PrinterMode mode, const QString &deviceId);
    ~QOhosPdfPrintEngine() override;

    bool begin(QPaintDevice *pdev) override;
    bool end() override;

    void setProperty(PrintEnginePropertyKey key, const QVariant &value) override;
    QVariant property(PrintEnginePropertyKey key) const override;

private:
    Print_DuplexMode nativeDuplexMode() const;
    Print_ColorMode nativeColorMode() const;
    Print_OrientationMode nativeOrientationMode() const;

    void updateUnsupportedPrinterParameters();

    QString m_deviceId;
    QPrinter::DuplexMode m_duplexMode = QPrinter::DuplexNone;
    int m_copies = 1;

    static QTemporaryDir s_tempDir;
};

class QOhosPdfPrintEnginePrivate : public QPdfPrintEnginePrivate
{
    Q_DISABLE_COPY(QOhosPdfPrintEnginePrivate)
    Q_DECLARE_PUBLIC(QOhosPdfPrintEngine)

public:
    QOhosPdfPrintEnginePrivate(QPrinter::PrinterMode m);
    ~QOhosPdfPrintEnginePrivate();
};

QT_END_NAMESPACE

#endif // QOHOSPDFPRINTENGINE_H_
