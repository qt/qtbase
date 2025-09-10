// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowdefs.h"
#include "qfontdialog.h"

#include "qfontdialog_p.h"

#include <qapplication.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qevent.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qstyle.h>
#include <qdialogbuttonbox.h>
#include <qheaderview.h>
#include <qlistview.h>
#include <qstringlistmodel.h>
#include <qvalidator.h>
#include <private/qfontdatabase_p.h>
#include <private/qdialog_p.h>
#include <private/qfont_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QFontListView : public QListView
{
    Q_OBJECT
public:
    QFontListView(QWidget *parent);
    inline QStringListModel *model() const {
        return static_cast<QStringListModel *>(QListView::model());
    }
    inline void setCurrentItem(int item) {
        QListView::setCurrentIndex(static_cast<QAbstractListModel*>(model())->index(item));
    }
    inline int currentItem() const {
        return QListView::currentIndex().row();
    }
    inline int count() const {
        return model()->rowCount();
    }
    inline QString currentText() const {
        int row = QListView::currentIndex().row();
        return row < 0 ? QString() : model()->stringList().at(row);
    }
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) override {
        QListView::currentChanged(current, previous);
        if (current.isValid())
            emit highlighted(current.row());
    }
    QString text(int i) const {
        return model()->stringList().at(i);
    }
signals:
    void highlighted(int);
};

QFontListView::QFontListView(QWidget *parent)
    : QListView(parent)
{
    setModel(new QStringListModel(parent));
    setEditTriggers(NoEditTriggers);
}

static const Qt::WindowFlags qfd_DefaultWindowFlags =
        Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint;

QFontDialogPrivate::QFontDialogPrivate()
    : writingSystem(QFontDatabase::Any),
      options(QFontDialogOptions::create())
{
}

QFontDialogPrivate::~QFontDialogPrivate()
{
}

/*!
  \class QFontDialog
  \ingroup standard-dialogs
  \inmodule QtWidgets

  \brief The QFontDialog class provides a dialog widget for selecting a font.

    A font dialog is created through one of the static getFont()
    functions.

  Examples:

  \snippet code/src_gui_dialogs_qfontdialog.cpp 0

    The dialog can also be used to set a widget's font directly:
  \snippet code/src_gui_dialogs_qfontdialog.cpp 1
  If the user clicks OK the font they chose will be used for myWidget,
  and if they click Cancel the original font is used.

  \image fusion-fontdialog.png A font dialog in the Fusion widget style.

  \sa QFont, QFontInfo, QFontMetrics, QColorDialog, QFileDialog,
      {Standard Dialogs Example}
*/

/*!
    Constructs a standard font dialog.

    Use setCurrentFont() to set the initial font attributes.

    The \a parent parameter is passed to the QDialog constructor.

    \sa getFont()
*/
QFontDialog::QFontDialog(QWidget *parent)
    : QDialog(*new QFontDialogPrivate, parent, qfd_DefaultWindowFlags)
{
    Q_D(QFontDialog);
    d->init();
}

/*!
    Constructs a standard font dialog with the given \a parent and specified
    \a initial font.
*/
QFontDialog::QFontDialog(const QFont &initial, QWidget *parent)
    : QFontDialog(parent)
{
    setCurrentFont(initial);
}

