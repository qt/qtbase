// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgtk3dialoghelpers.h"
#include "qgtk3theme.h"

#include <qeventloop.h>
#include <qwindow.h>
#include <qcolor.h>
#include <qdebug.h>
#include <qfont.h>
#include <qfileinfo.h>

#include <private/qguiapplication_p.h>
#include <private/qgenericunixservices_p.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformfontdatabase.h>

#undef signals
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <pango/pango.h>

#if QT_CONFIG(xlib) && defined(GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#endif

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif

// The size of the preview we display for selected image files. We set height
// larger than width because generally there is more free space vertically
// than horizontally (setting the preview image will always expand the width of
// the dialog, but usually not the height). The image's aspect ratio will always
// be preserved.
#define PREVIEW_WIDTH 256
#define PREVIEW_HEIGHT 512

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QGtk3Dialog
{
public:
    QGtk3Dialog(GtkWidget *gtkWidget, QPlatformDialogHelper *helper);
    ~QGtk3Dialog();

    GtkDialog *gtkDialog() const;

    void exec();
    bool show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent);
    void hide();

protected:
    static void onResponse(QPlatformDialogHelper *helper, int response);

private:
    GtkWidget *gtkWidget;
    QPlatformDialogHelper *helper;
    Qt::WindowModality modality;
};

QGtk3Dialog::QGtk3Dialog(GtkWidget *gtkWidget, QPlatformDialogHelper *helper)
    : gtkWidget(gtkWidget)
    , helper(helper)
{
    g_signal_connect_swapped(G_OBJECT(gtkWidget), "response", G_CALLBACK(onResponse), helper);
    g_signal_connect(G_OBJECT(gtkWidget), "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
}

QGtk3Dialog::~QGtk3Dialog()
{
    gtk_clipboard_store(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
    gtk_widget_destroy(gtkWidget);
}

GtkDialog *QGtk3Dialog::gtkDialog() const
{
    return GTK_DIALOG(gtkWidget);
}

void QGtk3Dialog::exec()
{
    if (modality == Qt::ApplicationModal) {
        // block input to the whole app, including other GTK dialogs
        gtk_dialog_run(gtkDialog());
    } else {
        // block input to the window, allow input to other GTK dialogs
        QEventLoop loop;
        loop.connect(helper, SIGNAL(accept()), SLOT(quit()));
        loop.connect(helper, SIGNAL(reject()), SLOT(quit()));
        loop.exec();
    }
}

bool QGtk3Dialog::show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent)
{
    Q_UNUSED(flags);
    this->modality = modality;

    gtk_widget_realize(gtkWidget); // creates X window

    GdkWindow *gdkWindow = gtk_widget_get_window(gtkWidget);
    if (parent) {
        if (false) {
#if defined(GDK_WINDOWING_WAYLAND) && GTK_CHECK_VERSION(3, 22, 0)
        } else if (GDK_IS_WAYLAND_WINDOW(gdkWindow)) {
            const auto unixServices = dynamic_cast<QGenericUnixServices *>(
                    QGuiApplicationPrivate::platformIntegration()->services());
            if (unixServices) {
                const auto handle = unixServices->portalWindowIdentifier(parent);
                if (handle.startsWith("wayland:"_L1)) {
                    auto handleBa = handle.sliced(8).toUtf8();
                    gdk_wayland_window_set_transient_for_exported(gdkWindow, handleBa.data());
                }
            }
#endif
#if QT_CONFIG(xlib) && defined(GDK_WINDOWING_X11)
        } else if (GDK_IS_X11_WINDOW(gdkWindow)) {
            GdkDisplay *gdkDisplay = gdk_window_get_display(gdkWindow);
            XSetTransientForHint(gdk_x11_display_get_xdisplay(gdkDisplay),
                                 gdk_x11_window_get_xid(gdkWindow),
                                 parent->winId());
#endif
        }
    }

    if (modality != Qt::NonModal) {
        gdk_window_set_modal_hint(gdkWindow, true);
    }

    gtk_widget_show(gtkWidget);
    gdk_window_focus(gdkWindow, GDK_CURRENT_TIME);
    return true;
}

void QGtk3Dialog::hide()
{
    gtk_widget_hide(gtkWidget);
}

void QGtk3Dialog::onResponse(QPlatformDialogHelper *helper, int response)
{
    if (response == GTK_RESPONSE_OK)
        emit helper->accept();
    else
        emit helper->reject();
}

QGtk3ColorDialogHelper::QGtk3ColorDialogHelper()
{
    d.reset(new QGtk3Dialog(gtk_color_chooser_dialog_new("", nullptr), this));
    g_signal_connect_swapped(d->gtkDialog(), "notify::rgba", G_CALLBACK(onColorChanged), this);
}

QGtk3ColorDialogHelper::~QGtk3ColorDialogHelper()
{
}

bool QGtk3ColorDialogHelper::show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent)
{
    applyOptions();
    return d->show(flags, modality, parent);
}

