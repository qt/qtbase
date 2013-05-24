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

#include "wizardpanel.h"

#include <QWizard>
#include <QWizardPage>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QRadioButton>
#include <QPushButton>
#include <QCheckBox>
#include <QButtonGroup>
#include <QSpacerItem>
#include <QGroupBox>
#include <QLabel>
#include <QStyle>
#include <QIcon>
#include <QImage>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QHash>

static QIcon coloredIcon(const Qt::GlobalColor color)
{
    QImage image(QSize(24, 24), QImage::Format_RGB32);
    image.fill(color);
    return QIcon(QPixmap::fromImage(image));
}

static QPixmap pixmapWithText(const QString &text, const QColor color)
{
    QFont font;
    QFontMetrics metric(font);
    QRect rectangle = metric.boundingRect(text);
    rectangle.setBottomRight(rectangle.bottomRight() + QPoint(20, 20));
    QImage image(rectangle.size(), QImage::Format_RGB32);
    image.fill(color);
    QPainter painter(&image);
    painter.setFont(font);
    painter.drawText(rectangle, Qt::AlignHCenter | Qt::AlignVCenter, text);
    return QPixmap::fromImage(image);
}

// A radio-group control for QWizard::WizardStyle.
class WizardStyleControl : public QGroupBox
{
    Q_OBJECT
public:
    WizardStyleControl(QWidget *parent = 0);

    void setWizardStyle(int style);
    QWizard::WizardStyle wizardStyle() const;

signals:
    void wizardStyleChanged(int);

private:
    QButtonGroup *m_group;
};

WizardStyleControl::WizardStyleControl(QWidget *parent)
    : QGroupBox(tr("Style"), parent)
    , m_group(new QButtonGroup(this))
{
    m_group->setExclusive(true);
    connect(m_group, SIGNAL(buttonClicked(int)), this, SIGNAL(wizardStyleChanged(int)));
    QVBoxLayout *vLayout = new QVBoxLayout(this);
    QRadioButton *radioButton = new QRadioButton(tr("None/OS Default"), this);
    m_group->addButton(radioButton, QWizard::NStyles);
    vLayout->addWidget(radioButton);
    radioButton = new QRadioButton(tr("ClassicStyle"), this);
    m_group->addButton(radioButton, QWizard::ClassicStyle);
    vLayout->addWidget(radioButton);
    radioButton = new QRadioButton(tr("ModernStyle"), this);
    m_group->addButton(radioButton, QWizard::ModernStyle);
    vLayout->addWidget(radioButton);
    radioButton = new QRadioButton(tr("MacStyle"), this);
    m_group->addButton(radioButton, QWizard::MacStyle);
    vLayout->addWidget(radioButton);
    radioButton = new QRadioButton(tr("AeroStyle"), this);
    m_group->addButton(radioButton, QWizard::AeroStyle);
    vLayout->addWidget(radioButton);
    vLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));

    setWizardStyle(style()->styleHint(QStyle::SH_WizardStyle));
}

QWizard::WizardStyle WizardStyleControl::wizardStyle() const
{
    return static_cast<QWizard::WizardStyle>(m_group->checkedId());
}

void WizardStyleControl::setWizardStyle(int wizardStyle)
{
    if (wizardStyle < 0 || wizardStyle > QWizard::NStyles)
        wizardStyle = QWizard::NStyles;
    QAbstractButton *button = m_group->button(wizardStyle);
    Q_ASSERT(button);
    const bool blocked = m_group->blockSignals(true);
    button->setChecked(true);
    m_group->blockSignals(blocked);
}

// A control with checkboxes for QWizard::WizardOption.
class WizardOptionsControl : public QGroupBox
{
public:
    explicit WizardOptionsControl(QWidget *parent = 0);

    QWizard::WizardOption wizardOptions() const;
    void setWizardOptions(int options);

private:
    typedef QHash<int, QCheckBox *> CheckBoxHash;

    void addCheckBox(QVBoxLayout *layout, int flag, const QString &title);

    CheckBoxHash m_checkBoxes;
};