void QFontDialogPrivate::init()
{
    Q_Q(QFontDialog);

    q->setSizeGripEnabled(true);
    q->setWindowTitle(QFontDialog::tr("Select Font"));

    // grid
    familyEdit = new QLineEdit(q);
    familyEdit->setReadOnly(true);
    familyList = new QFontListView(q);
    familyEdit->setFocusProxy(familyList);

    familyAccel = new QLabel(q);
#ifndef QT_NO_SHORTCUT
    familyAccel->setBuddy(familyList);
#endif
    familyAccel->setIndent(2);

    styleEdit = new QLineEdit(q);
    styleEdit->setReadOnly(true);
    styleList = new QFontListView(q);
    styleEdit->setFocusProxy(styleList);

    styleAccel = new QLabel(q);
#ifndef QT_NO_SHORTCUT
    styleAccel->setBuddy(styleList);
#endif
    styleAccel->setIndent(2);

    sizeEdit = new QLineEdit(q);
    sizeEdit->setFocusPolicy(Qt::ClickFocus);
    QIntValidator *validator = new QIntValidator(1, 512, q);
    sizeEdit->setValidator(validator);
    sizeList = new QFontListView(q);

    sizeAccel = new QLabel(q);
#ifndef QT_NO_SHORTCUT
    sizeAccel->setBuddy(sizeEdit);
#endif
    sizeAccel->setIndent(2);

    // effects box
    effects = new QGroupBox(q);
    QVBoxLayout *vbox = new QVBoxLayout(effects);
    strikeout = new QCheckBox(effects);
    vbox->addWidget(strikeout);
    underline = new QCheckBox(effects);
    vbox->addWidget(underline);

    sample = new QGroupBox(q);
    QHBoxLayout *hbox = new QHBoxLayout(sample);
    sampleEdit = new QLineEdit(sample);
    sampleEdit->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored));
    sampleEdit->setAlignment(Qt::AlignCenter);
    // Note that the sample text is *not* translated with tr(), as the
    // characters used depend on the charset encoding.
    sampleEdit->setText("AaBbYyZz"_L1);
    hbox->addWidget(sampleEdit);

    writingSystemCombo = new QComboBox(q);

    writingSystemAccel = new QLabel(q);
#ifndef QT_NO_SHORTCUT
    writingSystemAccel->setBuddy(writingSystemCombo);
#endif
    writingSystemAccel->setIndent(2);

    size = 0;
    smoothScalable = false;

    QObjectPrivate::connect(writingSystemCombo, &QComboBox::activated,
                            this, &QFontDialogPrivate::writingSystemHighlighted);
    QObjectPrivate::connect(familyList, &QFontListView::highlighted,
                            this, &QFontDialogPrivate::familyHighlighted);
    QObjectPrivate::connect(styleList, &QFontListView::highlighted,
                            this, &QFontDialogPrivate::styleHighlighted);
    QObjectPrivate::connect(sizeList, &QFontListView::highlighted,
                            this, &QFontDialogPrivate::sizeHighlighted);
    QObjectPrivate::connect(sizeEdit, &QLineEdit::textChanged,
                            this, &QFontDialogPrivate::sizeChanged);

    QObjectPrivate::connect(strikeout, &QCheckBox::clicked,
                            this, &QFontDialogPrivate::updateSample);
    QObjectPrivate::connect(underline, &QCheckBox::clicked, this,
                            &QFontDialogPrivate::updateSample);

    for (int i = 0; i < QFontDatabase::WritingSystemsCount; ++i) {
        QFontDatabase::WritingSystem ws = QFontDatabase::WritingSystem(i);
        QString writingSystemName = QFontDatabase::writingSystemName(ws);
        if (writingSystemName.isEmpty())
            break;
        writingSystemCombo->addItem(writingSystemName);
    }

    updateFamilies();
    if (familyList->count() != 0) {
        familyList->setCurrentItem(0);
        sizeList->setCurrentItem(0);
    }

    // grid layout
    QGridLayout *mainGrid = new QGridLayout(q);

    int spacing = mainGrid->spacing();
    if (spacing >= 0) {     // uniform spacing
       mainGrid->setSpacing(0);

       mainGrid->setColumnMinimumWidth(1, spacing);
       mainGrid->setColumnMinimumWidth(3, spacing);

       int margin = 0;
       mainGrid->getContentsMargins(nullptr, nullptr, nullptr, &margin);

       mainGrid->setRowMinimumHeight(3, margin);
       mainGrid->setRowMinimumHeight(6, 2);
       mainGrid->setRowMinimumHeight(8, margin);
    }

    mainGrid->addWidget(familyAccel, 0, 0);
    mainGrid->addWidget(familyEdit, 1, 0);
    mainGrid->addWidget(familyList, 2, 0);

    mainGrid->addWidget(styleAccel, 0, 2);
    mainGrid->addWidget(styleEdit, 1, 2);
    mainGrid->addWidget(styleList, 2, 2);

    mainGrid->addWidget(sizeAccel, 0, 4);
    mainGrid->addWidget(sizeEdit, 1, 4);
    mainGrid->addWidget(sizeList, 2, 4);

    mainGrid->setColumnStretch(0, 38);
    mainGrid->setColumnStretch(2, 24);
    mainGrid->setColumnStretch(4, 10);

    mainGrid->addWidget(effects, 4, 0);

    mainGrid->addWidget(sample, 4, 2, 4, 3);

    mainGrid->addWidget(writingSystemAccel, 5, 0);
    mainGrid->addWidget(writingSystemCombo, 7, 0);

    buttonBox = new QDialogButtonBox(q);
    mainGrid->addWidget(buttonBox, 9, 0, 1, 5);

    QPushButton *button
            = static_cast<QPushButton *>(buttonBox->addButton(QDialogButtonBox::Ok));
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, q, &QDialog::accept);
    button->setDefault(true);

    buttonBox->addButton(QDialogButtonBox::Cancel);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, q, &QDialog::reject);

    q->resize(500, 360);

    sizeEdit->installEventFilter(q);
    familyList->installEventFilter(q);
    styleList->installEventFilter(q);
    sizeList->installEventFilter(q);

    familyList->setFocus();
    retranslateStrings();
    sampleEdit->setObjectName("qt_fontDialog_sampleEdit"_L1);
}

