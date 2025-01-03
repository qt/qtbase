// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qprintpreviewdialog.h"
#include "qprintpreviewwidget.h"
#include <private/qprinter_p.h>
#include "qprintdialog.h"

#include <QtGui/qaction.h>
#include <QtGui/qactiongroup.h>
#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qlineedit.h>
#include <QtPrintSupport/qpagesetupdialog.h>
#include <QtPrintSupport/qprinter.h>
#include <QtWidgets/qstyle.h>
#include <QtWidgets/qtoolbutton.h>
#include <QtGui/qvalidator.h>
#if QT_CONFIG(filedialog)
#include <QtWidgets/qfiledialog.h>
#endif
#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/qtoolbar.h>
#include <QtCore/QCoreApplication>

#include "private/qdialog_p.h"

#include <QtWidgets/qformlayout.h>
#include <QtWidgets/qlabel.h>

static void _q_ppd_initResources()
{
    static bool resourcesInitialized = false;
    if (!resourcesInitialized) {
        Q_INIT_RESOURCE(qprintdialog);
        resourcesInitialized = true;
    }
}

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace {
class QPrintPreviewMainWindow : public QMainWindow
{
public:
    QPrintPreviewMainWindow(QWidget *parent) : QMainWindow(parent) {}
    QMenu *createPopupMenu() override { return nullptr; }
};

class ZoomFactorValidator : public QDoubleValidator
{
public:
    ZoomFactorValidator(QObject* parent)
        : QDoubleValidator(parent) {}
    ZoomFactorValidator(qreal bottom, qreal top, int decimals, QObject *parent)
        : QDoubleValidator(bottom, top, decimals, parent) {}

    State validate(QString &input, int &pos) const override
    {
        bool replacePercent = false;
        if (input.endsWith(u'%')) {
            input = input.left(input.size() - 1);
            replacePercent = true;
        }
        State state = QDoubleValidator::validate(input, pos);
        if (replacePercent)
            input += u'%';
        const int num_size = 4;
        if (state == Intermediate) {
            int i = input.indexOf(QLocale::system().decimalPoint());
            if ((i == -1 && input.size() > num_size)
                || (i != -1 && i > num_size))
                return Invalid;
        }
        return state;
    }
};

class LineEdit : public QLineEdit
{
    Q_OBJECT
public:
    LineEdit(QWidget* parent = nullptr)
        : QLineEdit(parent)
    {
        setContextMenuPolicy(Qt::NoContextMenu);
        connect(this, &LineEdit::returnPressed, this, &LineEdit::handleReturnPressed);
    }

protected:
    void focusInEvent(QFocusEvent *e) override
    {
        origText = text();
        QLineEdit::focusInEvent(e);
    }

    void focusOutEvent(QFocusEvent *e) override
    {
        if (isModified() && !hasAcceptableInput())
            setText(origText);
        QLineEdit::focusOutEvent(e);
    }

private slots:
    void handleReturnPressed()
    {
        origText = text();
    }

private:
    QString origText;
};
} // anonymous namespace

class QPrintPreviewDialogPrivate : public QDialogPrivate
{
    Q_DECLARE_PUBLIC(QPrintPreviewDialog)
public:
    QPrintPreviewDialogPrivate()
        : printDialog(nullptr), pageSetupDialog(nullptr),
          ownPrinter(false), initialized(false)  {}

    // private slots
    void _q_fit(QAction *action);
    void _q_zoomIn();
    void _q_zoomOut();
    void _q_navigate(QAction *action);
    void _q_setMode(QAction *action);
    void _q_pageNumEdited();
    void _q_print();
    void _q_pageSetup();
    void _q_previewChanged();
    void _q_zoomFactorChanged();

    void init(QPrinter *printer = nullptr);
    void populateScene();
    void layoutPages();
    void setupActions();
    void updateNavActions();
    void setFitting(bool on);
    bool isFitting();
    void updatePageNumLabel();
    void updateZoomFactor();

