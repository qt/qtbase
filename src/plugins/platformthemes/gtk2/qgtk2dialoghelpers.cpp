/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qgtk2dialoghelpers.h"

#include <qeventloop.h>
#include <qwindow.h>
#include <qcolor.h>
#include <qdebug.h>

#include <private/qguiapplication_p.h>

#undef signals
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

QT_BEGIN_NAMESPACE

class QGtk2Dialog : public QWindow
{
    Q_OBJECT

public:
    QGtk2Dialog(GtkWidget *gtkWidget);
    ~QGtk2Dialog();

    GtkDialog* gtkDialog() const;

    void exec();
    bool show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent);
    void hide();

Q_SIGNALS:
    void accept();
    void reject();

protected:
    static void onResponse(QGtk2Dialog *dialog, int response);

private:
    GtkWidget *gtkWidget;
};

QGtk2Dialog::QGtk2Dialog(GtkWidget *gtkWidget) : gtkWidget(gtkWidget)
{
    g_signal_connect_swapped(G_OBJECT(gtkWidget), "response", G_CALLBACK(onResponse), this);
    g_signal_connect(G_OBJECT(gtkWidget), "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
}

QGtk2Dialog::~QGtk2Dialog()
{
    gtk_widget_destroy(gtkWidget);
}

GtkDialog* QGtk2Dialog::gtkDialog() const
{
    return GTK_DIALOG(gtkWidget);
}

void QGtk2Dialog::exec()
{
    if (modality() == Qt::ApplicationModal) {
        // block input to the whole app, including other GTK dialogs
        gtk_dialog_run(gtkDialog());
    } else {
        // block input to the window, allow input to other GTK dialogs
        QEventLoop loop;
        connect(this, SIGNAL(accept()), &loop, SLOT(quit()));
        connect(this, SIGNAL(reject()), &loop, SLOT(quit()));
        loop.exec();
    }
}

bool QGtk2Dialog::show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent)
{
    setParent(parent);
    setFlags(flags);
    setModality(modality);

    gtk_widget_realize(gtkWidget); // creates X window

    if (parent) {
        XSetTransientForHint(gdk_x11_drawable_get_xdisplay(gtkWidget->window),
                             gdk_x11_drawable_get_xid(gtkWidget->window),
                             parent->winId());
    }

    if (modality != Qt::NonModal) {
        gdk_window_set_modal_hint(gtkWidget->window, true);
        QGuiApplicationPrivate::showModalWindow(this);
    }

    gtk_widget_show(gtkWidget);
    return true;
}

void QGtk2Dialog::hide()
{
    QGuiApplicationPrivate::hideModalWindow(this);
    gtk_widget_hide(gtkWidget);
}

void QGtk2Dialog::onResponse(QGtk2Dialog *dialog, int response)
{
    if (response == GTK_RESPONSE_OK)
        emit dialog->accept();
    else
        emit dialog->reject();
}

QGtk2ColorDialogHelper::QGtk2ColorDialogHelper()
{
    d.reset(new QGtk2Dialog(gtk_color_selection_dialog_new("")));
    connect(d.data(), SIGNAL(accept()), this, SLOT(onAccepted()));
    connect(d.data(), SIGNAL(reject()), this, SIGNAL(reject()));

    GtkWidget *gtkColorSelection = gtk_color_selection_dialog_get_color_selection(GTK_COLOR_SELECTION_DIALOG(d->gtkDialog()));
    g_signal_connect_swapped(gtkColorSelection, "color-changed", G_CALLBACK(onColorChanged), this);
}

QGtk2ColorDialogHelper::~QGtk2ColorDialogHelper()
{
}

bool QGtk2ColorDialogHelper::show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent)
{
    applyOptions();
    return d->show(flags, modality, parent);
}

void QGtk2ColorDialogHelper::exec()
{
    d->exec();
}

void QGtk2ColorDialogHelper::hide()
{
    d->hide();
}