/*!
  \internal
 Destroys the font dialog and frees up its storage.
*/

QFontDialog::~QFontDialog()
{
}

/*!
  Executes a modal font dialog and returns a font.

  If the user clicks \uicontrol OK, the selected font is returned. If the user
  clicks \uicontrol Cancel, the \a initial font is returned.

  The dialog is constructed with the given \a parent and the options specified
  in \a options. \a title is shown as the window title of the dialog and  \a
  initial is the initially selected font. If the \a ok parameter is not-null,
  the value it refers to is set to true if the user clicks \uicontrol OK, and set to
  false if the user clicks \uicontrol Cancel.

  Examples:
  \snippet code/src_gui_dialogs_qfontdialog.cpp 2

    The dialog can also be used to set a widget's font directly:
  \snippet code/src_gui_dialogs_qfontdialog.cpp 3
  In this example, if the user clicks OK the font they chose will be
  used, and if they click Cancel the original font is used.
*/
QFont QFontDialog::getFont(bool *ok, const QFont &initial, QWidget *parent, const QString &title,
                           FontDialogOptions options)
{
    return QFontDialogPrivate::getFont(ok, initial, parent, title, options);
}

/*!
    \overload

  Executes a modal font dialog and returns a font.

  If the user clicks \uicontrol OK, the selected font is returned. If the user
  clicks \uicontrol Cancel, the Qt default font is returned.

  The dialog is constructed with the given \a parent.
  If the \a ok parameter is not-null, the value it refers to is set
  to true if the user clicks \uicontrol OK, and false if the user clicks
  \uicontrol Cancel.

  Example:
  \snippet code/src_gui_dialogs_qfontdialog.cpp 4
*/
QFont QFontDialog::getFont(bool *ok, QWidget *parent)
{
    QFont initial;
    return QFontDialogPrivate::getFont(ok, initial, parent, QString(), { });
}

QFont QFontDialogPrivate::getFont(bool *ok, const QFont &initial, QWidget *parent,
                                  const QString &title, QFontDialog::FontDialogOptions options)
{
    QAutoPointer<QFontDialog> dlg(new QFontDialog(parent));
    dlg->setOptions(options);
    dlg->setCurrentFont(initial);
    if (!title.isEmpty())
        dlg->setWindowTitle(title);

    int ret = (dlg->exec() || (options & QFontDialog::NoButtons));
    if (ok)
        *ok = !!ret;
    if (ret && bool(dlg)) {
        return dlg->selectedFont();
    } else {
        return initial;
    }
}

/*!
    \internal
    An event filter to make the Up, Down, PageUp and PageDown keys work
    correctly in the line edits. The source of the event is the object
    \a o and the event is \a e.
*/

