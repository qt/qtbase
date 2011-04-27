/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <e32std.h>
#include <e32const.h>
#include <e32debug.h>

_LIT(KYear, "%Y");
_LIT(KMonth, "%M");
_LIT(KDay, "%D");
_LIT(KLocaleIndependent, "%F");
static TBuf<10> dateFormat;
static TBuf<10> timeFormat;

static void initialiseDateFormat()
{
    if(dateFormat.Length())
        return;

    TLocale locale;

    //Separator 1 is used between 1st and 2nd components of the date
    //Separator 2 is used between 2nd and 3rd components of the date
    //Usually they are the same, but they are allowed to be different
    TChar s1 = locale.DateSeparator(1);
    TChar s2 = locale.DateSeparator(2);
    dateFormat=KLocaleIndependent;
    switch(locale.DateFormat()) {
    case EDateAmerican:
        dateFormat.Append(KMonth);
        dateFormat.Append(s1);
        dateFormat.Append(KDay);
        dateFormat.Append(s2);
        dateFormat.Append(KYear);
        break;
    case EDateEuropean:
        dateFormat.Append(KDay);
        dateFormat.Append(s1);
        dateFormat.Append(KMonth);
        dateFormat.Append(s2);
        dateFormat.Append(KYear);
        break;
    case EDateJapanese:
    default: //it's closest to ISO format
        dateFormat.Append(KYear);
        dateFormat.Append(s1);
        dateFormat.Append(KMonth);
        dateFormat.Append(s2);
        dateFormat.Append(KDay);
        break;
    }
#ifdef _DEBUG
    RDebug::Print(_L("Date Format \"%S\""), &dateFormat);
#endif
}

static void initialiseTimeFormat()
{
    if(timeFormat.Length())
        return;

    TLocale locale;
    //Separator 1 is used between 1st and 2nd components of the time
    //Separator 2 is used between 2nd and 3rd components of the time
    //Usually they are the same, but they are allowed to be different
    TChar s1 = locale.TimeSeparator(1);
    TChar s2 = locale.TimeSeparator(2);
    switch(locale.TimeFormat()) {
    case ETime12:
        timeFormat.Append(_L("%I"));
        break;
    case ETime24:
    default:
        timeFormat.Append(_L("%H"));
        break;
    }
    timeFormat.Append(s1);
    timeFormat.Append(_L("%T"));
    timeFormat.Append(s2);
    timeFormat.Append(_L("%S"));

#ifdef _DEBUG
    RDebug::Print(_L("Time Format \"%S\""), &timeFormat);
#endif
}

EXPORT_C void defaultFormatL(TTime& time, TDes& des, const TDesC& fmt, const TLocale&)
{
    //S60 3.1 does not support format for a specific locale, so use default locale
    time.FormatL(des, fmt);
}

//S60 3.1 doesn't support extended locale date&time formats, so use default locale
EXPORT_C TPtrC defaultGetTimeFormatSpec(TExtendedLocale&)
{
    initialiseTimeFormat();
    return TPtrC(timeFormat);
}

EXPORT_C TPtrC defaultGetLongDateFormatSpec(TExtendedLocale&)
{
    initialiseDateFormat();
    return TPtrC(dateFormat);
}

EXPORT_C TPtrC defaultGetShortDateFormatSpec(TExtendedLocale&)
{
    initialiseDateFormat();
    return TPtrC(dateFormat);
}
