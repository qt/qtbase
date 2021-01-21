/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QLCDNUMBER_H
#define QLCDNUMBER_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qframe.h>

QT_BEGIN_NAMESPACE

QT_REQUIRE_CONFIG(lcdnumber);

class QLCDNumberPrivate;
class Q_WIDGETS_EXPORT QLCDNumber : public QFrame // LCD number widget
{
    Q_OBJECT
    Q_PROPERTY(bool smallDecimalPoint READ smallDecimalPoint WRITE setSmallDecimalPoint)
    Q_PROPERTY(int digitCount READ digitCount WRITE setDigitCount)
    Q_PROPERTY(Mode mode READ mode WRITE setMode)
    Q_PROPERTY(SegmentStyle segmentStyle READ segmentStyle WRITE setSegmentStyle)
    Q_PROPERTY(double value READ value WRITE display)
    Q_PROPERTY(int intValue READ intValue WRITE display)

public:
    explicit QLCDNumber(QWidget* parent = nullptr);
    explicit QLCDNumber(uint numDigits, QWidget* parent = nullptr);
    ~QLCDNumber();

    enum Mode {
        Hex, Dec, Oct, Bin
    };
    Q_ENUM(Mode)
    enum SegmentStyle {
        Outline, Filled, Flat
    };
    Q_ENUM(SegmentStyle)

    bool smallDecimalPoint() const;
    int digitCount() const;
    void setDigitCount(int nDigits);

    bool checkOverflow(double num) const;
    bool checkOverflow(int num) const;

    Mode mode() const;
    void setMode(Mode);

    SegmentStyle segmentStyle() const;
    void setSegmentStyle(SegmentStyle);

    double value() const;
    int intValue() const;

    QSize sizeHint() const override;

public Q_SLOTS:
    void display(const QString &str);
    void display(int num);
    void display(double num);
    void setHexMode();
    void setDecMode();
    void setOctMode();
    void setBinMode();
    void setSmallDecimalPoint(bool);

Q_SIGNALS:
    void overflow();

protected:
    bool event(QEvent *e) override;
    void paintEvent(QPaintEvent *) override;

public:

private:
    Q_DISABLE_COPY(QLCDNumber)
    Q_DECLARE_PRIVATE(QLCDNumber)
};

QT_END_NAMESPACE

#endif // QLCDNUMBER_H