bool QFontDialog::eventFilter(QObject *o , QEvent *e)
{
    Q_D(QFontDialog);
    if (e->type() == QEvent::KeyPress) {
        QKeyEvent *k = static_cast<QKeyEvent *>(e);
        if (o == d->sizeEdit &&
        (k->key() == Qt::Key_Up ||
             k->key() == Qt::Key_Down ||
         k->key() == Qt::Key_PageUp ||
         k->key() == Qt::Key_PageDown)) {

            int ci = d->sizeList->currentItem();
            QCoreApplication::sendEvent(d->sizeList, k);

            if (ci != d->sizeList->currentItem()
                    && style()->styleHint(QStyle::SH_FontDialog_SelectAssociatedText, nullptr, this))
                d->sizeEdit->selectAll();
            return true;
        } else if ((o == d->familyList || o == d->styleList) &&
                    (k->key() == Qt::Key_Return || k->key() == Qt::Key_Enter)) {
            k->accept();
        accept();
            return true;
        }
    } else if (e->type() == QEvent::FocusIn
               && style()->styleHint(QStyle::SH_FontDialog_SelectAssociatedText, nullptr, this)) {
        if (o == d->familyList)
            d->familyEdit->selectAll();
        else if (o == d->styleList)
            d->styleEdit->selectAll();
        else if (o == d->sizeList)
            d->sizeEdit->selectAll();
    } else if (e->type() == QEvent::MouseButtonPress && o == d->sizeList) {
            d->sizeEdit->setFocus();
    }
    return QDialog::eventFilter(o, e);
}

void QFontDialogPrivate::initHelper(QPlatformDialogHelper *h)
{
    Q_Q(QFontDialog);
    auto *fontDialogHelper = static_cast<QPlatformFontDialogHelper *>(h);
    fontDialogHelper->setOptions(options);
    fontDialogHelper->setCurrentFont(q->currentFont());
    QObject::connect(fontDialogHelper, &QPlatformFontDialogHelper::currentFontChanged,
                     q, &QFontDialog::currentFontChanged);
    QObject::connect(fontDialogHelper, &QPlatformFontDialogHelper::fontSelected,
                     q, &QFontDialog::fontSelected);
}

void QFontDialogPrivate::helperPrepareShow(QPlatformDialogHelper *)
{
    options->setWindowTitle(q_func()->windowTitle());
}

/*
    Updates the contents of the "font family" list box. This
    function can be reimplemented if you have special requirements.
*/

void QFontDialogPrivate::updateFamilies()
{
    Q_Q(QFontDialog);

    enum match_t { MATCH_NONE = 0, MATCH_LAST_RESORT = 1, MATCH_APP = 2, MATCH_FAMILY = 3 };

    const QFontDialog::FontDialogOptions scalableMask = (QFontDialog::ScalableFonts | QFontDialog::NonScalableFonts);
    const QFontDialog::FontDialogOptions spacingMask = (QFontDialog::ProportionalFonts | QFontDialog::MonospacedFonts);
    const QFontDialog::FontDialogOptions options = q->options();

    QStringList familyNames;
    const auto families = QFontDatabase::families(writingSystem);
    for (const QString &family : families) {
        if (QFontDatabase::isPrivateFamily(family))
            continue;

        if ((options & scalableMask) && (options & scalableMask) != scalableMask) {
            if (bool(options & QFontDialog::ScalableFonts) != QFontDatabase::isSmoothlyScalable(family))
                continue;
        }
        if ((options & spacingMask) && (options & spacingMask) != spacingMask) {
            if (bool(options & QFontDialog::MonospacedFonts) != QFontDatabase::isFixedPitch(family))
                continue;
        }
        familyNames << family;
    }

    familyList->model()->setStringList(familyNames);

    QString foundryName1, familyName1, foundryName2, familyName2;
    int bestFamilyMatch = -1;
    match_t bestFamilyType = MATCH_NONE;

    QFont f;

    // ##### do the right thing for a list of family names in the font.
    QFontDatabasePrivate::parseFontName(family, foundryName1, familyName1);

    QStringList::const_iterator it = familyNames.constBegin();
    int i = 0;
    for(; it != familyNames.constEnd(); ++it, ++i) {
        QFontDatabasePrivate::parseFontName(*it, foundryName2, familyName2);

        //try to match...
        if (familyName1 == familyName2) {
            bestFamilyType = MATCH_FAMILY;
            if (foundryName1 == foundryName2) {
                bestFamilyMatch = i;
                break;
            }
            if (bestFamilyMatch < MATCH_FAMILY)
                bestFamilyMatch = i;
        }

        //and try some fall backs
        match_t type = MATCH_NONE;
        if (bestFamilyType <= MATCH_NONE && familyName2 == "helvetica"_L1)
            type = MATCH_LAST_RESORT;
        if (bestFamilyType <= MATCH_LAST_RESORT && familyName2 == f.families().constFirst())
            type = MATCH_APP;
        // ### add fallback for writingSystem
        if (type != MATCH_NONE) {
            bestFamilyType = type;
            bestFamilyMatch = i;
        }
    }

    if (i != -1 && bestFamilyType != MATCH_NONE)
        familyList->setCurrentItem(bestFamilyMatch);
    else
        familyList->setCurrentItem(0);
    familyEdit->setText(familyList->currentText());
    if (q->style()->styleHint(QStyle::SH_FontDialog_SelectAssociatedText, nullptr, q)
            && familyList->hasFocus())
        familyEdit->selectAll();

    updateStyles();
}

