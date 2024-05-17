// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "colordialogpanel.h"
#include "utils.h"

#include <QGroupBox>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QComboBox>
#include <QTimer>
#include <QDebug>

// SVG color keyword names provided by the World Wide Web Consortium
static inline QStringList svgColorNames()
{
    return QStringList()
        << "aliceblue" << "antiquewhite" << "aqua" << "aquamarine" << "azure" << "beige" << "bisque"
        << "black" << "blanchedalmond" << "blue" << "blueviolet" << "brown" << "burlywood" << "cadetblue"
        << "chartreuse" << "chocolate" << "coral" << "cornflowerblue" << "cornsilk" << "crimson" << "cyan"
        << "darkblue" << "darkcyan" << "darkgoldenrod" << "darkgray" << "darkgreen" << "darkgrey"
        << "darkkhaki" << "darkmagenta" << "darkolivegreen" << "darkorange" << "darkorchid" << "darkred"
        << "darksalmon" << "darkseagreen" << "darkslateblue" << "darkslategray" << "darkslategrey"
        << "darkturquoise" << "darkviolet" << "deeppink" << "deepskyblue" << "dimgray" << "dimgrey"
        << "dodgerblue" << "firebrick" << "floralwhite" << "forestgreen" << "fuchsia" << "gainsboro"
        << "ghostwhite" << "gold" << "goldenrod" << "gray" << "grey" << "green" << "greenyellow"
        << "honeydew" << "hotpink" << "indianred" << "indigo" << "ivory" << "khaki" << "lavender"
        << "lavenderblush" << "lawngreen" << "lemonchiffon" << "lightblue" << "lightcoral" << "lightcyan"
        << "lightgoldenrodyellow" << "lightgray" << "lightgreen" << "lightgrey" << "lightpink"
        << "lightsalmon" << "lightseagreen" << "lightskyblue" << "lightslategray" << "lightslategrey"
        << "lightsteelblue" << "lightyellow" << "lime" << "limegreen" << "linen" << "magenta"
        << "maroon" << "mediumaquamarine" << "mediumblue" << "mediumorchid" << "mediumpurple"
        << "mediumseagreen" << "mediumslateblue" << "mediumspringgreen" << "mediumturquoise"
        << "mediumvioletred" << "midnightblue" << "mintcream" << "mistyrose" << "moccasin"
        << "navajowhite" << "navy" << "oldlace" << "olive" << "olivedrab" << "orange" << "orangered"
        << "orchid" << "palegoldenrod" << "palegreen" << "paleturquoise" << "palevioletred"
        << "papayawhip" << "peachpuff" << "peru" << "pink" << "plum" << "powderblue" << "purple" << "red"
        << "rosybrown" << "royalblue" << "saddlebrown" << "salmon" << "sandybrown" << "seagreen"
        << "seashell" << "sienna" << "silver" << "skyblue" << "slateblue" << "slategray" << "slategrey"
        << "snow" << "springgreen" << "steelblue" << "tan" << "teal" << "thistle" << "tomato"
        << "turquoise" << "violet" << "wheat" << "white" << "whitesmoke" << "yellow" << "yellowgreen";
}

class ColorProxyModel : public QSortFilterProxyModel
{
public:
    ColorProxyModel(QObject *parent = nullptr) : QSortFilterProxyModel(parent)
    {
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (role == Qt::DisplayRole) {
            QString name = data(index, Qt::EditRole).toString();
            return tr("%1 (%2)").arg(name, QColor(name).name());
        }
        if (role == Qt::DecorationRole)
            return QColor(data(index, Qt::EditRole).toString());
        return QSortFilterProxyModel::data(index, role);
    }
};

ColorDialogPanel::ColorDialogPanel(QWidget *parent)
    : QWidget(parent)
    , m_colorComboBox(new QComboBox)
    , m_showAlphaChannel(new QCheckBox(tr("Show alpha channel")))
    , m_noButtons(new QCheckBox(tr("Don't display OK/Cancel buttons")))
    , m_dontUseNativeDialog(new QCheckBox(tr("Don't use native dialog")))
{
    // Options
    QGroupBox *optionsGroupBox = new QGroupBox(tr("Options"), this);
    QVBoxLayout *optionsLayout = new QVBoxLayout(optionsGroupBox);
    optionsLayout->addWidget(m_showAlphaChannel);
    optionsLayout->addWidget(m_noButtons);
    optionsLayout->addWidget(m_dontUseNativeDialog);

    // Color
    QGroupBox *colorGroupBox = new QGroupBox(tr("Color"), this);
    QVBoxLayout *colorLayout = new QVBoxLayout(colorGroupBox);
    colorLayout->addWidget(m_colorComboBox);
    m_colorComboBox->addItems(svgColorNames());
    m_colorComboBox->setEditable(true);

    QAbstractItemModel *sourceModel = m_colorComboBox->model();
    ColorProxyModel* proxyModel = new ColorProxyModel(m_colorComboBox);
    proxyModel->setSourceModel(sourceModel);
    sourceModel->setParent(proxyModel);
    m_colorComboBox->setModel(proxyModel);

    // Buttons
    QGroupBox *buttonsGroupBox = new QGroupBox(tr("Show"));
    QVBoxLayout *buttonsLayout = new QVBoxLayout(buttonsGroupBox);
    addButton(tr("Exec modal"), buttonsLayout, this, SLOT(execModal()));
    addButton(tr("Show application modal"), buttonsLayout,
              [this]() { showModal(Qt::ApplicationModal); });
    addButton(tr("Show window modal"), buttonsLayout, [this]() { showModal(Qt::WindowModal); });
    m_deleteModalDialogButton =
        addButton(tr("Delete modal"), buttonsLayout, this, SLOT(deleteModalDialog()));
    addButton(tr("Show non-modal"), buttonsLayout, this, SLOT(showNonModal()));
    m_deleteNonModalDialogButton =
        addButton(tr("Delete non-modal"), buttonsLayout, this, SLOT(deleteNonModalDialog()));
    addButton(tr("Restore defaults"), buttonsLayout, this, SLOT(restoreDefaults()));
    buttonsLayout->addStretch();

    // Main layout
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    QVBoxLayout *leftLayout = new QVBoxLayout;
    leftLayout->addWidget(optionsGroupBox);
    leftLayout->addWidget(colorGroupBox);
    leftLayout->addStretch();
    mainLayout->addLayout(leftLayout);
    mainLayout->addWidget(buttonsGroupBox);

    enableDeleteModalDialogButton();
    enableDeleteNonModalDialogButton();
    restoreDefaults();
}

