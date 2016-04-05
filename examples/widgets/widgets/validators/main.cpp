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

#include <qdebug.h>
#include <QApplication>
#include <QLineEdit>
#include <QValidator>

#include "ui_validators.h"

class ValidatorWidget : public QWidget, public Ui::ValidatorsForm
{
    Q_OBJECT
public:
    ValidatorWidget(QWidget *parent = 0);

private slots:
    void updateValidator();
    void updateDoubleValidator();
    void _setLocale(const QLocale &l) { setLocale(l); updateValidator(); updateDoubleValidator(); }

private:
    QIntValidator *validator;
    QDoubleValidator *doubleValidator;
};

ValidatorWidget::ValidatorWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);

    connect(localeSelector, SIGNAL(localeSelected(QLocale)), this, SLOT(_setLocale(QLocale)));

    connect(minVal, SIGNAL(editingFinished()), this, SLOT(updateValidator()));
    connect(maxVal, SIGNAL(editingFinished()), this, SLOT(updateValidator()));
    connect(editor, SIGNAL(editingFinished()), ledWidget, SLOT(flash()));

    connect(doubleMaxVal, SIGNAL(editingFinished()), this, SLOT(updateDoubleValidator()));
    connect(doubleMinVal, SIGNAL(editingFinished()), this, SLOT(updateDoubleValidator()));
    connect(doubleDecimals, SIGNAL(valueChanged(int)), this, SLOT(updateDoubleValidator()));
    connect(doubleFormat, SIGNAL(activated(int)), this, SLOT(updateDoubleValidator()));
    connect(doubleEditor, SIGNAL(editingFinished()), doubleLedWidget, SLOT(flash()));

    validator = 0;
    doubleValidator = 0;
    updateValidator();
    updateDoubleValidator();
};

void ValidatorWidget::updateValidator()
{
    QIntValidator *v = new QIntValidator(minVal->value(), maxVal->value(), this);
    v->setLocale(locale());
    editor->setValidator(v);
    delete validator;
    validator = v;

    QString s = editor->text();
    int i = 0;
    if (validator->validate(s, i) == QValidator::Invalid) {
        editor->clear();
    } else {
        editor->setText(s);
    }
}

void ValidatorWidget::updateDoubleValidator()
{
    QDoubleValidator *v
        = new QDoubleValidator(doubleMinVal->value(), doubleMaxVal->value(),
                                doubleDecimals->value(), this);
    v->setNotation(static_cast<QDoubleValidator::Notation>(doubleFormat->currentIndex()));
    v->setLocale(locale());
    doubleEditor->setValidator(v);
    delete doubleValidator;
    doubleValidator = v;

    QString s = doubleEditor->text();
    int i = 0;
    if (doubleValidator->validate(s, i) == QValidator::Invalid) {
        doubleEditor->clear();
    } else {
        doubleEditor->setText(s);
    }
}

int main(int argc, char **argv)
{
    Q_INIT_RESOURCE(validators);

    QApplication app(argc, argv);

    ValidatorWidget w;
    w.show();

    return app.exec();
}

#include "main.moc"