void QGtk2ColorDialogHelper::setCurrentColor(const QColor &color)
{
    GtkDialog *gtkDialog = d->gtkDialog();
    GtkWidget *gtkColorSelection = gtk_color_selection_dialog_get_color_selection(GTK_COLOR_SELECTION_DIALOG(gtkDialog));
    GdkColor gdkColor;
    gdkColor.red = color.red() << 8;
    gdkColor.green = color.green() << 8;
    gdkColor.blue = color.blue() << 8;
    gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(gtkColorSelection), &gdkColor);
    if (color.alpha() < 255) {
        gtk_color_selection_set_has_opacity_control(GTK_COLOR_SELECTION(gtkColorSelection), true);
        gtk_color_selection_set_current_alpha(GTK_COLOR_SELECTION(gtkColorSelection), color.alpha() << 8);
    }
}

QColor QGtk2ColorDialogHelper::currentColor() const
{
    GtkDialog *gtkDialog = d->gtkDialog();
    GtkWidget *gtkColorSelection = gtk_color_selection_dialog_get_color_selection(GTK_COLOR_SELECTION_DIALOG(gtkDialog));
    GdkColor gdkColor;
    gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(gtkColorSelection), &gdkColor);
    guint16 alpha = gtk_color_selection_get_current_alpha(GTK_COLOR_SELECTION(gtkColorSelection));
    return QColor(gdkColor.red >> 8, gdkColor.green >> 8, gdkColor.blue >> 8, alpha >> 8);
}

void QGtk2ColorDialogHelper::onAccepted()
{
    emit accept();
    emit colorSelected(currentColor());
}

void QGtk2ColorDialogHelper::onColorChanged(QGtk2ColorDialogHelper *dialog)
{
    emit dialog->currentColorChanged(dialog->currentColor());
}

void QGtk2ColorDialogHelper::applyOptions()
{
    GtkDialog* gtkDialog = d->gtkDialog();
    gtk_window_set_title(GTK_WINDOW(gtkDialog), options()->windowTitle().toUtf8());

    GtkWidget *gtkColorSelection = gtk_color_selection_dialog_get_color_selection(GTK_COLOR_SELECTION_DIALOG(gtkDialog));
    gtk_color_selection_set_has_opacity_control(GTK_COLOR_SELECTION(gtkColorSelection), options()->testOption(QColorDialogOptions::ShowAlphaChannel));

    GtkWidget *okButton = 0;
    GtkWidget *cancelButton = 0;
    GtkWidget *helpButton = 0;
    g_object_get(G_OBJECT(gtkDialog), "ok-button", &okButton, "cancel-button", &cancelButton, "help-button", &helpButton, NULL);
    if (okButton)
        g_object_set(G_OBJECT(okButton), "visible", !options()->testOption(QColorDialogOptions::NoButtons), NULL);
    if (cancelButton)
        g_object_set(G_OBJECT(cancelButton), "visible", !options()->testOption(QColorDialogOptions::NoButtons), NULL);
    if (helpButton)
        gtk_widget_hide(helpButton);
}

QGtk2FileDialogHelper::QGtk2FileDialogHelper()
{
    d.reset(new QGtk2Dialog(gtk_file_chooser_dialog_new("", 0,
                                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                        GTK_STOCK_OK, GTK_RESPONSE_OK, NULL)));
    connect(d.data(), SIGNAL(accept()), this, SLOT(onAccepted()));
    connect(d.data(), SIGNAL(reject()), this, SIGNAL(reject()));

    g_signal_connect(GTK_FILE_CHOOSER(d->gtkDialog()), "selection-changed", G_CALLBACK(onSelectionChanged), this);
    g_signal_connect_swapped(GTK_FILE_CHOOSER(d->gtkDialog()), "current-folder-changed", G_CALLBACK(onCurrentFolderChanged), this);
}

QGtk2FileDialogHelper::~QGtk2FileDialogHelper()
{
}

bool QGtk2FileDialogHelper::show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent)
{
    _dir.clear();
    _selection.clear();

    applyOptions();
    return d->show(flags, modality, parent);
}