WizardOptionsControl::WizardOptionsControl(QWidget *parent)
    : QGroupBox(tr("Options"), parent)
{
    QVBoxLayout *vLayout = new QVBoxLayout(this);
    addCheckBox(vLayout, QWizard::IndependentPages, QLatin1String("IndependentPages"));
    addCheckBox(vLayout, QWizard::IgnoreSubTitles, QLatin1String("IgnoreSubTitles"));
    addCheckBox(vLayout, QWizard::ExtendedWatermarkPixmap, QLatin1String("ExtendedWatermarkPixmap"));
    addCheckBox(vLayout, QWizard::NoDefaultButton, QLatin1String("NoDefaultButton"));
    addCheckBox(vLayout, QWizard::NoBackButtonOnStartPage, QLatin1String("NoBackButtonOnStartPage"));
    addCheckBox(vLayout, QWizard::NoBackButtonOnLastPage, QLatin1String("NoBackButtonOnLastPage"));
    addCheckBox(vLayout, QWizard::DisabledBackButtonOnLastPage, QLatin1String("DisabledBackButtonOnLastPage"));
    addCheckBox(vLayout, QWizard::HaveNextButtonOnLastPage, QLatin1String("HaveNextButtonOnLastPage"));
    addCheckBox(vLayout, QWizard::HaveFinishButtonOnEarlyPages, QLatin1String("HaveFinishButtonOnEarlyPages"));
    addCheckBox(vLayout, QWizard::NoCancelButton, QLatin1String("NoCancelButton"));
    addCheckBox(vLayout, QWizard::CancelButtonOnLeft, QLatin1String("CancelButtonOnLeft"));
    addCheckBox(vLayout, QWizard::HaveHelpButton, QLatin1String("HaveHelpButton"));
    addCheckBox(vLayout, QWizard::HelpButtonOnRight, QLatin1String("HelpButtonOnRight"));
    addCheckBox(vLayout, QWizard::HaveCustomButton1, QLatin1String("HaveCustomButton1"));
    addCheckBox(vLayout, QWizard::HaveCustomButton2, QLatin1String("HaveCustomButton2"));
    addCheckBox(vLayout, QWizard::HaveCustomButton3, QLatin1String("HaveCustomButton3"));
    vLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));
}

void WizardOptionsControl::addCheckBox(QVBoxLayout *layout, int flag, const QString &title)
{
    QCheckBox *checkBox = new QCheckBox(title, this);
    layout->addWidget(checkBox);
    m_checkBoxes.insert(flag, checkBox);
}

QWizard::WizardOption WizardOptionsControl::wizardOptions() const
{
    int result = 0;
    typedef CheckBoxHash::const_iterator ConstIterator;
    const ConstIterator cend = m_checkBoxes.constEnd();
    for (ConstIterator it = m_checkBoxes.constBegin(); it != cend; ++it)
        if (it.value()->isChecked())
            result |= it.key();
    return static_cast<QWizard::WizardOption>(result);
}

void WizardOptionsControl::setWizardOptions(int options)
{
    typedef CheckBoxHash::iterator Iterator;
    const Iterator end = m_checkBoxes.end();
    for (Iterator it = m_checkBoxes.begin(); it != end; ++it)
        it.value()->setChecked(options & it.key());
}

// A test wizard with a slot to change its style.
class Wizard : public QWizard {
    Q_OBJECT
public:
    explicit Wizard(QWidget *parent = 0, Qt::WindowFlags flags = 0);

public slots:
    void changeWizardStyle(int newStyle);
};

void Wizard::changeWizardStyle(int newStyle)
{
    if (newStyle >= 0 && newStyle < int(QWizard::NStyles))
        setWizardStyle(static_cast<QWizard::WizardStyle>(newStyle));
}

// A test wizard page with a WizardStyleControl.
class WizardPage : public QWizardPage
{
public:
    explicit WizardPage(const QString &title, QWidget *parent = 0);

    void initializePage();

private:
    WizardStyleControl *m_styleControl;
    bool m_firstTimeShown;
};

WizardPage::WizardPage(const QString &title, QWidget *parent)
    : QWizardPage(parent)
    , m_styleControl(new WizardStyleControl(this))
    , m_firstTimeShown(true)
{
    setTitle(title);
    setSubTitle(title + QLatin1String(" SubTitle"));
    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->addWidget(m_styleControl);
}

void WizardPage::initializePage()
{
    m_styleControl->setWizardStyle(wizard()->wizardStyle());
    if (m_firstTimeShown) {
        m_firstTimeShown = false;
        connect(m_styleControl, SIGNAL(wizardStyleChanged(int)),
                wizard(), SLOT(changeWizardStyle(int)));
    }
}