/*
    Updates the contents of the "font style" list box. This
    function can be reimplemented if you have special requirements.
*/
void QFontDialogPrivate::updateStyles()
{
    Q_Q(QFontDialog);
    QStringList styles = QFontDatabase::styles(familyList->currentText());
    styleList->model()->setStringList(styles);

    if (styles.isEmpty()) {
        styleEdit->clear();
        smoothScalable = false;
    } else {
        if (!style.isEmpty()) {
            bool found = false;
            bool first = true;
            QString cstyle = style;

        redo:
            for (int i = 0; i < static_cast<int>(styleList->count()); i++) {
                if (cstyle == styleList->text(i)) {
                     styleList->setCurrentItem(i);
                     found = true;
                     break;
                 }
            }
            if (!found && first) {
                if (cstyle.contains("Italic"_L1)) {
                    cstyle.replace("Italic"_L1, "Oblique"_L1);
                    first = false;
                    goto redo;
                } else if (cstyle.contains("Oblique"_L1)) {
                    cstyle.replace("Oblique"_L1, "Italic"_L1);
                    first = false;
                    goto redo;
                } else if (cstyle.contains("Regular"_L1)) {
                    cstyle.replace("Regular"_L1, "Normal"_L1);
                    first = false;
                    goto redo;
                } else if (cstyle.contains("Normal"_L1)) {
                    cstyle.replace("Normal"_L1, "Regular"_L1);
                    first = false;
                    goto redo;
                }
            }
            if (!found)
                styleList->setCurrentItem(0);
        } else {
            styleList->setCurrentItem(0);
        }

        styleEdit->setText(styleList->currentText());
        if (q->style()->styleHint(QStyle::SH_FontDialog_SelectAssociatedText, nullptr, q)
                && styleList->hasFocus())
            styleEdit->selectAll();

        smoothScalable = QFontDatabase::isSmoothlyScalable(familyList->currentText(), styleList->currentText());
    }

    updateSizes();
}

/*!
    \internal
    Updates the contents of the "font size" list box. This
  function can be reimplemented if you have special requirements.
*/

void QFontDialogPrivate::updateSizes()
{
    Q_Q(QFontDialog);

    if (!familyList->currentText().isEmpty()) {
        QList<int> sizes = QFontDatabase::pointSizes(familyList->currentText(), styleList->currentText());

        int i = 0;
        int current = -1;
        QStringList str_sizes;
        str_sizes.reserve(sizes.size());
        for(QList<int>::const_iterator it = sizes.constBegin(); it != sizes.constEnd(); ++it) {
            str_sizes.append(QString::number(*it));
            if (current == -1 && *it == size)
                current = i;
            ++i;
        }
        sizeList->model()->setStringList(str_sizes);
        if (current != -1)
            sizeList->setCurrentItem(current);

        const QSignalBlocker blocker(sizeEdit);
        sizeEdit->setText((smoothScalable ? QString::number(size) : sizeList->currentText()));
        if (q->style()->styleHint(QStyle::SH_FontDialog_SelectAssociatedText, nullptr, q)
                && sizeList->hasFocus())
            sizeEdit->selectAll();
    } else {
        sizeEdit->clear();
    }

    updateSample();
}