    QPrintDialog *printDialog;
    QPageSetupDialog *pageSetupDialog;
    QPrintPreviewWidget *preview;
    QPrinter *printer;
    bool ownPrinter;
    bool initialized;

    // widgets:
    QLineEdit *pageNumEdit;
    QLabel *pageNumLabel;
    QComboBox *zoomFactor;

    // actions:
    QActionGroup* navGroup;
    QAction *nextPageAction;
    QAction *prevPageAction;
    QAction *firstPageAction;
    QAction *lastPageAction;

    QActionGroup* fitGroup;
    QAction *fitWidthAction;
    QAction *fitPageAction;

    QActionGroup* zoomGroup;
    QAction *zoomInAction;
    QAction *zoomOutAction;

    QActionGroup* orientationGroup;
    QAction *portraitAction;
    QAction *landscapeAction;

    QActionGroup* modeGroup;
    QAction *singleModeAction;
    QAction *facingModeAction;
    QAction *overviewModeAction;

    QActionGroup *printerGroup;
    QAction *printAction;
    QAction *pageSetupAction;

    QPointer<QObject> receiverToDisconnectOnClose;
    QByteArray memberToDisconnectOnClose;
};

void QPrintPreviewDialogPrivate::init(QPrinter *_printer)
{
    Q_Q(QPrintPreviewDialog);

    _q_ppd_initResources();

    if (_printer) {
        preview = new QPrintPreviewWidget(_printer, q);
        printer = _printer;
    } else {
        ownPrinter = true;
        printer = new QPrinter;
        preview = new QPrintPreviewWidget(printer, q);
    }
    QObject::connect(preview, SIGNAL(paintRequested(QPrinter*)), q, SIGNAL(paintRequested(QPrinter*)));
    QObject::connect(preview, SIGNAL(previewChanged()), q, SLOT(_q_previewChanged()));
    setupActions();

    pageNumEdit = new LineEdit;
    pageNumEdit->setAlignment(Qt::AlignRight);
    pageNumEdit->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    pageNumLabel = new QLabel;
    QObject::connect(pageNumEdit, SIGNAL(editingFinished()), q, SLOT(_q_pageNumEdited()));

    zoomFactor = new QComboBox;
    zoomFactor->setEditable(true);
    zoomFactor->setMinimumContentsLength(7);
    zoomFactor->setInsertPolicy(QComboBox::NoInsert);
    LineEdit *zoomEditor = new LineEdit;
    zoomEditor->setValidator(new ZoomFactorValidator(1, 1000, 1, zoomEditor));
    zoomFactor->setLineEdit(zoomEditor);
    static const short factorsX2[] = { 25, 50, 100, 200, 250, 300, 400, 800, 1600 };
    for (auto factorX2 : factorsX2)
        zoomFactor->addItem(QPrintPreviewDialog::tr("%1%").arg(factorX2 / 2.0));
    QObject::connect(zoomFactor->lineEdit(), SIGNAL(editingFinished()),
                     q, SLOT(_q_zoomFactorChanged()));
    QObject::connect(zoomFactor, SIGNAL(currentIndexChanged(int)),
                     q, SLOT(_q_zoomFactorChanged()));

    QPrintPreviewMainWindow *mw = new QPrintPreviewMainWindow(q);
    QToolBar *toolbar = new QToolBar(mw);
    toolbar->addAction(fitWidthAction);
    toolbar->addAction(fitPageAction);
    toolbar->addSeparator();
    toolbar->addWidget(zoomFactor);
    toolbar->addAction(zoomOutAction);
    toolbar->addAction(zoomInAction);
    toolbar->addSeparator();
    toolbar->addAction(portraitAction);
    toolbar->addAction(landscapeAction);
    toolbar->addSeparator();
    toolbar->addAction(firstPageAction);
    toolbar->addAction(prevPageAction);

    // this is to ensure the label text and the editor text are
    // aligned in all styles - the extra QVBoxLayout is a workaround
    // for bug in QFormLayout
    QWidget *pageEdit = new QWidget(toolbar);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    vboxLayout->setContentsMargins(0, 0, 0, 0);
#ifdef Q_OS_MAC
    // We query the widgets about their size and then we fix the size.
    // This should do the trick for the laying out part...
    QSize pageNumEditSize, pageNumLabelSize;
    pageNumEditSize = pageNumEdit->minimumSizeHint();
    pageNumLabelSize = pageNumLabel->minimumSizeHint();
    pageNumEdit->resize(pageNumEditSize);
    pageNumLabel->resize(pageNumLabelSize);
#endif
    QFormLayout *formLayout = new QFormLayout;
#ifdef Q_OS_MAC
    // We have to change the growth policy in Mac.
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
#endif
    formLayout->setWidget(0, QFormLayout::LabelRole, pageNumEdit);
    formLayout->setWidget(0, QFormLayout::FieldRole, pageNumLabel);
    vboxLayout->addLayout(formLayout);
    vboxLayout->setAlignment(Qt::AlignVCenter);
    pageEdit->setLayout(vboxLayout);
    toolbar->addWidget(pageEdit);

    toolbar->addAction(nextPageAction);
    toolbar->addAction(lastPageAction);
    toolbar->addSeparator();
    toolbar->addAction(singleModeAction);
    toolbar->addAction(facingModeAction);
    toolbar->addAction(overviewModeAction);
    toolbar->addSeparator();
    toolbar->addAction(pageSetupAction);
    toolbar->addAction(printAction);

    // Cannot use the actions' triggered signal here, since it doesn't autorepeat
    QToolButton *zoomInButton = static_cast<QToolButton *>(toolbar->widgetForAction(zoomInAction));
    QToolButton *zoomOutButton = static_cast<QToolButton *>(toolbar->widgetForAction(zoomOutAction));
    zoomInButton->setAutoRepeat(true);
    zoomInButton->setAutoRepeatInterval(200);
    zoomInButton->setAutoRepeatDelay(200);
    zoomOutButton->setAutoRepeat(true);
    zoomOutButton->setAutoRepeatInterval(200);
    zoomOutButton->setAutoRepeatDelay(200);
    QObject::connect(zoomInButton, SIGNAL(clicked()), q, SLOT(_q_zoomIn()));
    QObject::connect(zoomOutButton, SIGNAL(clicked()), q, SLOT(_q_zoomOut()));

    mw->addToolBar(toolbar);
    mw->setCentralWidget(preview);
    // QMainWindows are always created as top levels, force it to be a
    // plain widget
    mw->setParent(q, Qt::Widget);

    QVBoxLayout *topLayout = new QVBoxLayout;
    topLayout->addWidget(mw);
    topLayout->setContentsMargins(0, 0, 0, 0);
    q->setLayout(topLayout);

    QString caption = QCoreApplication::translate("QPrintPreviewDialog", "Print Preview");
    if (!printer->docName().isEmpty())
        caption += ": "_L1 + printer->docName();
    q->setWindowTitle(caption);

    if (!printer->isValid()
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
        || printer->outputFormat() != QPrinter::NativeFormat
#endif
        )
        pageSetupAction->setEnabled(false);
    preview->setFocus();
}

