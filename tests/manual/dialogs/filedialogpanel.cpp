/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "filedialogpanel.h"

#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QSpacerItem>
#include <QGroupBox>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QLabel>
#include <QMessageBox>
#include <QApplication>

#include <QTimer>
#include <QDebug>

struct ComboData
{
    const char *description;
    int value;
};

const ComboData acceptModeComboData[] =
{
{"AcceptOpen", QFileDialog::AcceptOpen },
{"AcceptSave", QFileDialog::AcceptSave }
};

const ComboData viewModeComboData[] =
{
    {"Detail", QFileDialog::Detail},
    {"List", QFileDialog::List}
};

const ComboData fileModeComboData[] =
{
    {"AnyFile", QFileDialog::AnyFile},
    {"ExistingFile", QFileDialog::ExistingFile},
    {"ExistingFiles", QFileDialog::ExistingFiles},
    {"Directory", QFileDialog::Directory},
    {"DirectoryOnly", QFileDialog::DirectoryOnly}
};

static QComboBox *createCombo(QWidget *parent, const ComboData *d, size_t size)
{
    QComboBox *c = new QComboBox(parent);
    for (size_t i = 0; i < size; ++i)
        c->addItem(QLatin1String(d[i].description), QVariant(d[i].value));
    return c;
}

template <class Enum>
Enum comboBoxValue(const QComboBox *c)
{
    return static_cast<Enum>(c->itemData(c->currentIndex()).toInt());
}

inline void setComboBoxValue(QComboBox *c, int v)
{
    c->setCurrentIndex(c->findData(QVariant(v)));
}

static inline QPushButton *addButton(const QString &description, QGridLayout *layout,
                                     int &row, int column, QObject *receiver, const char *slotFunc)
{
    QPushButton *button = new QPushButton(description);
    QObject::connect(button, SIGNAL(clicked()), receiver, slotFunc);
    layout->addWidget(button, row++, column);
    return button;
}

// A line edit for editing the label fields of the dialog, keeping track of whether it has
// been modified by the user to avoid applying Qt's default texts to native dialogs.

class LabelLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit LabelLineEdit(QFileDialog::DialogLabel label, QWidget *parent = 0) : QLineEdit(parent), m_label(label), m_dirty(false)
    {
        connect(this, SIGNAL(textEdited(QString)), this, SLOT(setDirty()));
    }

    void restoreDefault(const QFileDialog *d)
    {
        setText(d->labelText(m_label));
        m_dirty = false;
    }

    void apply(QFileDialog *d) const
    {
        if (m_dirty)
            d->setLabelText(m_label, text());
    }

private slots:
    void setDirty() { m_dirty = true; }

private:
    const QFileDialog::DialogLabel m_label;
    bool m_dirty;
};