void QFontDialogPrivate::updateSample()
{
    // compute new font
    int pSize = sizeEdit->text().toInt();
    QFont newFont(QFontDatabase::font(familyList->currentText(), style, pSize));
    newFont.setStrikeOut(strikeout->isChecked());
    newFont.setUnderline(underline->isChecked());

    if (familyList->currentText().isEmpty())
        sampleEdit->clear();

    updateSampleFont(newFont);
}

void QFontDialogPrivate::updateSampleFont(const QFont &newFont)
{
    Q_Q(QFontDialog);
    if (newFont != sampleEdit->font()) {
        sampleEdit->setFont(newFont);
        emit q->currentFontChanged(newFont);
    }
}

/*!
    \internal
*/
void QFontDialogPrivate::writingSystemHighlighted(int index)
{
    writingSystem = QFontDatabase::WritingSystem(index);
    sampleEdit->setText(QFontDatabase::writingSystemSample(writingSystem));
    updateFamilies();
}

/*!
    \internal
*/
void QFontDialogPrivate::familyHighlighted(int i)
{
    Q_Q(QFontDialog);
    family = familyList->text(i);
    familyEdit->setText(family);
    if (q->style()->styleHint(QStyle::SH_FontDialog_SelectAssociatedText, nullptr, q)
            && familyList->hasFocus())
        familyEdit->selectAll();

    updateStyles();
}


/*!
    \internal
*/

void QFontDialogPrivate::styleHighlighted(int index)
{
    Q_Q(QFontDialog);
    QString s = styleList->text(index);
    styleEdit->setText(s);
    if (q->style()->styleHint(QStyle::SH_FontDialog_SelectAssociatedText, nullptr, q)
            && styleList->hasFocus())
        styleEdit->selectAll();

    style = s;

    updateSizes();
}


/*!
    \internal
*/

void QFontDialogPrivate::sizeHighlighted(int index)
{
    Q_Q(QFontDialog);
    QString s = sizeList->text(index);
    sizeEdit->setText(s);
    if (q->style()->styleHint(QStyle::SH_FontDialog_SelectAssociatedText, nullptr, q)
            && sizeEdit->hasFocus())
        sizeEdit->selectAll();

    size = s.toInt();
    updateSample();
}

/*!
    \internal
    This slot is called if the user changes the font size.
    The size is passed in the \a s argument as a \e string.
*/

void QFontDialogPrivate::sizeChanged(const QString &s)
{
    // no need to check if the conversion is valid, since we have an QIntValidator in the size edit
    int size = s.toInt();
    if (this->size == size)
        return;

    this->size = size;
    if (sizeList->count() != 0) {
        int i;
        for (i = 0; i < sizeList->count() - 1; i++) {
            if (sizeList->text(i).toInt() >= this->size)
                break;
        }
        const QSignalBlocker blocker(sizeList);
        if (sizeList->text(i).toInt() == this->size)
            sizeList->setCurrentItem(i);
        else
            sizeList->clearSelection();
    }
    updateSample();
}

void QFontDialogPrivate::retranslateStrings()
{
    familyAccel->setText(QFontDialog::tr("&Font"));
    styleAccel->setText(QFontDialog::tr("Font st&yle"));
    sizeAccel->setText(QFontDialog::tr("&Size"));
    effects->setTitle(QFontDialog::tr("Effects"));
    strikeout->setText(QFontDialog::tr("Stri&keout"));
    underline->setText(QFontDialog::tr("&Underline"));
    sample->setTitle(QFontDialog::tr("Sample"));
    writingSystemAccel->setText(QFontDialog::tr("Wr&iting System"));
}