void QGtk3ColorDialogHelper::exec()
{
    d->exec();
}

void QGtk3ColorDialogHelper::hide()
{
    d->hide();
}

void QGtk3ColorDialogHelper::setCurrentColor(const QColor &color)
{
    GtkDialog *gtkDialog = d->gtkDialog();
    if (color.alpha() < 255)
        gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(gtkDialog), true);
    GdkRGBA gdkColor;
    gdkColor.red = color.redF();
    gdkColor.green = color.greenF();
    gdkColor.blue = color.blueF();
    gdkColor.alpha = color.alphaF();
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(gtkDialog), &gdkColor);
}

QColor QGtk3ColorDialogHelper::currentColor() const
{
    GtkDialog *gtkDialog = d->gtkDialog();
    GdkRGBA gdkColor;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(gtkDialog), &gdkColor);
    return QColor::fromRgbF(gdkColor.red, gdkColor.green, gdkColor.blue, gdkColor.alpha);
}

void QGtk3ColorDialogHelper::onColorChanged(QGtk3ColorDialogHelper *dialog)
{
    emit dialog->currentColorChanged(dialog->currentColor());
}

void QGtk3ColorDialogHelper::applyOptions()
{
    GtkDialog *gtkDialog = d->gtkDialog();
    gtk_window_set_title(GTK_WINDOW(gtkDialog), qUtf8Printable(options()->windowTitle()));

    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(gtkDialog), options()->testOption(QColorDialogOptions::ShowAlphaChannel));
}

QGtk3FileDialogHelper::QGtk3FileDialogHelper()
{
    d.reset(new QGtk3Dialog(gtk_file_chooser_dialog_new("", nullptr,
                                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                                        qUtf8Printable(QGtk3Theme::defaultStandardButtonText(QPlatformDialogHelper::Cancel)), GTK_RESPONSE_CANCEL,
                                                        qUtf8Printable(QGtk3Theme::defaultStandardButtonText(QPlatformDialogHelper::Ok)), GTK_RESPONSE_OK,
                                                        NULL), this));

    g_signal_connect(GTK_FILE_CHOOSER(d->gtkDialog()), "selection-changed", G_CALLBACK(onSelectionChanged), this);
    g_signal_connect_swapped(GTK_FILE_CHOOSER(d->gtkDialog()), "current-folder-changed", G_CALLBACK(onCurrentFolderChanged), this);
    g_signal_connect_swapped(GTK_FILE_CHOOSER(d->gtkDialog()), "notify::filter", G_CALLBACK(onFilterChanged), this);

    previewWidget = gtk_image_new();
    g_signal_connect(G_OBJECT(d->gtkDialog()), "update-preview", G_CALLBACK(onUpdatePreview), this);
    gtk_file_chooser_set_preview_widget(GTK_FILE_CHOOSER(d->gtkDialog()), previewWidget);
}

QGtk3FileDialogHelper::~QGtk3FileDialogHelper()
{
}

bool QGtk3FileDialogHelper::show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent)
{
    _dir.clear();
    _selection.clear();

    applyOptions();
    return d->show(flags, modality, parent);
}

void QGtk3FileDialogHelper::exec()
{
    d->exec();
}

void QGtk3FileDialogHelper::hide()
{
    // After GtkFileChooserDialog has been hidden, gtk_file_chooser_get_current_folder()
    // & gtk_file_chooser_get_filenames() will return bogus values -> cache the actual
    // values before hiding the dialog
    _dir = directory();
    _selection = selectedFiles();

    d->hide();
}

bool QGtk3FileDialogHelper::defaultNameFilterDisables() const
{
    return false;
}

void QGtk3FileDialogHelper::setDirectory(const QUrl &directory)
{
    GtkDialog *gtkDialog = d->gtkDialog();
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(gtkDialog), qUtf8Printable(directory.toLocalFile()));
}

