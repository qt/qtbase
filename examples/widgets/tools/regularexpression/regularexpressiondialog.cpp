/****************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
** Copyright (C) 2015 Samuel Gaist <samuel.gaist@edeltech.ch>
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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

#include "regularexpressiondialog.h"

#include <QApplication>

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QPlainTextEdit>
#include <QTreeWidget>

#include <QAction>
#include <QClipboard>

#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>

#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>

Q_DECLARE_METATYPE(QRegularExpression::MatchType)

RegularExpressionDialog::RegularExpressionDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi();
    setWindowTitle(tr("QRegularExpression Example"));

    connect(patternLineEdit, &QLineEdit::textChanged, this, &RegularExpressionDialog::refresh);
    connect(subjectTextEdit, &QPlainTextEdit::textChanged, this, &RegularExpressionDialog::refresh);

    connect(caseInsensitiveOptionCheckBox, &QCheckBox::toggled, this, &RegularExpressionDialog::refresh);
    connect(dotMatchesEverythingOptionCheckBox, &QCheckBox::toggled, this, &RegularExpressionDialog::refresh);
    connect(multilineOptionCheckBox, &QCheckBox::toggled, this, &RegularExpressionDialog::refresh);
    connect(extendedPatternSyntaxOptionCheckBox, &QCheckBox::toggled, this, &RegularExpressionDialog::refresh);
    connect(invertedGreedinessOptionCheckBox, &QCheckBox::toggled, this, &RegularExpressionDialog::refresh);
    connect(dontCaptureOptionCheckBox, &QCheckBox::toggled, this, &RegularExpressionDialog::refresh);
    connect(useUnicodePropertiesOptionCheckBox, &QCheckBox::toggled, this, &RegularExpressionDialog::refresh);
    connect(optimizeOnFirstUsageOptionCheckBox, &QCheckBox::toggled, this, &RegularExpressionDialog::refresh);
    connect(dontAutomaticallyOptimizeOptionCheckBox, &QCheckBox::toggled, this, &RegularExpressionDialog::refresh);

    connect(offsetSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &RegularExpressionDialog::refresh);

    connect(matchTypeComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &RegularExpressionDialog::refresh);

    connect(anchoredMatchOptionCheckBox, &QCheckBox::toggled, this, &RegularExpressionDialog::refresh);
    connect(dontCheckSubjectStringMatchOptionCheckBox, &QCheckBox::toggled, this, &RegularExpressionDialog::refresh);

    patternLineEdit->setText(tr("(\\+?\\d+)-(?<prefix>\\d+)-(?<number>\\w+)"));
    subjectTextEdit->setPlainText(tr("My office number is +43-152-0123456, my mobile is 001-41-255512 instead."));

    refresh();
}

void RegularExpressionDialog::refresh()
{
    setUpdatesEnabled(false);

    const QString pattern = patternLineEdit->text();
    const QString text = subjectTextEdit->toPlainText();

    offsetSpinBox->setMaximum(qMax(0, text.length() - 1));

    QString escaped = pattern;
    escaped.replace(QLatin1String("\\"), QLatin1String("\\\\"));
    escaped.replace(QLatin1String("\""), QLatin1String("\\\""));
    escaped.prepend(QLatin1Char('"'));
    escaped.append(QLatin1Char('"'));
    escapedPatternLineEdit->setText(escaped);

    QRegularExpression rx(pattern);
    QRegularExpression::MatchType matchType = matchTypeComboBox->currentData().value<QRegularExpression::MatchType>();
    QRegularExpression::PatternOptions patternOptions = QRegularExpression::NoPatternOption;
    QRegularExpression::MatchOptions matchOptions = QRegularExpression::NoMatchOption;

    if (anchoredMatchOptionCheckBox->isChecked())
        matchOptions |= QRegularExpression::AnchoredMatchOption;
    if (dontCheckSubjectStringMatchOptionCheckBox->isChecked())
        matchOptions |= QRegularExpression::DontCheckSubjectStringMatchOption;

    if (caseInsensitiveOptionCheckBox->isChecked())
        patternOptions |= QRegularExpression::CaseInsensitiveOption;
    if (dotMatchesEverythingOptionCheckBox->isChecked())
        patternOptions |= QRegularExpression::DotMatchesEverythingOption;
    if (multilineOptionCheckBox->isChecked())
        patternOptions |= QRegularExpression::MultilineOption;
    if (extendedPatternSyntaxOptionCheckBox->isChecked())
        patternOptions |= QRegularExpression::ExtendedPatternSyntaxOption;
    if (invertedGreedinessOptionCheckBox->isChecked())
        patternOptions |= QRegularExpression::InvertedGreedinessOption;
    if (dontCaptureOptionCheckBox->isChecked())
        patternOptions |= QRegularExpression::DontCaptureOption;
    if (useUnicodePropertiesOptionCheckBox->isChecked())
        patternOptions |= QRegularExpression::UseUnicodePropertiesOption;
    if (optimizeOnFirstUsageOptionCheckBox->isChecked())
        patternOptions |= QRegularExpression::OptimizeOnFirstUsageOption;
    if (dontAutomaticallyOptimizeOptionCheckBox->isChecked())
        patternOptions |= QRegularExpression::DontAutomaticallyOptimizeOption;

    rx.setPatternOptions(patternOptions);

    QPalette palette = patternLineEdit->palette();
    if (rx.isValid())
        palette.setColor(QPalette::Text, subjectTextEdit->palette().color(QPalette::Text));
    else
        palette.setColor(QPalette::Text, Qt::red);
    patternLineEdit->setPalette(palette);

    matchDetailsTreeWidget->clear();
    matchDetailsTreeWidget->setEnabled(rx.isValid());

    if (rx.isValid()) {
        const int capturingGroupsCount = rx.captureCount() + 1;

        QRegularExpressionMatchIterator iterator = rx.globalMatch(text, offsetSpinBox->value(), matchType, matchOptions);
        int i = 0;

        while (iterator.hasNext()) {
            QRegularExpressionMatch match = iterator.next();

            QTreeWidgetItem *matchDetailTopItem = new QTreeWidgetItem(matchDetailsTreeWidget);
            matchDetailTopItem->setText(0, QString::number(i));

            for (int captureGroupIndex = 0; captureGroupIndex < capturingGroupsCount; ++captureGroupIndex) {
                QTreeWidgetItem *matchDetailItem = new QTreeWidgetItem(matchDetailTopItem);
                matchDetailItem->setText(1, QString::number(captureGroupIndex));
                matchDetailItem->setText(2, match.captured(captureGroupIndex));
            }

            ++i;
        }
    }

    matchDetailsTreeWidget->expandAll();

    namedGroupsTreeWidget->clear();
    namedGroupsTreeWidget->setEnabled(rx.isValid());

    if (rx.isValid()) {
        regexpStatusLabel->setText(tr("Valid"));

        const QStringList namedCaptureGroups = rx.namedCaptureGroups();
        for (int i = 0; i < namedCaptureGroups.size(); ++i) {
            const QString currentNamedCaptureGroup = namedCaptureGroups.at(i);

            QTreeWidgetItem *namedGroupItem = new QTreeWidgetItem(namedGroupsTreeWidget);
            namedGroupItem->setText(0, QString::number(i));
            namedGroupItem->setText(1, currentNamedCaptureGroup.isNull() ? tr("<no name>") : currentNamedCaptureGroup);
        }
    } else {
        regexpStatusLabel->setText(tr("Invalid: syntax error at position %1 (%2)")
                                   .arg(rx.patternErrorOffset())
                                   .arg(rx.errorString()));
    }

    setUpdatesEnabled(true);
}

void RegularExpressionDialog::copyEscapedPatternToClipboard()
{
#ifndef QT_NO_CLIPBOARD
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (clipboard)
        clipboard->setText(escapedPatternLineEdit->text());
#endif
}

void RegularExpressionDialog::setupUi()
{
    QWidget *leftHalfContainer = setupLeftUi();

    QFrame *verticalSeparator = new QFrame;
    verticalSeparator->setFrameStyle(QFrame::VLine | QFrame::Sunken);

    QWidget *rightHalfContainer = setupRightUi();

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(leftHalfContainer);
    mainLayout->addWidget(verticalSeparator);
    mainLayout->addWidget(rightHalfContainer);

    setLayout(mainLayout);
}

QWidget *RegularExpressionDialog::setupLeftUi()
{
    QWidget *container = new QWidget;

    QFormLayout *layout = new QFormLayout(container);
    layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    layout->setMargin(0);

    QLabel *regexpAndSubjectLabel = new QLabel(tr("<h3>Regular expression and text input</h3>"));
    layout->addRow(regexpAndSubjectLabel);

    patternLineEdit = new QLineEdit;
    patternLineEdit->setClearButtonEnabled(true);
    layout->addRow(tr("&Pattern:"), patternLineEdit);

    escapedPatternLineEdit = new QLineEdit;
    escapedPatternLineEdit->setReadOnly(true);
    QPalette palette = escapedPatternLineEdit->palette();
    palette.setBrush(QPalette::Base, palette.brush(QPalette::Disabled, QPalette::Base));
    escapedPatternLineEdit->setPalette(palette);

#ifndef QT_NO_CLIPBOARD
    QAction *copyEscapedPatternAction = new QAction(this);
    copyEscapedPatternAction->setText(tr("Copy to clipboard"));
    copyEscapedPatternAction->setIcon(QIcon(QStringLiteral(":/images/copy.png")));
    connect(copyEscapedPatternAction, &QAction::triggered, this, &RegularExpressionDialog::copyEscapedPatternToClipboard);
    escapedPatternLineEdit->addAction(copyEscapedPatternAction, QLineEdit::TrailingPosition);
#endif

    layout->addRow(tr("&Escaped pattern:"), escapedPatternLineEdit);

    subjectTextEdit = new QPlainTextEdit;
    layout->addRow(tr("&Subject text:"), subjectTextEdit);

    caseInsensitiveOptionCheckBox = new QCheckBox(tr("Case insensitive (/i)"));
    dotMatchesEverythingOptionCheckBox = new QCheckBox(tr("Dot matches everything (/s)"));
    multilineOptionCheckBox = new QCheckBox(tr("Multiline (/m)"));
    extendedPatternSyntaxOptionCheckBox = new QCheckBox(tr("Extended pattern (/x)"));
    invertedGreedinessOptionCheckBox = new QCheckBox(tr("Inverted greediness"));
    dontCaptureOptionCheckBox = new QCheckBox(tr("Don't capture"));
    useUnicodePropertiesOptionCheckBox = new QCheckBox(tr("Use unicode properties (/u)"));
    optimizeOnFirstUsageOptionCheckBox = new QCheckBox(tr("Optimize on first usage"));
    dontAutomaticallyOptimizeOptionCheckBox = new QCheckBox(tr("Don't automatically optimize"));

    QGridLayout *patternOptionsCheckBoxLayout = new QGridLayout;
    int gridRow = 0;
    patternOptionsCheckBoxLayout->addWidget(caseInsensitiveOptionCheckBox, gridRow, 1);
    patternOptionsCheckBoxLayout->addWidget(dotMatchesEverythingOptionCheckBox, gridRow, 2);
    ++gridRow;
    patternOptionsCheckBoxLayout->addWidget(multilineOptionCheckBox, gridRow, 1);
    patternOptionsCheckBoxLayout->addWidget(extendedPatternSyntaxOptionCheckBox, gridRow, 2);
    ++gridRow;
    patternOptionsCheckBoxLayout->addWidget(invertedGreedinessOptionCheckBox, gridRow, 1);
    patternOptionsCheckBoxLayout->addWidget(dontCaptureOptionCheckBox, gridRow, 2);
    ++gridRow;
    patternOptionsCheckBoxLayout->addWidget(useUnicodePropertiesOptionCheckBox, gridRow, 1);
    patternOptionsCheckBoxLayout->addWidget(optimizeOnFirstUsageOptionCheckBox, gridRow, 2);
    ++gridRow;
    patternOptionsCheckBoxLayout->addWidget(dontAutomaticallyOptimizeOptionCheckBox, gridRow, 1);

    layout->addRow(tr("Pattern options:"), patternOptionsCheckBoxLayout);

    offsetSpinBox = new QSpinBox;
    layout->addRow(tr("Match &offset:"), offsetSpinBox);

    matchTypeComboBox = new QComboBox;
    matchTypeComboBox->addItem(tr("Normal"), QVariant::fromValue(QRegularExpression::NormalMatch));
    matchTypeComboBox->addItem(tr("Partial prefer complete"), QVariant::fromValue(QRegularExpression::PartialPreferCompleteMatch));
    matchTypeComboBox->addItem(tr("Partial prefer first"), QVariant::fromValue(QRegularExpression::PartialPreferFirstMatch));
    matchTypeComboBox->addItem(tr("No match"), QVariant::fromValue(QRegularExpression::NoMatch));
    layout->addRow(tr("Match &type:"), matchTypeComboBox);

    dontCheckSubjectStringMatchOptionCheckBox = new QCheckBox(tr("Don't check subject string"));
    anchoredMatchOptionCheckBox = new QCheckBox(tr("Anchored match"));

    QGridLayout *matchOptionsCheckBoxLayout = new QGridLayout;
    matchOptionsCheckBoxLayout->addWidget(dontCheckSubjectStringMatchOptionCheckBox, 0, 0);
    matchOptionsCheckBoxLayout->addWidget(anchoredMatchOptionCheckBox, 0, 1);
    layout->addRow(tr("Match options:"), matchOptionsCheckBoxLayout);

    return container;
}

QWidget *RegularExpressionDialog::setupRightUi()
{
    QWidget *container = new QWidget;

    QFormLayout *layout = new QFormLayout(container);
    layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    layout->setMargin(0);

    QLabel *matchInfoLabel = new QLabel(tr("<h3>Match information</h3>"));
    layout->addRow(matchInfoLabel);

    matchDetailsTreeWidget = new QTreeWidget;
    matchDetailsTreeWidget->setHeaderLabels(QStringList() << tr("Match index") << tr("Group index") << tr("Captured string"));
    matchDetailsTreeWidget->setSizeAdjustPolicy(QTreeWidget::AdjustToContents);
    layout->addRow(tr("Match details:"), matchDetailsTreeWidget);

    QFrame *horizontalSeparator = new QFrame;
    horizontalSeparator->setFrameStyle(QFrame::HLine | QFrame::Sunken);
    layout->addRow(horizontalSeparator);

    QLabel *regexpInfoLabel = new QLabel(tr("<h3>Regular expression information</h3>"));
    layout->addRow(regexpInfoLabel);

    regexpStatusLabel = new QLabel(tr("Valid"));
    regexpStatusLabel->setWordWrap(true);
    layout->addRow(tr("Pattern status:"), regexpStatusLabel);

    namedGroupsTreeWidget = new QTreeWidget;
    namedGroupsTreeWidget->setHeaderLabels(QStringList() << tr("Index") << tr("Named group"));
    namedGroupsTreeWidget->setSizeAdjustPolicy(QTreeWidget::AdjustToContents);
    namedGroupsTreeWidget->setRootIsDecorated(false);
    layout->addRow(tr("Named groups:"), namedGroupsTreeWidget);

    return container;
}