/*!
    \reimp
*/
void QFontDialog::changeEvent(QEvent *e)
{
    Q_D(QFontDialog);
    if (e->type() == QEvent::LanguageChange) {
        d->retranslateStrings();
    }
    QDialog::changeEvent(e);
}

/*!
    \property QFontDialog::currentFont
    \brief the current font of the dialog.
*/

/*!
    Sets the font highlighted in the QFontDialog to the given \a font.

    \sa selectedFont()
*/
void QFontDialog::setCurrentFont(const QFont &font)
{
    Q_D(QFontDialog);
    d->family = font.families().value(0);
    d->style = QFontDatabase::styleString(font);
    d->size = font.pointSize();
    if (d->size == -1) {
        QFontInfo fi(font);
        d->size = fi.pointSize();
    }
    d->strikeout->setChecked(font.strikeOut());
    d->underline->setChecked(font.underline());
    d->updateFamilies();

    if (d->nativeDialogInUse) {
        if (QPlatformFontDialogHelper *helper = d->platformFontDialogHelper())
            helper->setCurrentFont(font);
    }
}

/*!
    Returns the current font.

    \sa selectedFont()
*/
QFont QFontDialog::currentFont() const
{
    Q_D(const QFontDialog);

    if (d->nativeDialogInUse) {
        if (const QPlatformFontDialogHelper *helper = d->platformFontDialogHelper())
            return helper->currentFont();
    }
    return d->sampleEdit->font();
}

/*!
    Returns the font that the user selected by clicking the \uicontrol{OK}
    or equivalent button.

    \note This font is not always the same as the font held by the
    \l currentFont property since the user can choose different fonts
    before finally selecting the one to use.
*/
QFont QFontDialog::selectedFont() const
{
    Q_D(const QFontDialog);
    return d->selectedFont;
}

/*!
    \enum QFontDialog::FontDialogOption

    This enum specifies various options that affect the look and feel
    of a font dialog.

    For instance, it allows to specify which type of font should be
    displayed. If none are specified all fonts available will be listed.

    Note that the font filtering options might not be supported on some
    platforms (e.g. Mac). They are always supported by the non native
    dialog (used on Windows or Linux).

    \value NoButtons Don't display \uicontrol{OK} and \uicontrol{Cancel} buttons. (Useful for "live dialogs".)
    \value DontUseNativeDialog Use Qt's standard font dialog on the Mac instead of Apple's
                               native font panel.
    \value ScalableFonts Show scalable fonts
    \value NonScalableFonts Show non scalable fonts
    \value MonospacedFonts Show monospaced fonts
    \value ProportionalFonts Show proportional fonts

    \sa options, setOption(), testOption()
*/

/*!
    Sets the given \a option to be enabled if \a on is true;
    otherwise, clears the given \a option.

    \sa options, testOption()
*/
void QFontDialog::setOption(FontDialogOption option, bool on)
{
    const QFontDialog::FontDialogOptions previousOptions = options();
    if (!(previousOptions & option) != !on)
        setOptions(previousOptions ^ option);
}

/*!
    Returns \c true if the given \a option is enabled; otherwise, returns
    false.

    \sa options, setOption()
*/
bool QFontDialog::testOption(FontDialogOption option) const
{
    Q_D(const QFontDialog);
    return d->options->testOption(static_cast<QFontDialogOptions::FontDialogOption>(option));
}

/*!
    \property QFontDialog::options
    \brief the various options that affect the look and feel of the dialog

    By default, all options are disabled.

    Options should be set before showing the dialog. Setting them while the
    dialog is visible is not guaranteed to have an immediate effect on the
    dialog (depending on the option and on the platform).

    \sa setOption(), testOption()
*/
void QFontDialog::setOptions(FontDialogOptions options)
{
    Q_D(QFontDialog);

    if (QFontDialog::options() == options)
        return;

    d->options->setOptions(QFontDialogOptions::FontDialogOptions(int(options)));
    d->buttonBox->setVisible(!(options & NoButtons));
}

QFontDialog::FontDialogOptions QFontDialog::options() const
{
    Q_D(const QFontDialog);
    return QFontDialog::FontDialogOptions(int(d->options->options()));
}

