// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGTK3DIALOGHELPERS_H
#define QGTK3DIALOGHELPERS_H

#include <QtCore/qhash.h>
#include <QtCore/qlist.h>
#include <QtCore/qurl.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qstring.h>
#include <qpa/qplatformdialoghelper.h>

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkDialog GtkDialog;
typedef struct _GtkFileFilter GtkFileFilter;

QT_BEGIN_NAMESPACE

class QGtk3Dialog;
class QColor;

class QGtk3ColorDialogHelper : public QPlatformColorDialogHelper
{
    Q_OBJECT

public:
    QGtk3ColorDialogHelper();
    ~QGtk3ColorDialogHelper();

    bool show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent) override;
    void exec() override;
    void hide() override;

    void setCurrentColor(const QColor &color) override;
    QColor currentColor() const override;

private:
    static void onColorChanged(QGtk3ColorDialogHelper *helper);
    void applyOptions();

    QScopedPointer<QGtk3Dialog> d;
};

class QGtk3FileDialogHelper : public QPlatformFileDialogHelper
{
    Q_OBJECT

public:
    QGtk3FileDialogHelper();
    ~QGtk3FileDialogHelper();

    bool show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent) override;
    void exec() override;
    void hide() override;

    bool defaultNameFilterDisables() const override;
    void setDirectory(const QUrl &directory) override;
    QUrl directory() const override;
    void selectFile(const QUrl &filename) override;
    QList<QUrl> selectedFiles() const override;
    void setFilter() override;
    void selectNameFilter(const QString &filter) override;
    QString selectedNameFilter() const override;

private:
    static void onSelectionChanged(GtkDialog *dialog, QGtk3FileDialogHelper *helper);
    static void onCurrentFolderChanged(QGtk3FileDialogHelper *helper);
    static void onFilterChanged(QGtk3FileDialogHelper *helper);
    static void onUpdatePreview(GtkDialog *dialog, QGtk3FileDialogHelper *helper);
    void applyOptions();
    void setNameFilters(const QStringList &filters);
    void selectFileInternal(const QUrl &filename);
    void setFileChooserAction();

    QUrl _dir;
    QList<QUrl> _selection;
    QHash<QString, GtkFileFilter*> _filters;
    QHash<GtkFileFilter*, QString> _filterNames;
    QScopedPointer<QGtk3Dialog> d;
    GtkWidget *previewWidget;
};

class QGtk3FontDialogHelper : public QPlatformFontDialogHelper
{
    Q_OBJECT

public:
    QGtk3FontDialogHelper();
    ~QGtk3FontDialogHelper();

    bool show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent) override;
    void exec() override;
    void hide() override;

    void setCurrentFont(const QFont &font) override;
    QFont currentFont() const override;

private:
    static void onFontChanged(QGtk3FontDialogHelper *helper);
    void applyOptions();

    QScopedPointer<QGtk3Dialog> d;
};

QT_END_NAMESPACE

#endif // QGTK3DIALOGHELPERS_H