void QGtk2FileDialogHelper::exec()
{
    d->exec();
}

void QGtk2FileDialogHelper::hide()
{
    // After GtkFileChooserDialog has been hidden, gtk_file_chooser_get_current_folder()
    // & gtk_file_chooser_get_filenames() will return bogus values -> cache the actual
    // values before hiding the dialog
    _dir = directory();
    _selection = selectedFiles();

    d->hide();
}

bool QGtk2FileDialogHelper::defaultNameFilterDisables() const
{
    return false;
}

void QGtk2FileDialogHelper::setDirectory(const QString &directory)
{
    GtkDialog *gtkDialog = d->gtkDialog();
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(gtkDialog), directory.toUtf8());
}

QString QGtk2FileDialogHelper::directory() const
{
    // While GtkFileChooserDialog is hidden, gtk_file_chooser_get_current_folder()
    // returns a bogus value -> return the cached value before hiding
    if (!_dir.isEmpty())
        return _dir;

    QString ret;
    GtkDialog *gtkDialog = d->gtkDialog();
    gchar *folder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(gtkDialog));
    if (folder) {
        ret = QString::fromUtf8(folder);
        g_free(folder);
    }
    return ret;
}

void QGtk2FileDialogHelper::selectFile(const QString &filename)
{
    GtkDialog *gtkDialog = d->gtkDialog();
    gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(gtkDialog), filename.toUtf8());
}

QStringList QGtk2FileDialogHelper::selectedFiles() const
{
    // While GtkFileChooserDialog is hidden, gtk_file_chooser_get_filenames()
    // returns a bogus value -> return the cached value before hiding
    if (!_selection.isEmpty())
        return _selection;

    QStringList selection;
    GtkDialog *gtkDialog = d->gtkDialog();
    GSList *filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(gtkDialog));
    for (GSList *it  = filenames; it; it = it->next)
        selection += QString::fromUtf8((const char*)it->data);
    g_slist_free(filenames);
    return selection;
}

void QGtk2FileDialogHelper::setFilter()
{
    applyOptions();
}

void QGtk2FileDialogHelper::selectNameFilter(const QString &filter)
{
    GtkFileFilter *gtkFilter = _filters.value(filter);
    if (gtkFilter) {
        GtkDialog *gtkDialog = d->gtkDialog();
        gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(gtkDialog), gtkFilter);
    }
}

QString QGtk2FileDialogHelper::selectedNameFilter() const
{
    GtkDialog *gtkDialog = d->gtkDialog();
    GtkFileFilter *gtkFilter = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(gtkDialog));
    return _filterNames.value(gtkFilter);
}

void QGtk2FileDialogHelper::onAccepted()
{
    emit accept();

    QString filter = selectedNameFilter();
    if (filter.isEmpty())
        emit filterSelected(filter);

    QStringList files = selectedFiles();
    emit filesSelected(files);
    if (files.count() == 1)
        emit fileSelected(files.first());
}

void QGtk2FileDialogHelper::onSelectionChanged(GtkDialog *gtkDialog, QGtk2FileDialogHelper *helper)
{
    QString selection;
    gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(gtkDialog));
    if (filename) {
        selection = QString::fromUtf8(filename);
        g_free(filename);
    }
    emit helper->currentChanged(selection);
}

void QGtk2FileDialogHelper::onCurrentFolderChanged(QGtk2FileDialogHelper *dialog)
{
    emit dialog->directoryEntered(dialog->directory());
}

static GtkFileChooserAction gtkFileChooserAction(const QSharedPointer<QFileDialogOptions> &options)
{
    switch (options->fileMode()) {
    case QFileDialogOptions::AnyFile:
    case QFileDialogOptions::ExistingFile:
    case QFileDialogOptions::ExistingFiles:
        if (options->acceptMode() == QFileDialogOptions::AcceptOpen)
            return GTK_FILE_CHOOSER_ACTION_OPEN;
        else
            return GTK_FILE_CHOOSER_ACTION_SAVE;
    case QFileDialogOptions::Directory:
    case QFileDialogOptions::DirectoryOnly:
    default:
        if (options->acceptMode() == QFileDialogOptions::AcceptOpen)
            return GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
        else
            return GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER;
    }
}