FileDialogPanel::FileDialogPanel(QWidget *parent)
    : QWidget(parent)
    , m_readOnly(new QCheckBox(tr("Read only")))
    , m_confirmOverWrite(new QCheckBox(tr("Confirm overwrite")))
    , m_nameFilterDetailsVisible(new QCheckBox(tr("Name filter details visible")))
    , m_resolveSymLinks(new QCheckBox(tr("Resolve symlinks")))
    , m_native(new QCheckBox(tr("Use native dialog")))
    , m_acceptMode(createCombo(this, acceptModeComboData, sizeof(acceptModeComboData)/sizeof(ComboData)))
    , m_fileMode(createCombo(this, fileModeComboData, sizeof(fileModeComboData)/sizeof(ComboData)))
    , m_viewMode(createCombo(this, viewModeComboData, sizeof(viewModeComboData)/sizeof(ComboData)))
    , m_defaultSuffix(new QLineEdit(this))
    , m_directory(new QLineEdit(this))
    , m_selectedFileName(new QLineEdit(this))
    , m_nameFilters(new QPlainTextEdit)
    , m_selectedNameFilter(new QLineEdit(this))
    , m_deleteNonModalDialogButton(0)
    , m_deleteModalDialogButton(0)
{
    // Options
    QGroupBox *optionsGroupBox = new QGroupBox(tr("Options"));
    QFormLayout *optionsLayout = new QFormLayout(optionsGroupBox);
    optionsLayout->addRow(tr("AcceptMode:"), m_acceptMode);
    optionsLayout->addRow(tr("FileMode:"), m_fileMode);
    optionsLayout->addRow(tr("ViewMode:"), m_viewMode);
    optionsLayout->addRow(m_native);
    optionsLayout->addRow(m_confirmOverWrite);
    optionsLayout->addRow(m_nameFilterDetailsVisible);
    optionsLayout->addRow(m_resolveSymLinks);
    optionsLayout->addRow(m_readOnly);

    // Files
    QGroupBox *filesGroupBox = new QGroupBox(tr("Files / Filters"));
    QFormLayout *filesLayout = new QFormLayout(filesGroupBox);
    filesLayout->addRow(tr("Default Suffix:"), m_defaultSuffix);
    filesLayout->addRow(tr("Directory:"), m_directory);
    filesLayout->addRow(tr("Selected file:"), m_selectedFileName);
    m_nameFilters->setMaximumHeight(80);
    filesLayout->addRow(tr("Name filters:"), m_nameFilters);
    filesLayout->addRow(tr("Selected name filter:"), m_selectedNameFilter);

    // Optional labels
    QGroupBox *labelsGroupBox = new QGroupBox(tr("Labels"));
    QFormLayout *labelsLayout = new QFormLayout(labelsGroupBox);
    m_labelLineEdits.push_back(new LabelLineEdit(QFileDialog::LookIn, this));
    labelsLayout->addRow(tr("Look in label:"), m_labelLineEdits.back());
    m_labelLineEdits.push_back(new LabelLineEdit(QFileDialog::FileName, this));
    labelsLayout->addRow(tr("File name label:"), m_labelLineEdits.back());
    m_labelLineEdits.push_back(new LabelLineEdit(QFileDialog::FileType, this));
    labelsLayout->addRow(tr("File type label:"), m_labelLineEdits.back());
    m_labelLineEdits.push_back(new LabelLineEdit(QFileDialog::Accept, this));
    labelsLayout->addRow(tr("Accept label:"), m_labelLineEdits.back());
    m_labelLineEdits.push_back(new LabelLineEdit(QFileDialog::Reject, this));
    labelsLayout->addRow(tr("Reject label:"), m_labelLineEdits.back());

    // Buttons
    QGroupBox *buttonsGroupBox = new QGroupBox(tr("Show"));
    QGridLayout *buttonLayout = new QGridLayout(buttonsGroupBox);
    int row = 0;
    int column = 0;
    addButton(tr("Exec modal"), buttonLayout, row, column, this, SLOT(execModal()));
    addButton(tr("Show modal"), buttonLayout, row, column, this, SLOT(showModal()));
    m_deleteModalDialogButton =
        addButton(tr("Delete modal"), buttonLayout, row, column, this, SLOT(deleteModalDialog()));
    addButton(tr("Show non-modal"), buttonLayout, row, column, this, SLOT(showNonModal()));
    m_deleteNonModalDialogButton =
        addButton(tr("Delete non-modal"), buttonLayout, row, column, this, SLOT(deleteNonModalDialog()));
    row = 0;
    column++;
    addButton(tr("getOpenFileName"), buttonLayout, row, column, this, SLOT(getOpenFileName()));
    addButton(tr("getOpenFileNames"), buttonLayout, row, column, this, SLOT(getOpenFileNames()));
    addButton(tr("getSaveFileName"), buttonLayout, row, column, this, SLOT(getSaveFileName()));
    addButton(tr("getExistingDirectory"), buttonLayout, row, column, this, SLOT(getExistingDirectory()));
    addButton(tr("Restore defaults"), buttonLayout, row, column, this, SLOT(restoreDefaults()));

    // Main layout
    QGridLayout *gridLayout = new QGridLayout(this);
    gridLayout->addWidget(optionsGroupBox, 0, 0);
    gridLayout->addWidget(filesGroupBox, 0, 1);
    gridLayout->addWidget(labelsGroupBox, 1, 0);
    gridLayout->addWidget(buttonsGroupBox, 1, 1);

    enableDeleteModalDialogButton();
    enableDeleteNonModalDialogButton();
    restoreDefaults();
}