Wizard::Wizard(QWidget *parent, Qt::WindowFlags flags)
    : QWizard(parent, flags)
{
    setWindowIcon(coloredIcon(Qt::red));
    setWindowTitle(QLatin1String("Wizard ") + QLatin1String(QT_VERSION_STR));
    addPage(new WizardPage(tr("Page 1"), this));
    addPage(new WizardPage(tr("Page 2"), this));
    addPage(new WizardPage(tr("Page 3"), this));
}

// A dialog using a Wizard as child widget (emulating Qt Designer).
class WizardEmbeddingDialog : public QDialog {
public:
    explicit WizardEmbeddingDialog(QWidget *parent = 0);

    Wizard *wizard() const { return m_wizard; }

private:
    Wizard *m_wizard;
};

WizardEmbeddingDialog::WizardEmbeddingDialog(QWidget *parent)
    : QDialog(parent)
    , m_wizard(new Wizard)
{
    setWindowTitle(QString::fromLatin1("Dialog Embedding QWizard %1").arg(QT_VERSION_STR));
    QGridLayout *gridLayout = new QGridLayout(this);
    gridLayout->addWidget(new QLabel(tr("Above wizard")), 0, 0, 1, 3);
    gridLayout->addWidget(new QLabel(tr("Left of wizard")), 1, 0);
    m_wizard->setObjectName(QLatin1String("EmbeddedWizard"));
    m_wizard->setParent(this, Qt::Widget);
    gridLayout->addWidget(m_wizard, 1, 1);
    gridLayout->addWidget(new QLabel(tr("Right of wizard")), 1, 2);
    gridLayout->addWidget(new QLabel(tr("Below wizard")), 2, 0, 1, 3);
}

WizardPanel::WizardPanel(QWidget *parent)
    : QWidget(parent)
    , m_styleControl(new WizardStyleControl(this))
    , m_optionsControl(new WizardOptionsControl(this))
{
    {
        QWizard wizard;
        m_optionsControl->setWizardOptions(wizard.options());
        m_styleControl->setWizardStyle(wizard.wizardStyle());
    }

    QGridLayout *gridLayout = new QGridLayout(this);
    gridLayout->addWidget(m_optionsControl, 0, 0, 2, 1);
    gridLayout->addWidget(m_styleControl, 0, 1);
    QGroupBox *buttonGroupBox = new QGroupBox(this);
    QVBoxLayout *vLayout = new QVBoxLayout(buttonGroupBox);
    QPushButton *button = new QPushButton(tr("Show modal"), this);
    connect(button, SIGNAL(clicked()), this, SLOT(showModal()));
    vLayout->addWidget(button);
    button = new QPushButton(tr("Show non-modal"), this);
    connect(button, SIGNAL(clicked()), this, SLOT(showNonModal()));
    vLayout->addWidget(button);
    button = new QPushButton(tr("Show embedded"), this);
    button->setToolTip(tr("Test QWizard's behavior when used as a widget child."));
    connect(button, SIGNAL(clicked()), this, SLOT(showEmbedded()));
    vLayout->addWidget(button);
    vLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));
    gridLayout->addWidget(buttonGroupBox, 1, 1);
}

void WizardPanel::showModal()
{
    Wizard wizard(this);
    applyParameters(&wizard);
    wizard.exec();
}

void WizardPanel::showNonModal()
{
    Wizard *wizard = new Wizard(this);
    applyParameters(wizard);
    wizard->setModal(false);
    wizard->setAttribute(Qt::WA_DeleteOnClose);
    wizard->show();
}

void WizardPanel::showEmbedded()
{
    WizardEmbeddingDialog *dialog = new WizardEmbeddingDialog(this);
    applyParameters(dialog->wizard());
    dialog->setModal(false);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void WizardPanel::applyParameters(QWizard *wizard) const
{
    wizard->setWizardStyle(m_styleControl->wizardStyle());
    wizard->setOptions(m_optionsControl->wizardOptions());
    wizard->setPixmap(QWizard::WatermarkPixmap, pixmapWithText(QLatin1String("Watermark"), QColor(Qt::blue).lighter()));
    wizard->setPixmap(QWizard::LogoPixmap, pixmapWithText(QLatin1String("Logo"), Qt::green));
    wizard->setPixmap(QWizard::BannerPixmap, pixmapWithText(QLatin1String("Banner"), Qt::green));
    wizard->setPixmap(QWizard::BackgroundPixmap, pixmapWithText(QLatin1String("Background"), QColor(Qt::red).lighter()));
}

#include "wizardpanel.moc"