/*!
    Opens the dialog and connects its fontSelected() signal to the slot specified
    by \a receiver and \a member.

    The signal will be disconnected from the slot when the dialog is closed.
*/
void QFontDialog::open(QObject *receiver, const char *member)
{
    Q_D(QFontDialog);
    connect(this, SIGNAL(fontSelected(QFont)), receiver, member);
    d->receiverToDisconnectOnClose = receiver;
    d->memberToDisconnectOnClose = member;
    QDialog::open();
}

/*!
    \fn void QFontDialog::currentFontChanged(const QFont &font)

    This signal is emitted when the current font is changed. The new font is
    specified in \a font.

    The signal is emitted while a user is selecting a font. Ultimately, the
    chosen font may differ from the font currently selected.

    \sa currentFont, fontSelected(), selectedFont()
*/

/*!
    \fn void QFontDialog::fontSelected(const QFont &font)

    This signal is emitted when a font has been selected. The selected font is
    specified in \a font.

    The signal is only emitted when a user has chosen the final font to be
    used. It is not emitted while the user is changing the current font in the
    font dialog.

    \sa selectedFont(), currentFontChanged(), currentFont
*/

/*!
    \reimp
*/
void QFontDialog::setVisible(bool visible)
{
    // will call QFontDialogPrivate::setVisible
    QDialog::setVisible(visible);
}

/*!
    \internal

    The implementation of QFontDialog::setVisible() has to live here so that the call
    to hide() in ~QDialog calls this function; it wouldn't call the override of
    QDialog::setVisible().
*/
void QFontDialogPrivate::setVisible(bool visible)
{
    // Don't use Q_Q here! This function is called from ~QDialog,
    // so Q_Q calling q_func() invokes undefined behavior (invalid cast in q_func()).
    const auto q = static_cast<QDialog *>(q_ptr);

    if (canBeNativeDialog())
        setNativeDialogVisible(visible);
    if (nativeDialogInUse) {
        // Set WA_DontShowOnScreen so that QDialog::setVisible(visible) below
        // updates the state correctly, but skips showing the non-native version:
        q->setAttribute(Qt::WA_DontShowOnScreen, true);
    } else {
        q->setAttribute(Qt::WA_DontShowOnScreen, false);
    }
    QDialogPrivate::setVisible(visible);
}

/*!
  Closes the dialog and sets its result code to \a result. If this dialog
  is shown with exec(), done() causes the local event loop to finish,
  and exec() to return \a result.

  \sa QDialog::done()
*/
void QFontDialog::done(int result)
{
    Q_D(QFontDialog);
    if (result == Accepted) {
        // We check if this is the same font we had before, if so we emit currentFontChanged
        QFont selectedFont = currentFont();
        if (selectedFont != d->selectedFont)
            emit(currentFontChanged(selectedFont));
        d->selectedFont = selectedFont;
        emit fontSelected(d->selectedFont);
    } else
        d->selectedFont = QFont();
    if (d->receiverToDisconnectOnClose) {
        disconnect(this, SIGNAL(fontSelected(QFont)),
                   d->receiverToDisconnectOnClose, d->memberToDisconnectOnClose);
        d->receiverToDisconnectOnClose = nullptr;
    }
    d->memberToDisconnectOnClose.clear();
    QDialog::done(result);
}

bool QFontDialogPrivate::canBeNativeDialog() const
{
    // Don't use Q_Q here! This function is called from ~QDialog,
    // so Q_Q calling q_func() invokes undefined behavior (invalid cast in q_func()).
    const QDialog * const q = static_cast<const QDialog*>(q_ptr);
    if (nativeDialogInUse)
        return true;
    if (QCoreApplication::testAttribute(Qt::AA_DontUseNativeDialogs)
        || q->testAttribute(Qt::WA_DontShowOnScreen)
        || (options->options() & QFontDialog::DontUseNativeDialog)) {
        return false;
    }

    return strcmp(QFontDialog::staticMetaObject.className(), q->metaObject()->className()) == 0;
}

QT_END_NAMESPACE

#include "qfontdialog.moc"
#include "moc_qfontdialog.cpp"