QUrl QGtk3FileDialogHelper::directory() const
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
    return QUrl::fromLocalFile(ret);
}

void QGtk3FileDialogHelper::selectFile(const QUrl &filename)
{
    setFileChooserAction();
    selectFileInternal(filename);
}

void QGtk3FileDialogHelper::selectFileInternal(const QUrl &filename)
{
    GtkDialog *gtkDialog = d->gtkDialog();
    if (options()->acceptMode() == QFileDialogOptions::AcceptSave) {
        QFileInfo fi(filename.toLocalFile());
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(gtkDialog), qUtf8Printable(fi.path()));
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(gtkDialog), qUtf8Printable(fi.fileName()));
    } else {
        gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(gtkDialog), qUtf8Printable(filename.toLocalFile()));
    }
}

QList<QUrl> QGtk3FileDialogHelper::selectedFiles() const
{
    // While GtkFileChooserDialog is hidden, gtk_file_chooser_get_filenames()
    // returns a bogus value -> return the cached value before hiding
    if (!_selection.isEmpty())
        return _selection;

    QList<QUrl> selection;
    GtkDialog *gtkDialog = d->gtkDialog();
    GSList *filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(gtkDialog));
    for (GSList *it  = filenames; it; it = it->next)
        selection += QUrl::fromLocalFile(QString::fromUtf8((const char*)it->data));
    g_slist_free(filenames);
    return selection;
}

void QGtk3FileDialogHelper::setFilter()
{
    applyOptions();
}

void QGtk3FileDialogHelper::selectNameFilter(const QString &filter)
{
    GtkFileFilter *gtkFilter = _filters.value(filter);
    if (gtkFilter) {
        GtkDialog *gtkDialog = d->gtkDialog();
        gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(gtkDialog), gtkFilter);
    }
}

QString QGtk3FileDialogHelper::selectedNameFilter() const
{
    GtkDialog *gtkDialog = d->gtkDialog();
    GtkFileFilter *gtkFilter = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(gtkDialog));
    return _filterNames.value(gtkFilter);
}

void QGtk3FileDialogHelper::onSelectionChanged(GtkDialog *gtkDialog, QGtk3FileDialogHelper *helper)
{
    QString selection;
    gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(gtkDialog));
    if (filename) {
        selection = QString::fromUtf8(filename);
        g_free(filename);
    }
    emit helper->currentChanged(QUrl::fromLocalFile(selection));
}

void QGtk3FileDialogHelper::onCurrentFolderChanged(QGtk3FileDialogHelper *dialog)
{
    emit dialog->directoryEntered(dialog->directory());
}

void QGtk3FileDialogHelper::onFilterChanged(QGtk3FileDialogHelper *dialog)
{
    emit dialog->filterSelected(dialog->selectedNameFilter());
}

