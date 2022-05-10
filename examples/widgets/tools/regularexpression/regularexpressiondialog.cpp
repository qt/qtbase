// Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// Copyright (C) 2016 Samuel Gaist <samuel.gaist@edeltech.ch>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
#include <QFont>
#include <QFontDatabase>

#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>

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

static QFrame *createHorizontalSeparator()
{
    auto *result = new QFrame;
    result->setFrameStyle(QFrame::HLine | QFrame::Sunken);
    return result;
}

static QFrame *createVerticalSeparator()
{
    auto *result = new QFrame;
    result->setFrameStyle(QFrame::VLine | QFrame::Sunken);
    return result;
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

    connect(offsetSpinBox, &QSpinBox::valueChanged,
            this, &RegularExpressionDialog::refresh);

    connect(matchTypeComboBox, &QComboBox::currentIndexChanged,
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
    replacementTextEdit->setEnabled(enabled);
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
    replacementTextEdit->clear();

    if (pattern.isEmpty()) {
        setResultUiEnabled(false);
        setUpdatesEnabled(true);
        return;
    }

    regularExpression.setPattern(pattern);
    if (!regularExpression.isValid()) {
        setTextColor(patternLineEdit, Qt::red);
        regexpStatusLabel->setText(tr("Invalid: syntax error at position %1 (%2)")
                                   .arg(regularExpression.patternErrorOffset())
                                   .arg(regularExpression.errorString()));
        setResultUiEnabled(false);
        setUpdatesEnabled(true);
        return;
    }

    setResultUiEnabled(true);

    QRegularExpression::MatchType matchType = qvariant_cast<QRegularExpression::MatchType>(matchTypeComboBox->currentData());
    QRegularExpression::PatternOptions patternOptions = QRegularExpression::NoPatternOption;
    QRegularExpression::MatchOptions matchOptions = QRegularExpression::NoMatchOption;

    if (anchoredMatchOptionCheckBox->isChecked())
        matchOptions |= QRegularExpression::AnchorAtOffsetMatchOption;
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

    regularExpression.setPatternOptions(patternOptions);

    const int capturingGroupsCount = regularExpression.captureCount() + 1;

    const int offset = offsetSpinBox->value();
    QRegularExpressionMatchIterator iterator =
        regularExpression.globalMatch(text, offset, matchType, matchOptions);
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

    const QStringList namedCaptureGroups = regularExpression.namedCaptureGroups();
    for (int i = 0; i < namedCaptureGroups.size(); ++i) {
        const QString currentNamedCaptureGroup = namedCaptureGroups.at(i);

        QTreeWidgetItem *namedGroupItem = new QTreeWidgetItem(namedGroupsTreeWidget);
        namedGroupItem->setText(0, QString::number(i));
        namedGroupItem->setText(1, currentNamedCaptureGroup.isNull() ? tr("<no name>") : currentNamedCaptureGroup);
    }

    updateReplacement();

    setUpdatesEnabled(true);
}

void RegularExpressionDialog::updateReplacement()
{
    replacementTextEdit->clear();
    const QString &replacement = replacementLineEdit->text();
    if (!regularExpression.isValid() || replacement.isEmpty())
        return;

    QString replaced = subjectTextEdit->toPlainText();
    replaced.replace(regularExpression, replacement);
    replacementTextEdit->setPlainText(replaced);
}

void RegularExpressionDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(setupTextUi());
    mainLayout->addWidget(createHorizontalSeparator());
    auto *horizontalLayout = new QHBoxLayout();
    mainLayout->addLayout(horizontalLayout);
    horizontalLayout->addWidget(setupOptionsUi());
    horizontalLayout->addWidget(createVerticalSeparator());
    horizontalLayout->addWidget(setupInfoUi());

    auto font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    patternLineEdit->setFont(font);
    rawStringLiteralLineEdit->setFont(font);
    escapedPatternLineEdit->setFont(font);
    replacementLineEdit->setFont(font);
    subjectTextEdit->setFont(font);
    replacementTextEdit->setFont(font);
}

QWidget *RegularExpressionDialog::setupTextUi()
{
    QWidget *container = new QWidget;

    QFormLayout *layout = new QFormLayout(container);
    layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    layout->setContentsMargins(QMargins());

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

    layout->addRow(createHorizontalSeparator());

    QLabel *replaceLabel = new QLabel(tr("<h3>Replacement"));
    layout->addRow(replaceLabel);

    replacementLineEdit = new QLineEdit;
    replacementLineEdit->setClearButtonEnabled(true);
    connect(replacementLineEdit, &QLineEdit::textChanged, this,
            &RegularExpressionDialog::updateReplacement);
    layout->addRow(tr("&Replace by:"), replacementLineEdit);
    replacementLineEdit->setToolTip(tr("Use \\1, \\2... as placeholders for the captured groups."));

    replacementTextEdit = new QPlainTextEdit;
    replacementTextEdit->setReadOnly(true);
    layout->addRow(tr("Result:"), replacementTextEdit);

    return container;
}

QWidget *RegularExpressionDialog::setupOptionsUi()
{
    QWidget *container = new QWidget;

    QFormLayout *layout = new QFormLayout(container);
    layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    layout->setContentsMargins(QMargins());

    layout->addRow(new QLabel(tr("<h3>Options</h3>")));

    caseInsensitiveOptionCheckBox = new QCheckBox(tr("Case insensitive (/i)"));
    dotMatchesEverythingOptionCheckBox = new QCheckBox(tr("Dot matches everything (/s)"));
    multilineOptionCheckBox = new QCheckBox(tr("Multiline (/m)"));
    extendedPatternSyntaxOptionCheckBox = new QCheckBox(tr("Extended pattern (/x)"));
    invertedGreedinessOptionCheckBox = new QCheckBox(tr("Inverted greediness"));
    dontCaptureOptionCheckBox = new QCheckBox(tr("Don't capture"));
    useUnicodePropertiesOptionCheckBox = new QCheckBox(tr("Use unicode properties (/u)"));

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

QWidget *RegularExpressionDialog::setupInfoUi()
{
    QWidget *container = new QWidget;

    QFormLayout *layout = new QFormLayout(container);
    layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    layout->setContentsMargins(QMargins());

    QLabel *matchInfoLabel = new QLabel(tr("<h3>Match information</h3>"));
    layout->addRow(matchInfoLabel);

    matchDetailsTreeWidget = new QTreeWidget;
    matchDetailsTreeWidget->setHeaderLabels(QStringList() << tr("Match index") << tr("Group index") << tr("Captured string"));
    matchDetailsTreeWidget->setSizeAdjustPolicy(QTreeWidget::AdjustToContents);
    layout->addRow(tr("Match details:"), matchDetailsTreeWidget);

    layout->addRow(createHorizontalSeparator());

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
