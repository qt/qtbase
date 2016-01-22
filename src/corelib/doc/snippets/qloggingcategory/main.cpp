/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QCoreApplication>
#include <QLoggingCategory>

//![1]
// in a header
Q_DECLARE_LOGGING_CATEGORY(driverUsb)

// in one source file
Q_LOGGING_CATEGORY(driverUsb, "driver.usb")
//![1]

//![5]
Q_LOGGING_CATEGORY(driverUsbEvents, "driver.usb.events", QtWarningMsg)
//![5]

// Completely made up example, inspired by en.wikipedia.org/wiki/USB :)
struct UsbEntry {
    int id;
    int classtype;
};

QDebug operator<<(QDebug &debug, const UsbEntry &entry)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "" << entry.id << " (" << entry.classtype << ')';
    return debug;
}

QList<UsbEntry> usbEntries() {
    QList<UsbEntry> entries;
    return entries;
}

//![20]
void myCategoryFilter(QLoggingCategory *);
//![20]

//![21]
QLoggingCategory::CategoryFilter oldCategoryFilter;

void myCategoryFilter(QLoggingCategory *category)
{
    // configure driver.usb category here, otherwise forward to to default filter.
    if (qstrcmp(category->categoryName(), "driver.usb") == 0)
        category->setEnabled(QtDebugMsg, true);
    else
        oldCategoryFilter(category);
}
//![21]

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

//![2]
    QLoggingCategory::setFilterRules(QStringLiteral("driver.usb.debug=true"));
//![2]

//![22]

// ...
oldCategoryFilter = QLoggingCategory::installFilter(myCategoryFilter);
//![22]

//![3]
    qSetMessagePattern("%{category} %{message}");
//![3]

//![4]
    // usbEntries() will only be called if driverUsb category is enabled
    qCDebug(driverUsb) << "devices: " << usbEntries();
//![4]

    {
//![10]
    QLoggingCategory category("driver.usb");
    qCDebug(category) << "a debug message";
//![10]
    }

//![qcinfo_stream]
    QLoggingCategory category("driver.usb");
    qCInfo(category) << "an informational message";
//![qcinfo_stream]

    {
//![11]
    QLoggingCategory category("driver.usb");
    qCWarning(category) << "a warning message";
//![11]
    }

    {
//![12]
    QLoggingCategory category("driver.usb");
    qCCritical(category) << "a critical message";
//![12]
    }

    {
//![13]
    QLoggingCategory category("driver.usb");
    qCDebug(category, "a debug message logged into category %s", category.categoryName());
//![13]
    }

    {
//![qcinfo_printf]
    QLoggingCategory category("driver.usb");
    qCInfo(category, "an informational message logged into category %s", category.categoryName());
//![qcinfo_printf]
    }

    {
//![14]
    QLoggingCategory category("driver.usb");
    qCWarning(category, "a warning message logged into category %s", category.categoryName());
//![14]
    }

    {
//![15]
    QLoggingCategory category("driver.usb");
    qCCritical(category, "a critical message logged into category %s", category.categoryName());
//![15]
    }

    return 0;
}

