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

#ifndef QCOLORDIALOG_H
#define QCOLORDIALOG_H

#include <QtWidgets/qtwidgetsglobal.h>

#include <QtWidgets/qdialog.h>

QT_REQUIRE_CONFIG(colordialog);

QT_BEGIN_NAMESPACE

class QColorDialogPrivate;

class Q_WIDGETS_EXPORT QColorDialog : public QDialog
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QColorDialog)
    Q_PROPERTY(QColor currentColor READ currentColor WRITE setCurrentColor
               NOTIFY currentColorChanged)
    Q_PROPERTY(ColorDialogOptions options READ options WRITE setOptions)

public:
    enum ColorDialogOption {
        ShowAlphaChannel    = 0x00000001,
        NoButtons           = 0x00000002,
        DontUseNativeDialog = 0x00000004
    };
    Q_ENUM(ColorDialogOption)

    Q_DECLARE_FLAGS(ColorDialogOptions, ColorDialogOption)

    explicit QColorDialog(QWidget *parent = nullptr);
    explicit QColorDialog(const QColor &initial, QWidget *parent = nullptr);
    ~QColorDialog();

    void setCurrentColor(const QColor &color);
    QColor currentColor() const;

    QColor selectedColor() const;

    void setOption(ColorDialogOption option, bool on = true);
    bool testOption(ColorDialogOption option) const;
    void setOptions(ColorDialogOptions options);
    ColorDialogOptions options() const;

    using QDialog::open;
    void open(QObject *receiver, const char *member);

    void setVisible(bool visible) override;

    static QColor getColor(const QColor &initial = Qt::white,
                           QWidget *parent = nullptr,
                           const QString &title = QString(),
                           ColorDialogOptions options = ColorDialogOptions());

#if QT_DEPRECATED_SINCE(5, 12)
    QT_DEPRECATED_X("Use getColor()") static QRgb getRgba(QRgb rgba = 0xffffffff, bool *ok = nullptr, QWidget *parent = nullptr);
#endif

    static int customCount();
    static QColor customColor(int index);
    static void setCustomColor(int index, QColor color);
    static QColor standardColor(int index);
    static void setStandardColor(int index, QColor color);

Q_SIGNALS:
    void currentColorChanged(const QColor &color);
    void colorSelected(const QColor &color);

protected:
    void changeEvent(QEvent *event) override;
    void done(int result) override;

private:
    Q_DISABLE_COPY(QColorDialog)

    Q_PRIVATE_SLOT(d_func(), void _q_addCustom())
    Q_PRIVATE_SLOT(d_func(), void _q_newHsv(int h, int s, int v))
    Q_PRIVATE_SLOT(d_func(), void _q_newColorTypedIn(QRgb rgb))
    Q_PRIVATE_SLOT(d_func(), void _q_nextCustom(int, int))
    Q_PRIVATE_SLOT(d_func(), void _q_newCustom(int, int))
    Q_PRIVATE_SLOT(d_func(), void _q_newStandard(int, int))
    Q_PRIVATE_SLOT(d_func(), void _q_pickScreenColor())
    Q_PRIVATE_SLOT(d_func(), void _q_updateColorPicking())
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QColorDialog::ColorDialogOptions)

QT_END_NAMESPACE

#endif // QCOLORDIALOG_H