static inline void qt_setupActionIcon(QAction *action, QLatin1StringView name)
{
    const auto imagePrefix = ":/qt-project.org/dialogs/qprintpreviewdialog/images/"_L1;
    QIcon icon = QIcon::fromTheme(name);
    icon.addFile(imagePrefix + name + "-24.png"_L1, QSize(24, 24));
    icon.addFile(imagePrefix + name + "-32.png"_L1, QSize(32, 32));
    action->setIcon(icon);
}

void QPrintPreviewDialogPrivate::setupActions()
{
    Q_Q(QPrintPreviewDialog);

    // Navigation
    navGroup = new QActionGroup(q);
    navGroup->setExclusive(false);
    nextPageAction = navGroup->addAction(QCoreApplication::translate("QPrintPreviewDialog", "Next page"));
    prevPageAction = navGroup->addAction(QCoreApplication::translate("QPrintPreviewDialog", "Previous page"));
    firstPageAction = navGroup->addAction(QCoreApplication::translate("QPrintPreviewDialog", "First page"));
    lastPageAction = navGroup->addAction(QCoreApplication::translate("QPrintPreviewDialog", "Last page"));
    qt_setupActionIcon(nextPageAction, "go-next"_L1);
    qt_setupActionIcon(prevPageAction, "go-previous"_L1);
    qt_setupActionIcon(firstPageAction, "go-first"_L1);
    qt_setupActionIcon(lastPageAction, "go-last"_L1);
    QObject::connect(navGroup, SIGNAL(triggered(QAction*)), q, SLOT(_q_navigate(QAction*)));


    fitGroup = new QActionGroup(q);
    fitWidthAction = fitGroup->addAction(QCoreApplication::translate("QPrintPreviewDialog", "Fit width"));
    fitPageAction = fitGroup->addAction(QCoreApplication::translate("QPrintPreviewDialog", "Fit page"));
    fitWidthAction->setObjectName("fitWidthAction"_L1);
    fitPageAction->setObjectName("fitPageAction"_L1);
    fitWidthAction->setCheckable(true);
    fitPageAction->setCheckable(true);
    qt_setupActionIcon(fitWidthAction, "zoom-fit-width"_L1);
    qt_setupActionIcon(fitPageAction, "zoom-fit-page"_L1);
    QObject::connect(fitGroup, SIGNAL(triggered(QAction*)), q, SLOT(_q_fit(QAction*)));

    // Zoom
    zoomGroup = new QActionGroup(q);
    zoomInAction = zoomGroup->addAction(QCoreApplication::translate("QPrintPreviewDialog", "Zoom in"));
    zoomOutAction = zoomGroup->addAction(QCoreApplication::translate("QPrintPreviewDialog", "Zoom out"));
    qt_setupActionIcon(zoomInAction, "zoom-in"_L1);
    qt_setupActionIcon(zoomOutAction, "zoom-out"_L1);

    // Portrait/Landscape
    orientationGroup = new QActionGroup(q);
    portraitAction = orientationGroup->addAction(QCoreApplication::translate("QPrintPreviewDialog", "Portrait"));
    landscapeAction = orientationGroup->addAction(QCoreApplication::translate("QPrintPreviewDialog", "Landscape"));
    portraitAction->setCheckable(true);
    landscapeAction->setCheckable(true);
    qt_setupActionIcon(portraitAction, "layout-portrait"_L1);
    qt_setupActionIcon(landscapeAction, "layout-landscape"_L1);
    QObject::connect(portraitAction, SIGNAL(triggered(bool)), preview, SLOT(setPortraitOrientation()));
    QObject::connect(landscapeAction, SIGNAL(triggered(bool)), preview, SLOT(setLandscapeOrientation()));

    // Display mode
    modeGroup = new QActionGroup(q);
    singleModeAction = modeGroup->addAction(QCoreApplication::translate("QPrintPreviewDialog", "Show single page"));
    facingModeAction = modeGroup->addAction(QCoreApplication::translate("QPrintPreviewDialog", "Show facing pages"));
    overviewModeAction = modeGroup->addAction(QCoreApplication::translate("QPrintPreviewDialog", "Show overview of all pages"));
    qt_setupActionIcon(singleModeAction, "view-pages-single"_L1);
    qt_setupActionIcon(facingModeAction, "view-pages-facing"_L1);
    qt_setupActionIcon(overviewModeAction, "view-pages-overview"_L1);
    singleModeAction->setObjectName("singleModeAction"_L1);
    facingModeAction->setObjectName("facingModeAction"_L1);
    overviewModeAction->setObjectName("overviewModeAction"_L1);

    singleModeAction->setCheckable(true);
    facingModeAction->setCheckable(true);
    overviewModeAction->setCheckable(true);
    QObject::connect(modeGroup, SIGNAL(triggered(QAction*)), q, SLOT(_q_setMode(QAction*)));

    // Print
    printerGroup = new QActionGroup(q);
    printAction = printerGroup->addAction(QCoreApplication::translate("QPrintPreviewDialog", "Print"));
    pageSetupAction = printerGroup->addAction(QCoreApplication::translate("QPrintPreviewDialog", "Page setup"));
    qt_setupActionIcon(printAction, "printer"_L1);
    qt_setupActionIcon(pageSetupAction, "page-setup"_L1);
    QObject::connect(printAction, SIGNAL(triggered(bool)), q, SLOT(_q_print()));
    QObject::connect(pageSetupAction, SIGNAL(triggered(bool)), q, SLOT(_q_pageSetup()));

    // Initial state:
    fitPageAction->setChecked(true);
    singleModeAction->setChecked(true);
    if (preview->orientation() == QPageLayout::Portrait)
        portraitAction->setChecked(true);
    else
        landscapeAction->setChecked(true);
}


