// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "calendarBackendInterface.h"

#include <QApplication>
#include <QCalendar>
#include <QCalendarWidget>
#include <QCommandLineParser>
#include <QDebug>
#include <QPluginLoader>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setApplicationName("JulianGregorianCalendar");
    QCommandLineParser parser;
    parser.setApplicationDescription("Calendar Backend Plugin Example");
    parser.addHelpOption();
    parser.addPositionalArgument("date; names",
                                 "Date of transition between "
                                 "Julian and Gregorian calendars "
                                 "as string in the format 'yyyy-MM-dd;'. Optionally, user can "
                                 "provide names for the calendar separated with ';'");
    parser.process(a);
    const QStringList args = parser.positionalArguments();
    if (args.isEmpty())
        parser.showHelp(1);
    if (args.at(0).isEmpty())
        parser.showHelp(1);
//![0]
    QPluginLoader loader;
    loader.setFileName("../plugin/calendarPlugin");
    loader.load();
    if (!loader.isLoaded())
        return 1;
    auto *myplugin = qobject_cast<RequestedCalendarInterface*>(loader.instance());
//![0]
//![1]
    const auto cid = myplugin->loadCalendar(args.at(0));
    if (!cid.isValid()) {
        qWarning() << "Invalid ID";
        parser.showHelp(1);
    }
    const QCalendar calendar(cid);
//![1]
//![2]
    QCalendarWidget widget;
    widget.setCalendar(calendar);
    widget.show();
    QCalendar::YearMonthDay when = { 1582, 10, 4 };
    QCalendar julian = QCalendar(QCalendar::System::Julian);
    auto got = QDate::fromString(args.at(0).left(10), u"yyyy-MM-dd", julian);
    if (got.isValid())
        when = julian.partsFromDate(got);
    widget.setCurrentPage(when.year, when.month);
//![2]
    return a.exec();
}