void QGtk3FileDialogHelper::onUpdatePreview(GtkDialog *gtkDialog, QGtk3FileDialogHelper *helper)
{
    gchar *filename = gtk_file_chooser_get_preview_filename(GTK_FILE_CHOOSER(gtkDialog));
    if (!filename) {
        gtk_file_chooser_set_preview_widget_active(GTK_FILE_CHOOSER(gtkDialog), false);
        return;
    }

    // Don't attempt to open anything which isn't a regular file. If a named pipe,
    // this may hang.
    QFileInfo fileinfo(filename);
    if (!fileinfo.exists() || !fileinfo.isFile()) {
        g_free(filename);
        gtk_file_chooser_set_preview_widget_active(GTK_FILE_CHOOSER(gtkDialog), false);
        return;
    }

    // This will preserve the image's aspect ratio.
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_size(filename, PREVIEW_WIDTH, PREVIEW_HEIGHT, 0);
    g_free(filename);
    if (pixbuf) {
        gtk_image_set_from_pixbuf(GTK_IMAGE(helper->previewWidget), pixbuf);
        g_object_unref(pixbuf);
    }
    gtk_file_chooser_set_preview_widget_active(GTK_FILE_CHOOSER(gtkDialog), pixbuf ? true : false);
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

void QGtk3FileDialogHelper::setFileChooserAction()
{
    GtkDialog *gtkDialog = d->gtkDialog();

    const GtkFileChooserAction action = gtkFileChooserAction(options());
    gtk_file_chooser_set_action(GTK_FILE_CHOOSER(gtkDialog), action);
}

void QGtk3FileDialogHelper::applyOptions()
{
    GtkDialog *gtkDialog = d->gtkDialog();
    const QSharedPointer<QFileDialogOptions> &opts = options();

    gtk_window_set_title(GTK_WINDOW(gtkDialog), qUtf8Printable(opts->windowTitle()));
    gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(gtkDialog), true);

    setFileChooserAction();

    const bool selectMultiple = opts->fileMode() == QFileDialogOptions::ExistingFiles;
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(gtkDialog), selectMultiple);

    const bool confirmOverwrite = !opts->testOption(QFileDialogOptions::DontConfirmOverwrite);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(gtkDialog), confirmOverwrite);

    const bool readOnly = opts->testOption(QFileDialogOptions::ReadOnly);
    gtk_file_chooser_set_create_folders(GTK_FILE_CHOOSER(gtkDialog), !readOnly);

    const QStringList nameFilters = opts->nameFilters();
    if (!nameFilters.isEmpty())
        setNameFilters(nameFilters);

    if (opts->initialDirectory().isLocalFile())
        setDirectory(opts->initialDirectory());

    foreach (const QUrl &filename, opts->initiallySelectedFiles())
        selectFileInternal(filename);

    const QString initialNameFilter = opts->initiallySelectedNameFilter();
    if (!initialNameFilter.isEmpty())
        selectNameFilter(initialNameFilter);

    GtkWidget *acceptButton = gtk_dialog_get_widget_for_response(gtkDialog, GTK_RESPONSE_OK);
    if (acceptButton) {
        if (opts->isLabelExplicitlySet(QFileDialogOptions::Accept))
            gtk_button_set_label(GTK_BUTTON(acceptButton), qUtf8Printable(opts->labelText(QFileDialogOptions::Accept)));
        else if (opts->acceptMode() == QFileDialogOptions::AcceptOpen)
            gtk_button_set_label(GTK_BUTTON(acceptButton), qUtf8Printable(QGtk3Theme::defaultStandardButtonText(QPlatformDialogHelper::Open)));
        else
            gtk_button_set_label(GTK_BUTTON(acceptButton), qUtf8Printable(QGtk3Theme::defaultStandardButtonText(QPlatformDialogHelper::Save)));
    }

    GtkWidget *rejectButton = gtk_dialog_get_widget_for_response(gtkDialog, GTK_RESPONSE_CANCEL);
    if (rejectButton) {
        if (opts->isLabelExplicitlySet(QFileDialogOptions::Reject))
            gtk_button_set_label(GTK_BUTTON(rejectButton), qUtf8Printable(opts->labelText(QFileDialogOptions::Reject)));
        else
            gtk_button_set_label(GTK_BUTTON(rejectButton), qUtf8Printable(QGtk3Theme::defaultStandardButtonText(QPlatformDialogHelper::Cancel)));
    }
}

void QGtk3FileDialogHelper::setNameFilters(const QStringList &filters)
{
    GtkDialog *gtkDialog = d->gtkDialog();
    foreach (GtkFileFilter *filter, _filters)
        gtk_file_chooser_remove_filter(GTK_FILE_CHOOSER(gtkDialog), filter);

    _filters.clear();
    _filterNames.clear();

    foreach (const QString &filter, filters) {
        GtkFileFilter *gtkFilter = gtk_file_filter_new();
        const QString name = filter.left(filter.indexOf(u'('));
        const QStringList extensions = cleanFilterList(filter);

        gtk_file_filter_set_name(gtkFilter, qUtf8Printable(name.isEmpty() ? extensions.join(", "_L1) : name));
        foreach (const QString &ext, extensions)
            gtk_file_filter_add_pattern(gtkFilter, qUtf8Printable(ext));

        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(gtkDialog), gtkFilter);

        _filters.insert(filter, gtkFilter);
        _filterNames.insert(gtkFilter, filter);
    }
}

QGtk3FontDialogHelper::QGtk3FontDialogHelper()
{
    d.reset(new QGtk3Dialog(gtk_font_chooser_dialog_new("", nullptr), this));
    g_signal_connect_swapped(d->gtkDialog(), "notify::font", G_CALLBACK(onFontChanged), this);
}

QGtk3FontDialogHelper::~QGtk3FontDialogHelper()
{
}

bool QGtk3FontDialogHelper::show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent)
{
    applyOptions();
    return d->show(flags, modality, parent);
}