bool QPrintPreviewDialogPrivate::isFitting()
{
    return (fitGroup->isExclusive()
            && (fitWidthAction->isChecked() || fitPageAction->isChecked()));
}


void QPrintPreviewDialogPrivate::setFitting(bool on)
{
    if (isFitting() == on)
        return;
    fitGroup->setExclusive(on);
    if (on) {
        QAction* action = fitWidthAction->isChecked() ? fitWidthAction : fitPageAction;
        action->setChecked(true);
        if (fitGroup->checkedAction() != action) {
            // work around exclusitivity problem
            fitGroup->removeAction(action);
            fitGroup->addAction(action);
        }
    } else {
        fitWidthAction->setChecked(false);
        fitPageAction->setChecked(false);
    }
}

void QPrintPreviewDialogPrivate::updateNavActions()
{
    int curPage = preview->currentPage();
    int numPages = preview->pageCount();
    nextPageAction->setEnabled(curPage < numPages);
    prevPageAction->setEnabled(curPage > 1);
    firstPageAction->setEnabled(curPage > 1);
    lastPageAction->setEnabled(curPage < numPages);
    pageNumEdit->setText(QString::number(curPage));
}

void QPrintPreviewDialogPrivate::updatePageNumLabel()
{
    Q_Q(QPrintPreviewDialog);

    int numPages = preview->pageCount();
    int maxChars = QString::number(numPages).size();
    pageNumLabel->setText(QString::fromLatin1("/ %1").arg(numPages));
    int cyphersWidth = q->fontMetrics().horizontalAdvance(QString().fill(u'8', maxChars));
    int maxWidth = pageNumEdit->minimumSizeHint().width() + cyphersWidth;
    pageNumEdit->setMinimumWidth(maxWidth);
    pageNumEdit->setMaximumWidth(maxWidth);
    pageNumEdit->setValidator(new QIntValidator(1, numPages, pageNumEdit));
    // any old one will be deleted later along with its parent pageNumEdit
}

