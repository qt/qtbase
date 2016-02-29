/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <qt_windows.h>

#include <ocidl.h>
#include <olectl.h>

#include "comutils.h"
#include <QtCore/qdatetime.h>
#include <QtGui/qpixmap.h>
#include <QtGui/qfont.h>


#include <QtCore/qvariant.h>
#include <QtCore/qbytearray.h>
#include <QtGui/qcolor.h>

QT_BEGIN_NAMESPACE

static DATE QDateTimeToDATE(const QDateTime &dt)
{
    if (!dt.isValid() || dt.isNull())
        return 949998;  // Special value for no date (01/01/4501)

    SYSTEMTIME stime;
    memset(&stime, 0, sizeof(stime));
    QDate date = dt.date();
    QTime time = dt.time();
    if (date.isValid() && !date.isNull()) {
        stime.wDay = WORD(date.day());
        stime.wMonth = WORD(date.month());
        stime.wYear = WORD(date.year());
    }
    if (time.isValid() && !time.isNull()) {
        stime.wMilliseconds = WORD(time.msec());
        stime.wSecond = WORD(time.second());
        stime.wMinute = WORD(time.minute());
        stime.wHour = WORD(time.hour());
    }

    double vtime;
    SystemTimeToVariantTime(&stime, &vtime);

    return vtime;
}

inline uint QColorToOLEColor(const QColor &col)
{
    return qRgba(col.blue(), col.green(), col.red(), 0x00);
}

bool QVariant2VARIANT(const QVariant &var, VARIANT &arg, const QByteArray &typeName, bool out)
{
    QVariant qvar = var;
    // "type" is the expected type, so coerce if necessary
    QVariant::Type proptype = typeName.isEmpty() ? QVariant::Invalid : QVariant::nameToType(typeName);
    if (proptype == QVariant::UserType && !typeName.isEmpty()) {
        if (typeName == "short" || typeName == "char")
            proptype = QVariant::Int;
        else if (typeName == "float")
            proptype = QVariant::Double;
    }
    if (proptype != QVariant::Invalid && proptype != QVariant::UserType && proptype != qvar.type()) {
        if (qvar.canConvert(int(proptype)))
            qvar.convert(int(proptype));
        else
            qvar = QVariant(proptype);
    }

    if (out && arg.vt == (VT_VARIANT|VT_BYREF) && arg.pvarVal) {
        return QVariant2VARIANT(var, *arg.pvarVal, typeName, false);
    }

    if (out && proptype == QVariant::UserType && typeName == "QVariant") {
        VARIANT *pVariant = new VARIANT;
        QVariant2VARIANT(var, *pVariant, QByteArray(), false);
        arg.vt = VT_VARIANT|VT_BYREF;
        arg.pvarVal = pVariant;
        return true;
    }

    switch ((int)qvar.type()) {
    case QVariant::String:
        if (out && arg.vt == (VT_BSTR|VT_BYREF)) {
            if (*arg.pbstrVal)
                SysFreeString(*arg.pbstrVal);
            *arg.pbstrVal = QStringToBSTR(qvar.toString());
            arg.vt = VT_BSTR|VT_BYREF;
        } else {
            arg.vt = VT_BSTR;
            arg.bstrVal = QStringToBSTR(qvar.toString());
            if (out) {
                arg.pbstrVal = new BSTR(arg.bstrVal);
                arg.vt |= VT_BYREF;
            }
        }
        break;

    case QVariant::Int:
        if (out && arg.vt == (VT_I4|VT_BYREF)) {
            *arg.plVal = qvar.toInt();
        } else {
            arg.vt = VT_I4;
            arg.lVal = qvar.toInt();
            if (out) {
                if (typeName == "short") {
                    arg.vt = VT_I2;
                    arg.piVal = new short(arg.lVal);
                } else if (typeName == "char") {
                    arg.vt = VT_I1;
                    arg.pcVal= new char(arg.lVal);
                } else {
                    arg.plVal = new long(arg.lVal);
                }
                arg.vt |= VT_BYREF;
            }
        }
        break;

    case QVariant::UInt:
        if (out && (arg.vt == (VT_UINT|VT_BYREF) || arg.vt == (VT_I4|VT_BYREF))) {
            *arg.puintVal = qvar.toUInt();
        } else {
            arg.vt = VT_UINT;
            arg.uintVal = qvar.toUInt();
            if (out) {
                arg.puintVal = new uint(arg.uintVal);
                arg.vt |= VT_BYREF;
            }
        }
        break;

    case QVariant::LongLong:
        if (out && arg.vt == (VT_CY|VT_BYREF)) {
            arg.pcyVal->int64 = qvar.toLongLong();
#if !defined(Q_OS_WINCE) && defined(_MSC_VER) && _MSC_VER >= 1400
        } else if (out && arg.vt == (VT_I8|VT_BYREF)) {
            *arg.pllVal = qvar.toLongLong();
        } else {
            arg.vt = VT_I8;
            arg.llVal = qvar.toLongLong();
            if (out) {
                arg.pllVal = new LONGLONG(arg.llVal);
                arg.vt |= VT_BYREF;
            }
        }
#else
        } else {
            arg.vt = VT_CY;
            arg.cyVal.int64 = qvar.toLongLong();
            if (out) {
                arg.pcyVal = new CY(arg.cyVal);
                arg.vt |= VT_BYREF;
            }
        }
#endif
        break;

    case QVariant::ULongLong:
        if (out && arg.vt == (VT_CY|VT_BYREF)) {
            arg.pcyVal->int64 = qvar.toULongLong();
#if !defined(Q_OS_WINCE) && defined(_MSC_VER) && _MSC_VER >= 1400
        } else if (out && arg.vt == (VT_UI8|VT_BYREF)) {
            *arg.pullVal = qvar.toULongLong();
        } else {
            arg.vt = VT_UI8;
            arg.ullVal = qvar.toULongLong();
            if (out) {
                arg.pullVal = new ULONGLONG(arg.ullVal);
                arg.vt |= VT_BYREF;
            }
        }
#else
        } else {
            arg.vt = VT_CY;
            arg.cyVal.int64 = qvar.toULongLong();
            if (out) {
                arg.pcyVal = new CY(arg.cyVal);
                arg.vt |= VT_BYREF;
            }
        }

