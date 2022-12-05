// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
static QLoggingCategory::CategoryFilter oldCategoryFilter = nullptr;

void myCategoryFilter(QLoggingCategory *category)
{
    // For a category set up after this filter is installed, we first set it up
    // with the old filter. This ensures that any driver.usb logging configured
    // by the user is kept, aside from the one level we override; and any new
    // categories we're not interested in get configured by the old filter.
    if (oldCategoryFilter)
        oldCategoryFilter(category);

    // Tweak driver.usb's logging, over-riding the default filter:
    if (qstrcmp(category->categoryName(), "driver.usb") == 0)
        category->setEnabled(QtDebugMsg, true);
}
//![21]

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

//![2]
    QLoggingCategory::setFilterRules(QStringLiteral("driver.usb.debug=true"));
//![2]

//![22]
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

    {
//![16]
    QLoggingCategory category("driver.usb");
    qCFatal(category) << "a fatal message. Program will be terminated!";
//![16]
    }

    {
//![17]
    QLoggingCategory category("driver.usb");
    qCFatal(category, "a fatal message. Program will be terminated!");
//![17]
    }

    return 0;
}