void QPrintPreviewDialogPrivate::updateZoomFactor()
{
    zoomFactor->lineEdit()->setText(QString::asprintf("%.1f%%", preview->zoomFactor()*100));
}

void QPrintPreviewDialogPrivate::_q_fit(QAction* action)
{
    setFitting(true);
    if (action == fitPageAction)
        preview->fitInView();
    else
        preview->fitToWidth();
}

void QPrintPreviewDialogPrivate::_q_zoomIn()
{
    setFitting(false);
    preview->zoomIn();
    updateZoomFactor();
}

void QPrintPreviewDialogPrivate::_q_zoomOut()
{
    setFitting(false);
    preview->zoomOut();
    updateZoomFactor();
}

void QPrintPreviewDialogPrivate::_q_pageNumEdited()
{
    bool ok = false;
    int res = pageNumEdit->text().toInt(&ok);
    if (ok)
        preview->setCurrentPage(res);
}

void QPrintPreviewDialogPrivate::_q_navigate(QAction* action)
{
    int curPage = preview->currentPage();
    if (action == prevPageAction)
        preview->setCurrentPage(curPage - 1);
    else if (action == nextPageAction)
        preview->setCurrentPage(curPage + 1);
    else if (action == firstPageAction)
        preview->setCurrentPage(1);
    else if (action == lastPageAction)
        preview->setCurrentPage(preview->pageCount());
    updateNavActions();
}

