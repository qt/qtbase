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

#ifndef QABSTRACTSPINBOX_P_H
#define QABSTRACTSPINBOX_P_H

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

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "QtWidgets/qabstractspinbox.h"

#include "QtWidgets/qlineedit.h"
#include "QtWidgets/qstyleoption.h"
#include "QtGui/qvalidator.h"
#include "QtCore/qdatetime.h"
#include "QtCore/qvariant.h"
#include "private/qwidget_p.h"

QT_REQUIRE_CONFIG(spinbox);

QT_BEGIN_NAMESPACE

QVariant operator+(const QVariant &arg1, const QVariant &arg2);
QVariant operator-(const QVariant &arg1, const QVariant &arg2);
QVariant operator*(const QVariant &arg1, double multiplier);
double operator/(const QVariant &arg1, const QVariant &arg2);

enum EmitPolicy {
    EmitIfChanged,
    AlwaysEmit,
    NeverEmit
};

enum Button {
    None = 0x000,
    Keyboard = 0x001,
    Mouse = 0x002,
    Wheel = 0x004,
    ButtonMask = 0x008,
    Up = 0x010,
    Down = 0x020,
    DirectionMask = 0x040
};
class QSpinBoxValidator;
class QAbstractSpinBoxPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QAbstractSpinBox)
public:
    QAbstractSpinBoxPrivate();
    ~QAbstractSpinBoxPrivate();

    void init();
    void reset();
    void updateState(bool up, bool fromKeyboard = false);
    QString stripped(const QString &text, int *pos = nullptr) const;
    bool specialValue() const;
    virtual QVariant getZeroVariant() const;
    virtual void setRange(const QVariant &min, const QVariant &max);
    void setValue(const QVariant &val, EmitPolicy ep, bool updateEdit = true);
    virtual QVariant bound(const QVariant &val, const QVariant &old = QVariant(), int steps = 0) const;
    virtual void updateEdit();

    virtual void emitSignals(EmitPolicy ep, const QVariant &old);
    virtual void interpret(EmitPolicy ep);
    virtual QString textFromValue(const QVariant &n) const;
    virtual QVariant valueFromText(const QString &input) const;

    void _q_editorTextChanged(const QString &);
    virtual void _q_editorCursorPositionChanged(int oldpos, int newpos);

    virtual QStyle::SubControl newHoverControl(const QPoint &pos);
    bool updateHoverControl(const QPoint &pos);

    virtual void clearCache() const;
    virtual void updateEditFieldGeometry();

    static int variantCompare(const QVariant &arg1, const QVariant &arg2);
    static QVariant variantBound(const QVariant &min, const QVariant &value, const QVariant &max);

    virtual QVariant calculateAdaptiveDecimalStep(int steps) const;

    QLineEdit *edit;
    QString prefix, suffix, specialValueText;
    QVariant value, minimum, maximum, singleStep;
    QMetaType::Type type;
    int spinClickTimerId, spinClickTimerInterval, spinClickThresholdTimerId, spinClickThresholdTimerInterval;
    int effectiveSpinRepeatRate;
    uint buttonState;
    mutable QString cachedText;
    mutable QVariant cachedValue;
    mutable QValidator::State cachedState;
    mutable QSize cachedSizeHint, cachedMinimumSizeHint;
    uint pendingEmit : 1;
    uint readOnly : 1;
    uint wrapping : 1;
    uint ignoreCursorPositionChanged : 1;
    uint frame : 1;
    uint accelerate : 1;
    uint keyboardTracking : 1;
    uint cleared : 1;
    uint ignoreUpdateEdit : 1;
    QAbstractSpinBox::CorrectionMode correctionMode;
    QAbstractSpinBox::StepType stepType = QAbstractSpinBox::StepType::DefaultStepType;
    Qt::KeyboardModifier stepModifier = Qt::ControlModifier;
    int acceleration;
    QStyle::SubControl hoverControl;
    QRect hoverRect;
    QAbstractSpinBox::ButtonSymbols buttonSymbols;
    QSpinBoxValidator *validator;
    uint showGroupSeparator : 1;
    int wheelDeltaRemainder;
};

class QSpinBoxValidator : public QValidator
{
public:
    QSpinBoxValidator(QAbstractSpinBox *qptr, QAbstractSpinBoxPrivate *dptr);
    QValidator::State validate(QString &input, int &) const override;
    void fixup(QString &) const override;
private:
    QAbstractSpinBox *qptr;
    QAbstractSpinBoxPrivate *dptr;
};

QT_END_NAMESPACE

#endif // QABSTRACTSPINBOX_P_H
