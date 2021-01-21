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

#ifndef QFONTCOMBOBOX_H
#define QFONTCOMBOBOX_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qcombobox.h>
#include <QtGui/qfontdatabase.h>

QT_REQUIRE_CONFIG(fontcombobox);

QT_BEGIN_NAMESPACE

class QFontComboBoxPrivate;

class Q_WIDGETS_EXPORT QFontComboBox : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(QFontDatabase::WritingSystem writingSystem READ writingSystem WRITE setWritingSystem)
    Q_PROPERTY(FontFilters fontFilters READ fontFilters WRITE setFontFilters)
    Q_PROPERTY(QFont currentFont READ currentFont WRITE setCurrentFont NOTIFY currentFontChanged)

public:
    explicit QFontComboBox(QWidget *parent = nullptr);
    ~QFontComboBox();

    void setWritingSystem(QFontDatabase::WritingSystem);
    QFontDatabase::WritingSystem writingSystem() const;

    enum FontFilter {
        AllFonts = 0,
        ScalableFonts = 0x1,
        NonScalableFonts = 0x2,
        MonospacedFonts = 0x4,
        ProportionalFonts = 0x8
    };
    Q_DECLARE_FLAGS(FontFilters, FontFilter)
    Q_FLAG(FontFilters)

    void setFontFilters(FontFilters filters);
    FontFilters fontFilters() const;

    QFont currentFont() const;
    QSize sizeHint() const override;

public Q_SLOTS:
    void setCurrentFont(const QFont &f);

Q_SIGNALS:
    void currentFontChanged(const QFont &f);

protected:
    bool event(QEvent *e) override;

private:
    Q_DISABLE_COPY(QFontComboBox)
    Q_DECLARE_PRIVATE(QFontComboBox)
    Q_PRIVATE_SLOT(d_func(), void _q_currentChanged(const QString &))
    Q_PRIVATE_SLOT(d_func(), void _q_updateModel())
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QFontComboBox::FontFilters)

QT_END_NAMESPACE

#endif