void QPrintPreviewDialogPrivate::_q_setMode(QAction* action)
{
    if (action == overviewModeAction) {
        preview->setViewMode(QPrintPreviewWidget::AllPagesView);
        setFitting(false);
        fitGroup->setEnabled(false);
        navGroup->setEnabled(false);
        pageNumEdit->setEnabled(false);
        pageNumLabel->setEnabled(false);
    } else if (action == facingModeAction) {
        preview->setViewMode(QPrintPreviewWidget::FacingPagesView);
    } else {
        preview->setViewMode(QPrintPreviewWidget::SinglePageView);
    }
    if (action == facingModeAction || action == singleModeAction) {
        fitGroup->setEnabled(true);
        navGroup->setEnabled(true);
        pageNumEdit->setEnabled(true);
        pageNumLabel->setEnabled(true);
        setFitting(true);
    }
}

void QPrintPreviewDialogPrivate::_q_print()
{
    Q_Q(QPrintPreviewDialog);

#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    if (printer->outputFormat() != QPrinter::NativeFormat) {
        QString title = QCoreApplication::translate("QPrintPreviewDialog", "Export to PDF");
        QString suffix = ".pdf"_L1;
        QString fileName;
#if QT_CONFIG(filedialog)
        fileName = QFileDialog::getSaveFileName(q, title, printer->outputFileName(), u'*' + suffix);
#endif
        if (!fileName.isEmpty()) {
            if (QFileInfo(fileName).suffix().isEmpty())
                fileName.append(suffix);
            printer->setOutputFileName(fileName);
        }
        if (!printer->outputFileName().isEmpty())
            preview->print();
        q->accept();
        return;
    }
#endif

    if (!printDialog)
        printDialog = new QPrintDialog(printer, q);
    if (printDialog->exec() == QDialog::Accepted) {
        preview->print();
        q->accept();
    }
}

void QPrintPreviewDialogPrivate::_q_pageSetup()
{
    Q_Q(QPrintPreviewDialog);

    if (!pageSetupDialog)
        pageSetupDialog = new QPageSetupDialog(printer, q);

    if (pageSetupDialog->exec() == QDialog::Accepted) {
        // update possible orientation changes
        if (preview->orientation() == QPageLayout::Portrait) {
            portraitAction->setChecked(true);
            preview->setPortraitOrientation();
        }else {
            landscapeAction->setChecked(true);
            preview->setLandscapeOrientation();
        }
    }
}

void QPrintPreviewDialogPrivate::_q_previewChanged()
{
    updateNavActions();
    updatePageNumLabel();
    updateZoomFactor();
}

void QPrintPreviewDialogPrivate::_q_zoomFactorChanged()
{
    QString text = zoomFactor->lineEdit()->text();
    bool ok;
    qreal factor = text.remove(u'%').toFloat(&ok);
    factor = qMax(qreal(1.0), qMin(qreal(1000.0), factor));
    if (ok) {
        preview->setZoomFactor(factor/100.0);
        zoomFactor->setEditText(QString::fromLatin1("%1%").arg(factor));
        setFitting(false);
    }
}

///////////////////////////////////////////////////////////////////////////

/*!
    \class QPrintPreviewDialog
    \since 4.4

    \brief The QPrintPreviewDialog class provides a dialog for
    previewing and configuring page layouts for printer output.

    \ingroup standard-dialogs
    \ingroup printing
    \inmodule QtPrintSupport

    Using QPrintPreviewDialog in your existing application is
    straightforward:

    \list 1
    \li Create the QPrintPreviewDialog.

    You can construct a QPrintPreviewDialog with an existing QPrinter
    object, or you can have QPrintPreviewDialog create one for you,
    which will be the system default printer.

    \li Connect the paintRequested() signal to a slot.

    When the dialog needs to generate a set of preview pages, the
    paintRequested() signal will be emitted. You can use the exact
    same code for the actual printing as for having the preview
    generated, including calling QPrinter::newPage() to start a new
    page in the preview. Connect a slot to the paintRequested()
    signal, where you draw onto the QPrinter object that is passed
    into the slot.

    \li Call exec().

    Call QPrintPreviewDialog::exec() to show the preview dialog.
    \endlist

    \sa QPrinter, QPrintDialog, QPageSetupDialog, QPrintPreviewWidget
*/