#endif

        break;

    case QVariant::Bool:
        if (out && arg.vt == (VT_BOOL|VT_BYREF)) {
            *arg.pboolVal = qvar.toBool() ? VARIANT_TRUE : VARIANT_FALSE;
        } else {
            arg.vt = VT_BOOL;
            arg.boolVal = qvar.toBool() ? VARIANT_TRUE : VARIANT_FALSE;
            if (out) {
                arg.pboolVal = new short(arg.boolVal);
                arg.vt |= VT_BYREF;
            }
        }
        break;
    case QVariant::Double:
        if (out && arg.vt == (VT_R8|VT_BYREF)) {
            *arg.pdblVal = qvar.toDouble();
        } else {
            arg.vt = VT_R8;
            arg.dblVal = qvar.toDouble();
            if (out) {
                if (typeName == "float") {
                    arg.vt = VT_R4;
                    arg.pfltVal = new float(arg.dblVal);
                } else {
                    arg.pdblVal = new double(arg.dblVal);
                }
                arg.vt |= VT_BYREF;
            }
        }
        break;
    case QVariant::Color:
        if (out && arg.vt == (VT_COLOR|VT_BYREF)) {

            *arg.plVal = QColorToOLEColor(qvariant_cast<QColor>(qvar));
        } else {
            arg.vt = VT_COLOR;
            arg.lVal = QColorToOLEColor(qvariant_cast<QColor>(qvar));
            if (out) {
                arg.plVal = new long(arg.lVal);
                arg.vt |= VT_BYREF;
            }
        }
        break;

    case QVariant::Date:
    case QVariant::Time:
    case QVariant::DateTime:
        if (out && arg.vt == (VT_DATE|VT_BYREF)) {
            *arg.pdate = QDateTimeToDATE(qvar.toDateTime());
        } else {
            arg.vt = VT_DATE;
            arg.date = QDateTimeToDATE(qvar.toDateTime());
            if (out) {
                arg.pdate = new DATE(arg.date);
                arg.vt |= VT_BYREF;
            }
        }
        break;

    case QVariant::Invalid: // default-parameters not set
        if (out && arg.vt == (VT_ERROR|VT_BYREF)) {
            *arg.plVal = DISP_E_PARAMNOTFOUND;
        } else {
            arg.vt = VT_ERROR;
            arg.lVal = DISP_E_PARAMNOTFOUND;
            if (out) {
                arg.plVal = new long(arg.lVal);
                arg.vt |= VT_BYREF;
            }
        }
        break;

    default:
        return false;
    }

    Q_ASSERT(!out || (arg.vt & VT_BYREF));
    return true;
}

QT_END_NAMESPACE