void QGtk3FontDialogHelper::exec()
{
    d->exec();
}

void QGtk3FontDialogHelper::hide()
{
    d->hide();
}

static QString qt_fontToString(const QFont &font)
{
    PangoFontDescription *desc = pango_font_description_new();
    pango_font_description_set_size(desc, (font.pointSizeF() > 0.0 ? font.pointSizeF() : QFontInfo(font).pointSizeF()) * PANGO_SCALE);
    pango_font_description_set_family(desc, qUtf8Printable(QFontInfo(font).family()));

    int weight = font.weight();
    if (weight >= QFont::Black)
        pango_font_description_set_weight(desc, PANGO_WEIGHT_HEAVY);
    else if (weight >= QFont::ExtraBold)
        pango_font_description_set_weight(desc, PANGO_WEIGHT_ULTRABOLD);
    else if (weight >= QFont::Bold)
        pango_font_description_set_weight(desc, PANGO_WEIGHT_BOLD);
    else if (weight >= QFont::DemiBold)
        pango_font_description_set_weight(desc, PANGO_WEIGHT_SEMIBOLD);
    else if (weight >= QFont::Medium)
        pango_font_description_set_weight(desc, PANGO_WEIGHT_MEDIUM);
    else if (weight >= QFont::Normal)
        pango_font_description_set_weight(desc, PANGO_WEIGHT_NORMAL);
    else if (weight >= QFont::Light)
        pango_font_description_set_weight(desc, PANGO_WEIGHT_LIGHT);
    else if (weight >= QFont::ExtraLight)
        pango_font_description_set_weight(desc, PANGO_WEIGHT_ULTRALIGHT);
    else
        pango_font_description_set_weight(desc, PANGO_WEIGHT_THIN);

    int style = font.style();
    if (style == QFont::StyleItalic)
        pango_font_description_set_style(desc, PANGO_STYLE_ITALIC);
    else if (style == QFont::StyleOblique)
        pango_font_description_set_style(desc, PANGO_STYLE_OBLIQUE);
    else
        pango_font_description_set_style(desc, PANGO_STYLE_NORMAL);

    char *str = pango_font_description_to_string(desc);
    QString name = QString::fromUtf8(str);
    pango_font_description_free(desc);
    g_free(str);
    return name;
}

static QFont qt_fontFromString(const QString &name)
{
    QFont font;
    PangoFontDescription *desc = pango_font_description_from_string(qUtf8Printable(name));
    font.setPointSizeF(static_cast<float>(pango_font_description_get_size(desc)) / PANGO_SCALE);

    QString family = QString::fromUtf8(pango_font_description_get_family(desc));
    if (!family.isEmpty())
        font.setFamilies(QStringList{family});

    font.setWeight(QFont::Weight(pango_font_description_get_weight(desc)));

    PangoStyle style = pango_font_description_get_style(desc);
    if (style == PANGO_STYLE_ITALIC)
        font.setStyle(QFont::StyleItalic);
    else if (style == PANGO_STYLE_OBLIQUE)
        font.setStyle(QFont::StyleOblique);
    else
        font.setStyle(QFont::StyleNormal);

    pango_font_description_free(desc);
    return font;
}

void QGtk3FontDialogHelper::setCurrentFont(const QFont &font)
{
    GtkFontChooser *gtkDialog = GTK_FONT_CHOOSER(d->gtkDialog());
    gtk_font_chooser_set_font(gtkDialog, qUtf8Printable(qt_fontToString(font)));
}

QFont QGtk3FontDialogHelper::currentFont() const
{
    GtkFontChooser *gtkDialog = GTK_FONT_CHOOSER(d->gtkDialog());
    gchar *name = gtk_font_chooser_get_font(gtkDialog);
    QFont font = qt_fontFromString(QString::fromUtf8(name));
    g_free(name);
    return font;
}

void QGtk3FontDialogHelper::onFontChanged(QGtk3FontDialogHelper *dialog)
{
    emit dialog->currentFontChanged(dialog->currentFont());
}

void QGtk3FontDialogHelper::applyOptions()
{
    GtkDialog *gtkDialog = d->gtkDialog();
    const QSharedPointer<QFontDialogOptions> &opts = options();

    gtk_window_set_title(GTK_WINDOW(gtkDialog), qUtf8Printable(opts->windowTitle()));
}

QT_END_NAMESPACE

#include "moc_qgtk3dialoghelpers.cpp"