/*!
    Constructs a QPrintPreviewDialog based on \a printer and with \a
    parent as the parent widget. The widget flags \a flags are passed on
    to the QWidget constructor.

    \sa QWidget::setWindowFlags()
*/
QPrintPreviewDialog::QPrintPreviewDialog(QPrinter* printer, QWidget *parent, Qt::WindowFlags flags)
    : QDialog(*new QPrintPreviewDialogPrivate, parent, flags)
{
    Q_D(QPrintPreviewDialog);
    d->init(printer);
}

/*!
    \overload
    \fn QPrintPreviewDialog::QPrintPreviewDialog(QWidget *parent, Qt::WindowFlags flags)

    This will create an internal QPrinter object, which will use the
    system default printer.
*/
QPrintPreviewDialog::QPrintPreviewDialog(QWidget *parent, Qt::WindowFlags f)
    : QDialog(*new QPrintPreviewDialogPrivate, parent, f)
{
    Q_D(QPrintPreviewDialog);
    d->init();
}

/*!
    Destroys the QPrintPreviewDialog.
*/
QPrintPreviewDialog::~QPrintPreviewDialog()
{
    Q_D(QPrintPreviewDialog);
    if (d->ownPrinter)
        delete d->printer;
    delete d->printDialog;
    delete d->pageSetupDialog;
}

/*!
    \reimp
*/
void QPrintPreviewDialog::setVisible(bool visible)
{
    Q_D(QPrintPreviewDialog);
    // this will make the dialog get a decent default size
    if (visible && !d->initialized) {
        d->preview->updatePreview();
        d->initialized = true;
    }
    QDialog::setVisible(visible);
}

/*!
    \reimp
*/
void QPrintPreviewDialog::done(int result)
{
    Q_D(QPrintPreviewDialog);
    QDialog::done(result);
    if (d->receiverToDisconnectOnClose) {
        disconnect(this, SIGNAL(finished(int)),
                   d->receiverToDisconnectOnClose, d->memberToDisconnectOnClose);
        d->receiverToDisconnectOnClose = nullptr;
    }
    d->memberToDisconnectOnClose.clear();
}

/*!
    \overload
    \since 4.5

    Opens the dialog and connects its finished(int) signal to the slot specified
    by \a receiver and \a member.

    The signal will be disconnected from the slot when the dialog is closed.
*/
void QPrintPreviewDialog::open(QObject *receiver, const char *member)
{
    Q_D(QPrintPreviewDialog);
    // the int parameter isn't very useful here; we could just as well connect
    // to reject(), but this feels less robust somehow
    connect(this, SIGNAL(finished(int)), receiver, member);
    d->receiverToDisconnectOnClose = receiver;
    d->memberToDisconnectOnClose = member;
    QDialog::open();
}

/*!
    Returns a pointer to the QPrinter object this dialog is currently
    operating on.
*/
QPrinter *QPrintPreviewDialog::printer()
{
    Q_D(QPrintPreviewDialog);
    return d->printer;
}

/*!
    \fn void QPrintPreviewDialog::paintRequested(QPrinter *printer)

    This signal is emitted when the QPrintPreviewDialog needs to generate
    a set of preview pages.

    The \a printer instance supplied is the paint device onto which you should
    paint the contents of each page, using the QPrinter instance in the same way
    as you would when printing directly.
*/


QT_END_NAMESPACE

#include "moc_qprintpreviewdialog.cpp"
#include "qprintpreviewdialog.moc"