void QGtk2FileDialogHelper::applyOptions()
{
    GtkDialog *gtkDialog = d->gtkDialog();
    const QSharedPointer<QFileDialogOptions> &opts = options();

    gtk_window_set_title(GTK_WINDOW(gtkDialog), opts->windowTitle().toUtf8());
    gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(gtkDialog), true);

    const GtkFileChooserAction action = gtkFileChooserAction(opts);
    gtk_file_chooser_set_action(GTK_FILE_CHOOSER(gtkDialog), action);

    const bool selectMultiple = opts->fileMode() == QFileDialogOptions::ExistingFiles;
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(gtkDialog), selectMultiple);

    const bool confirmOverwrite = !opts->testOption(QFileDialogOptions::DontConfirmOverwrite);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(gtkDialog), confirmOverwrite);

    const QStringList nameFilters = opts->nameFilters();
    if (!nameFilters.isEmpty())
        setNameFilters(nameFilters);

    const QString initialDirectory = opts->initialDirectory();
    if (!initialDirectory.isEmpty())
        setDirectory(initialDirectory);

    foreach (const QString &filename, opts->initiallySelectedFiles())
        selectFile(filename);

    const QString initialNameFilter = opts->initiallySelectedNameFilter();
    if (!initialNameFilter.isEmpty())
        selectNameFilter(initialNameFilter);

    GtkWidget *acceptButton = gtk_dialog_get_widget_for_response(gtkDialog, GTK_RESPONSE_OK);
    if (acceptButton) {
        if (opts->isLabelExplicitlySet(QFileDialogOptions::Accept))
            gtk_button_set_label(GTK_BUTTON(acceptButton), opts->labelText(QFileDialogOptions::Accept).toUtf8());
        else if (opts->acceptMode() == QFileDialogOptions::AcceptOpen)
            gtk_button_set_label(GTK_BUTTON(acceptButton), GTK_STOCK_OPEN);
        else
            gtk_button_set_label(GTK_BUTTON(acceptButton), GTK_STOCK_SAVE);
    }

    GtkWidget *rejectButton = gtk_dialog_get_widget_for_response(gtkDialog, GTK_RESPONSE_CANCEL);
    if (rejectButton) {
        if (opts->isLabelExplicitlySet(QFileDialogOptions::Reject))
            gtk_button_set_label(GTK_BUTTON(rejectButton), opts->labelText(QFileDialogOptions::Reject).toUtf8());
        else
            gtk_button_set_label(GTK_BUTTON(rejectButton), GTK_STOCK_CANCEL);
    }
}

void QGtk2FileDialogHelper::setNameFilters(const QStringList& filters)
{
    GtkDialog *gtkDialog = d->gtkDialog();
    foreach (GtkFileFilter *filter, _filters)
        gtk_file_chooser_remove_filter(GTK_FILE_CHOOSER(gtkDialog), filter);

    _filters.clear();
    _filterNames.clear();

    foreach (const QString &filter, filters) {
        GtkFileFilter *gtkFilter = gtk_file_filter_new();
        const QString name = filter.left(filter.indexOf(QLatin1Char('(')));
        const QStringList extensions = cleanFilterList(filter);

        gtk_file_filter_set_name(gtkFilter, name.isEmpty() ? extensions.join(QStringLiteral(", ")).toUtf8() : name.toUtf8());
        foreach (const QString &ext, extensions)
            gtk_file_filter_add_pattern(gtkFilter, ext.toUtf8());

        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(gtkDialog), gtkFilter);

        _filters.insert(filter, gtkFilter);
        _filterNames.insert(gtkFilter, filter);
    }
}

QT_END_NAMESPACE

#include "qgtk2dialoghelpers.moc"