void FileDialogPanel::execModal()
{
    QFileDialog dialog(this);
    applySettings(&dialog);
    connect(&dialog, SIGNAL(accepted()), this, SLOT(accepted()));
    dialog.setWindowTitle(tr("Modal File Dialog Qt %1").arg(QLatin1String(QT_VERSION_STR)));
    dialog.exec();
}

void FileDialogPanel::showModal()
{
    if (m_modalDialog.isNull()) {
        static int  n = 0;
        m_modalDialog = new QFileDialog(this);
        m_modalDialog->setModal(true);
        connect(m_modalDialog.data(), SIGNAL(accepted()), this, SLOT(accepted()));
        m_modalDialog->setWindowTitle(tr("Modal File Dialog #%1 Qt %2")
                                      .arg(++n)
                                      .arg(QLatin1String(QT_VERSION_STR)));
        enableDeleteModalDialogButton();
    }
    applySettings(m_modalDialog);
    m_modalDialog->show();
}

void FileDialogPanel::showNonModal()
{
    if (m_nonModalDialog.isNull()) {
        static int  n = 0;
        m_nonModalDialog = new QFileDialog(this);
        connect(m_nonModalDialog.data(), SIGNAL(accepted()), this, SLOT(accepted()));
        m_nonModalDialog->setWindowTitle(tr("Non-Modal File Dialog #%1 Qt %2")
                                         .arg(++n)
                                         .arg(QLatin1String(QT_VERSION_STR)));
        enableDeleteNonModalDialogButton();
    }
    applySettings(m_nonModalDialog);
    m_nonModalDialog->show();
}

void FileDialogPanel::deleteNonModalDialog()
{
    if (!m_nonModalDialog.isNull())
        delete m_nonModalDialog;
    enableDeleteNonModalDialogButton();
}

void FileDialogPanel::deleteModalDialog()
{
    if (!m_modalDialog.isNull())
        delete m_modalDialog;
    enableDeleteModalDialogButton();
}

void FileDialogPanel::enableDeleteNonModalDialogButton()
{
    m_deleteNonModalDialogButton->setEnabled(!m_nonModalDialog.isNull());
}

void FileDialogPanel::enableDeleteModalDialogButton()
{
    m_deleteModalDialogButton->setEnabled(!m_modalDialog.isNull());
}


QString FileDialogPanel::filterString() const
{
    return m_nameFilters->toPlainText().trimmed().replace(QLatin1String("\n"), QLatin1String(";;"));
}

QFileDialog::Options FileDialogPanel::options() const
{
    QFileDialog::Options result;
    if (!m_nameFilterDetailsVisible->isChecked())
        result |= QFileDialog::HideNameFilterDetails;
    if (!m_resolveSymLinks->isChecked())
        result |= QFileDialog::DontResolveSymlinks;
    if (m_readOnly->isChecked())
        result |= QFileDialog::ReadOnly;
    if (!m_confirmOverWrite->isChecked())
        result |= QFileDialog::DontConfirmOverwrite;
    if (!m_native->isChecked())
        result |= QFileDialog::DontUseNativeDialog;
    return result;
}

void FileDialogPanel::getOpenFileNames()
{
    QString selectedFilter = m_selectedNameFilter->text().trimmed();
    const QStringList files =
        QFileDialog::getOpenFileNames(this, tr("getOpenFileNames Qt %1").arg(QLatin1String(QT_VERSION_STR)),
                                      m_directory->text(), filterString(), &selectedFilter, options());
    if (!files.isEmpty()) {
        QString result;
        QDebug(&result).nospace()
            << "Files: " << files
            << "\nName filter: " << selectedFilter;
        QMessageBox::information(this, tr("getOpenFileNames"), result, QMessageBox::Ok);
    }
}

void FileDialogPanel::getOpenFileName()
{
    QString selectedFilter = m_selectedNameFilter->text().trimmed();
    const QString file =
        QFileDialog::getOpenFileName(this, tr("getOpenFileName Qt %1").arg(QLatin1String(QT_VERSION_STR)),
                                      m_directory->text(), filterString(), &selectedFilter, options());
    if (!file.isEmpty()) {
        QString result;
        QDebug(&result).nospace()
            << "File: " << file
            << "\nName filter: " << selectedFilter;
        QMessageBox::information(this, tr("getOpenFileName"), result, QMessageBox::Ok);
    }
}

