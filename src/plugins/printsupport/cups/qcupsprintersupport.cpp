/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2014 John Layt <jlayt@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qcupsprintersupport_p.h"

#include "qcupsprintengine_p.h"
#include "qppdprintdevice.h"
#include <private/qprinterinfo_p.h>
#include <private/qprintdevice_p.h>

#include <QtPrintSupport/QPrinterInfo>

#if QT_CONFIG(dialogbuttonbox)
#include <QGuiApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#endif // QT_CONFIG(dialogbuttonbox)

#include <cups/ppd.h>
#ifndef QT_LINUXBASE // LSB merges everything into cups.h
# include <cups/language.h>
#endif

QT_BEGIN_NAMESPACE

#if QT_CONFIG(dialogbuttonbox)
static const char *getPasswordCB(const char */*prompt*/, http_t *http, const char */*method*/, const char *resource, void */*user_data*/)
{
    // cups doesn't free the const char * we return so keep around
    // the last password so we don't leak memory if called multiple times.
    static QByteArray password;

    // prompt is always "Password for %s on %s? " but we can't use it since we allow the user to change the user.
    // That is fine because cups always calls cupsUser after calling this callback.
    // We build our own prompt with the hostname (if not localhost) and the resource that is being used

    char hostname[HTTP_MAX_HOST];
    httpGetHostname(http, hostname, HTTP_MAX_HOST);

    const QString username = QString::fromLocal8Bit(cupsUser());

    QDialog dialog;
    dialog.setWindowTitle(QCoreApplication::translate("QCupsPrinterSupport", "Authentication Needed"));

    QFormLayout *layout = new QFormLayout(&dialog);
    layout->setSizeConstraint(QLayout::SetFixedSize);

    QLineEdit *usernameLE = new QLineEdit();
    usernameLE->setText(username);

    QLineEdit *passwordLE = new QLineEdit();
    passwordLE->setEchoMode(QLineEdit::Password);

    QString resourceString = QString::fromLocal8Bit(resource);
    if (resourceString.startsWith(QStringLiteral("/printers/")))
        resourceString = resourceString.mid(QStringLiteral("/printers/").length());

    QLabel *label = new QLabel();
    if (hostname == QStringLiteral("localhost")) {
        label->setText(QCoreApplication::translate("QCupsPrinterSupport", "Authentication needed to use %1.").arg(resourceString));
    } else {
        label->setText(QCoreApplication::translate("QCupsPrinterSupport", "Authentication needed to use %1 on %2.").arg(resourceString).arg(hostname));
        label->setWordWrap(true);
    }

    layout->addRow(label);
    layout->addRow(new QLabel(QCoreApplication::translate("QCupsPrinterSupport", "Username:")), usernameLE);
    layout->addRow(new QLabel(QCoreApplication::translate("QCupsPrinterSupport", "Password:")), passwordLE);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addRow(buttonBox);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    passwordLE->setFocus();

    if (dialog.exec() != QDialog::Accepted)
        return nullptr;

    if (usernameLE->text() != username)
        cupsSetUser(usernameLE->text().toLocal8Bit().constData());

    password = passwordLE->text().toLocal8Bit();

    return password.constData();
}
#endif // QT_CONFIG(dialogbuttonbox)

QCupsPrinterSupport::QCupsPrinterSupport()
    : QPlatformPrinterSupport()
{
#if QT_CONFIG(dialogbuttonbox)
    // Only show password dialog if GUI application
    if (qobject_cast<QGuiApplication*>(QCoreApplication::instance()))
        cupsSetPasswordCB2(getPasswordCB, nullptr /* user_data */ );
#endif // QT_CONFIG(dialogbuttonbox)
}

QCupsPrinterSupport::~QCupsPrinterSupport()
{
}

QPrintEngine *QCupsPrinterSupport::createNativePrintEngine(QPrinter::PrinterMode printerMode, const QString &deviceId)
{
    return new QCupsPrintEngine(printerMode, (deviceId.isEmpty() ? defaultPrintDeviceId() : deviceId));
}

QPaintEngine *QCupsPrinterSupport::createPaintEngine(QPrintEngine *engine, QPrinter::PrinterMode printerMode)
{
    Q_UNUSED(printerMode)
    return static_cast<QCupsPrintEngine *>(engine);
}

QPrintDevice QCupsPrinterSupport::createPrintDevice(const QString &id)
{
    return QPlatformPrinterSupport::createPrintDevice(new QPpdPrintDevice(id));
}

QStringList QCupsPrinterSupport::availablePrintDeviceIds() const
{
    QStringList list;
    cups_dest_t *dests;
    int count = cupsGetDests(&dests);
    list.reserve(count);
    for (int i = 0; i < count; ++i) {
        QString printerId = QString::fromLocal8Bit(dests[i].name);
        if (dests[i].instance)
            printerId += QLatin1Char('/') + QString::fromLocal8Bit(dests[i].instance);
        list.append(printerId);
    }
    cupsFreeDests(count, dests);
    return list;
}

QString QCupsPrinterSupport::defaultPrintDeviceId() const
{
    return staticDefaultPrintDeviceId();
}

QString QCupsPrinterSupport::staticDefaultPrintDeviceId()
{
    QString printerId;
    cups_dest_t *dests;
    int count = cupsGetDests(&dests);
    for (int i = 0; i < count; ++i) {
        if (dests[i].is_default) {
            printerId = QString::fromLocal8Bit(dests[i].name);
            if (dests[i].instance) {
                printerId += QLatin1Char('/') + QString::fromLocal8Bit(dests[i].instance);
                break;
            }
        }
    }
    cupsFreeDests(count, dests);
    return printerId;
}

QT_END_NAMESPACE
