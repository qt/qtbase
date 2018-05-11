/****************************************************************************
**
** Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
** Copyright (C) 2016 Samuel Gaist <samuel.gaist@edeltech.ch>
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

#include "regularexpressiondialog.h"

#include <QApplication>

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QSpinBox>
#include <QPlainTextEdit>
#include <QTreeWidget>

#include <QAction>
#include <QClipboard>
#include <QContextMenuEvent>

#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>

#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>

Q_DECLARE_METATYPE(QRegularExpression::MatchType)

static QString rawStringLiteral(QString pattern)
{
    pattern.prepend(QLatin1String("R\"RX("));
    pattern.append(QLatin1String(")RX\""));
    return pattern;
}

static QString patternToCode(QString pattern)
{
    pattern.replace(QLatin1String("\\"), QLatin1String("\\\\"));
    pattern.replace(QLatin1String("\""), QLatin1String("\\\""));
    pattern.prepend(QLatin1Char('"'));
    pattern.append(QLatin1Char('"'));
    return pattern;
}

static QString codeToPattern(QString code)
{
    for (int i = 0; i < code.size(); ++i) {
        if (code.at(i) == QLatin1Char('\\'))
            code.remove(i, 1);
    }
    if (code.startsWith(QLatin1Char('"')) && code.endsWith(QLatin1Char('"'))) {
        code.chop(1);
        code.remove(0, 1);
    }
    return code;
}

class PatternLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit PatternLineEdit(QWidget *parent = nullptr);

private slots:
    void copyToCode();
    void pasteFromCode();
    void escapeSelection();

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    QAction *escapeSelectionAction;
    QAction *copyToCodeAction;
    QAction *pasteFromCodeAction;
};

PatternLineEdit::PatternLineEdit(QWidget *parent) :
    QLineEdit(parent),
    escapeSelectionAction(new QAction(tr("Escape Selection"), this)),
    copyToCodeAction(new QAction(tr("Copy to Code"), this)),
    pasteFromCodeAction(new QAction(tr("Paste from Code"), this))
{
    setClearButtonEnabled(true);
    connect(escapeSelectionAction, &QAction::triggered, this, &PatternLineEdit::escapeSelection);
    connect(copyToCodeAction, &QAction::triggered, this, &PatternLineEdit::copyToCode);
    connect(pasteFromCodeAction, &QAction::triggered, this, &PatternLineEdit::pasteFromCode);
#if !QT_CONFIG(clipboard)
    copyToCodeAction->setEnabled(false);
    pasteFromCodeAction->setEnabled(false);
#endif
}

void PatternLineEdit::escapeSelection()
{
    const QString selection = selectedText();
    const QString escapedSelection = QRegularExpression::escape(selection);
    if (escapedSelection != selection) {
        QString t = text();
        t.replace(selectionStart(), selection.size(), escapedSelection);
        setText(t);
    }
}

void PatternLineEdit::copyToCode()
{
#if QT_CONFIG(clipboard)
    QGuiApplication::clipboard()->setText(patternToCode(text()));
#endif
}

void PatternLineEdit::pasteFromCode()
{
#if QT_CONFIG(clipboard)
    setText(codeToPattern(QGuiApplication::clipboard()->text()));
#endif
}

void PatternLineEdit::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->addSeparator();
    escapeSelectionAction->setEnabled(hasSelectedText());
    menu->addAction(escapeSelectionAction);
    menu->addSeparator();
    menu->addAction(copyToCodeAction);
    menu->addAction(pasteFromCodeAction);
    menu->popup(event->globalPos());
}

class DisplayLineEdit : public QLineEdit
{
public:
    explicit DisplayLineEdit(QWidget *parent = nullptr);
};

DisplayLineEdit::DisplayLineEdit(QWidget *parent) : QLineEdit(parent)
{
    setReadOnly(true);
    QPalette disabledPalette = palette();
    disabledPalette.setBrush(QPalette::Base, disabledPalette.brush(QPalette::Disabled, QPalette::Base));
    setPalette(disabledPalette);

#if QT_CONFIG(clipboard)
    QAction *copyAction = new QAction(this);
    copyAction->setText(RegularExpressionDialog::tr("Copy to clipboard"));
    copyAction->setIcon(QIcon(QStringLiteral(":/images/copy.png")));
    connect(copyAction, &QAction::triggered, this,
            [this] () { QGuiApplication::clipboard()->setText(text()); });
    addAction(copyAction, QLineEdit::TrailingPosition);
#endif
}

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

    connect(offsetSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &RegularExpressionDialog::refresh);

    connect(matchTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RegularExpressionDialog::refresh);

    connect(anchoredMatchOptionCheckBox, &QCheckBox::toggled, this, &RegularExpressionDialog::refresh);
    connect(dontCheckSubjectStringMatchOptionCheckBox, &QCheckBox::toggled, this, &RegularExpressionDialog::refresh);

    patternLineEdit->setText(tr("(\\+?\\d+)-(?<prefix>\\d+)-(?<number>\\w+)"));
    subjectTextEdit->setPlainText(tr("My office number is +43-152-0123456, my mobile is 001-41-255512 instead."));

    refresh();
}

void RegularExpressionDialog::setResultUiEnabled(bool enabled)
{
    matchDetailsTreeWidget->setEnabled(enabled);
    namedGroupsTreeWidget->setEnabled(enabled);
}

static void setTextColor(QWidget *widget, const QColor &color)
{
    QPalette palette = widget->palette();
    palette.setColor(QPalette::Text, color);
    widget->setPalette(palette);
}

void RegularExpressionDialog::refresh()
{
    setUpdatesEnabled(false);

    const QString pattern = patternLineEdit->text();
    const QString text = subjectTextEdit->toPlainText();

    offsetSpinBox->setMaximum(qMax(0, text.length() - 1));

    escapedPatternLineEdit->setText(patternToCode(pattern));
    rawStringLiteralLineEdit->setText(rawStringLiteral(pattern));

    setTextColor(patternLineEdit, subjectTextEdit->palette().color(QPalette::Text));
    matchDetailsTreeWidget->clear();
    namedGroupsTreeWidget->clear();
    regexpStatusLabel->setText(QString());

    if (pattern.isEmpty()) {
        setResultUiEnabled(false);
        setUpdatesEnabled(true);
        return;
    }

    QRegularExpression rx(pattern);
    if (!rx.isValid()) {
        setTextColor(patternLineEdit, Qt::red);
        regexpStatusLabel->setText(tr("Invalid: syntax error at position %1 (%2)")
                                   .arg(rx.patternErrorOffset())
                                   .arg(rx.errorString()));
        setResultUiEnabled(false);
        setUpdatesEnabled(true);
        return;
    }

    setResultUiEnabled(true);

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

    matchDetailsTreeWidget->expandAll();

    regexpStatusLabel->setText(tr("Valid"));

    const QStringList namedCaptureGroups = rx.namedCaptureGroups();
    for (int i = 0; i < namedCaptureGroups.size(); ++i) {
        const QString currentNamedCaptureGroup = namedCaptureGroups.at(i);

        QTreeWidgetItem *namedGroupItem = new QTreeWidgetItem(namedGroupsTreeWidget);
        namedGroupItem->setText(0, QString::number(i));
        namedGroupItem->setText(1, currentNamedCaptureGroup.isNull() ? tr("<no name>") : currentNamedCaptureGroup);
    }


    setUpdatesEnabled(true);
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

    patternLineEdit = new PatternLineEdit;
    patternLineEdit->setClearButtonEnabled(true);
    layout->addRow(tr("&Pattern:"), patternLineEdit);

    rawStringLiteralLineEdit = new DisplayLineEdit;
    layout->addRow(tr("&Raw string literal:"), rawStringLiteralLineEdit);
    escapedPatternLineEdit = new DisplayLineEdit;
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

#include "regularexpressiondialog.moc"