void FileDialogPanel::getSaveFileName()
{
    QString selectedFilter = m_selectedNameFilter->text().trimmed();
    const QString file =
        QFileDialog::getSaveFileName(this, tr("getSaveFileName Qt %1").arg(QLatin1String(QT_VERSION_STR)),
                                      m_directory->text(), filterString(), &selectedFilter, options());
    if (!file.isEmpty()) {
        QString result;
        QDebug(&result).nospace()
            << "File: " << file
            << "\nName filter: " << selectedFilter;
        QMessageBox::information(this, tr("getSaveFileNames"), result, QMessageBox::Ok);
    }
}

void FileDialogPanel::getExistingDirectory()
{
    const QString dir =
        QFileDialog::getExistingDirectory(this, tr("getExistingDirectory Qt %1").arg(QLatin1String(QT_VERSION_STR)),
                                          m_directory->text(), options() | QFileDialog::ShowDirsOnly);
    if (!dir.isEmpty())
        QMessageBox::information(this, tr("getExistingDirectory"), QLatin1String("Directory: ") + dir, QMessageBox::Ok);
}

void FileDialogPanel::restoreDefaults()
{
    QFileDialog d;
    setComboBoxValue(m_acceptMode, d.acceptMode());
    setComboBoxValue(m_fileMode, d.fileMode());
    setComboBoxValue(m_viewMode, d.viewMode());
    m_confirmOverWrite->setChecked(d.confirmOverwrite());
    m_nameFilterDetailsVisible->setChecked(d.isNameFilterDetailsVisible());
    m_resolveSymLinks->setChecked(d.resolveSymlinks());
    m_readOnly->setChecked(d.isReadOnly());
    m_native->setChecked(true);
    m_directory->setText(QDir::homePath());
    m_defaultSuffix->setText(QLatin1String("txt"));
    m_nameFilters->setPlainText(QLatin1String("Any files (*)\nImage files (*.png *.xpm *.jpg)\nText files (*.txt)"));
    m_selectedFileName->setText(QString());
    m_selectedNameFilter->setText(QString());
    foreach (LabelLineEdit *l, m_labelLineEdits)
        l->restoreDefault(&d);
}

void FileDialogPanel::applySettings(QFileDialog *d) const
{
    d->setAcceptMode(comboBoxValue<QFileDialog::AcceptMode>(m_acceptMode));
    d->setViewMode(comboBoxValue<QFileDialog::ViewMode>(m_viewMode));
    d->setFileMode(comboBoxValue<QFileDialog::FileMode>(m_fileMode));
    d->setOptions(options());
    d->setDefaultSuffix(m_defaultSuffix->text().trimmed());
    const QString directory = m_directory->text().trimmed();
    if (!directory.isEmpty())
        d->setDirectory(directory);
    const QString file = m_selectedFileName->text().trimmed();
    if (!file.isEmpty())
       d->selectFile(file);
    d->setNameFilters(m_nameFilters->toPlainText().trimmed().split(QLatin1Char('\n'), QString::SkipEmptyParts));
    const QString filter = m_selectedNameFilter->text().trimmed();
    if (!filter.isEmpty())
        d->selectNameFilter(filter);
    foreach (LabelLineEdit *l, m_labelLineEdits)
        l->apply(d);
}

void FileDialogPanel::accepted()
{
    const QFileDialog *d = qobject_cast<const QFileDialog *>(sender());
    Q_ASSERT(d);
    m_result.clear();
    QDebug(&m_result).nospace()
        << "Files: " << d->selectedFiles()
        << "\nDirectory: " << d->directory().absolutePath()
        << "\nName filter: " << d->selectedNameFilter();
    QTimer::singleShot(0, this, SLOT(showAcceptedResult())); // Avoid problems with the closing (modal) dialog as parent.
}

void FileDialogPanel::showAcceptedResult()
{
    QMessageBox::information(this, tr("File Dialog Accepted"), m_result, QMessageBox::Ok);
}

#include "filedialogpanel.moc"