void ColorDialogPanel::execModal()
{
    QColorDialog dialog(this);
    applySettings(&dialog);
    connect(&dialog, SIGNAL(accepted()), this, SLOT(accepted()));
    connect(&dialog, SIGNAL(rejected()), this, SLOT(rejected()));
    connect(&dialog, SIGNAL(currentColorChanged(const QColor&)), this, SLOT(currentColorChanged(const QColor&)));
    dialog.setWindowTitle(tr("Modal Color Dialog Qt %1").arg(QLatin1String(QT_VERSION_STR)));
    dialog.exec();
}

void ColorDialogPanel::showModal(Qt::WindowModality modality)
{
    if (m_modalDialog.isNull()) {
        static int  n = 0;
        m_modalDialog = new QColorDialog(this);
        m_modalDialog->setModal(true);
        connect(m_modalDialog.data(), SIGNAL(accepted()), this, SLOT(accepted()));
        connect(m_modalDialog.data(), SIGNAL(rejected()), this, SLOT(rejected()));
        connect(m_modalDialog.data(), SIGNAL(currentColorChanged(const QColor&)), this, SLOT(currentColorChanged(const QColor&)));
        m_modalDialog->setWindowTitle(tr("Modal Color Dialog #%1 Qt %2")
                                      .arg(++n)
                                      .arg(QLatin1String(QT_VERSION_STR)));
        enableDeleteModalDialogButton();
    }
    m_modalDialog->setWindowModality(modality);
    applySettings(m_modalDialog);
    m_modalDialog->show();
}

void ColorDialogPanel::showNonModal()
{
    if (m_nonModalDialog.isNull()) {
        static int  n = 0;
        m_nonModalDialog = new QColorDialog(this);
        connect(m_nonModalDialog.data(), SIGNAL(accepted()), this, SLOT(accepted()));
        connect(m_nonModalDialog.data(), SIGNAL(rejected()), this, SLOT(rejected()));
        connect(m_nonModalDialog.data(), SIGNAL(currentColorChanged(const QColor&)), this, SLOT(currentColorChanged(const QColor&)));
        m_nonModalDialog->setWindowTitle(tr("Non-Modal Color Dialog #%1 Qt %2")
                                         .arg(++n)
                                         .arg(QLatin1String(QT_VERSION_STR)));
        enableDeleteNonModalDialogButton();
    }
    applySettings(m_nonModalDialog);
    m_nonModalDialog->show();
}

void ColorDialogPanel::deleteNonModalDialog()
{
    if (!m_nonModalDialog.isNull())
        delete m_nonModalDialog;
    enableDeleteNonModalDialogButton();
}

void ColorDialogPanel::deleteModalDialog()
{
    if (!m_modalDialog.isNull())
        delete m_modalDialog;
    enableDeleteModalDialogButton();
}

void ColorDialogPanel::accepted()
{
    const QColorDialog *d = qobject_cast<const QColorDialog *>(sender());
    Q_ASSERT(d);
    m_result.clear();
    qDebug() << "Current color: " << d->currentColor()
             << "Selected color: " << d->selectedColor();
    QDebug(&m_result).nospace()
        << "Current color: " << d->currentColor()
        << "\nSelected color: " << d->selectedColor();
    QTimer::singleShot(0, this, SLOT(showAcceptedResult())); // Avoid problems with the closing (modal) dialog as parent.
}

void ColorDialogPanel::rejected()
{
    qDebug() << "rejected";
}

void ColorDialogPanel::currentColorChanged(const QColor &color)
{
    qDebug() << color;
}

void ColorDialogPanel::showAcceptedResult()
{
    QMessageBox::information(this, tr("Color Dialog Accepted"), m_result, QMessageBox::Ok);
}

void ColorDialogPanel::restoreDefaults()
{
    QColorDialog d;
    m_showAlphaChannel->setChecked(d.testOption(QColorDialog::ShowAlphaChannel));
    m_noButtons->setChecked(d.testOption(QColorDialog::NoButtons));
    m_dontUseNativeDialog->setChecked(d.testOption(QColorDialog::DontUseNativeDialog));
}

void ColorDialogPanel::enableDeleteNonModalDialogButton()
{
    m_deleteNonModalDialogButton->setEnabled(!m_nonModalDialog.isNull());
}

void ColorDialogPanel::enableDeleteModalDialogButton()
{
    m_deleteModalDialogButton->setEnabled(!m_modalDialog.isNull());
}

void ColorDialogPanel::applySettings(QColorDialog *d) const
{
    d->setOption(QColorDialog::ShowAlphaChannel, m_showAlphaChannel->isChecked());
    d->setOption(QColorDialog::NoButtons, m_noButtons->isChecked());
    d->setOption(QColorDialog::DontUseNativeDialog, m_dontUseNativeDialog->isChecked());
    d->setCurrentColor(QColor(m_colorComboBox->itemData(m_colorComboBox->currentIndex(), Qt::EditRole).toString()));
}
