/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGTK3DIALOGHELPERS_H
#define QGTK3DIALOGHELPERS_H

#include <QtCore/qhash.h>
#include <QtCore/qlist.h>
#include <QtCore/qurl.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qstring.h>
#include <qpa/qplatformdialoghelper.h>

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

    bool show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent) Q_DECL_OVERRIDE;
    void exec() Q_DECL_OVERRIDE;
    void hide() Q_DECL_OVERRIDE;

    void setCurrentColor(const QColor &color) Q_DECL_OVERRIDE;
    QColor currentColor() const Q_DECL_OVERRIDE;

private Q_SLOTS:
    void onAccepted();

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

    bool show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent) Q_DECL_OVERRIDE;
    void exec() Q_DECL_OVERRIDE;
    void hide() Q_DECL_OVERRIDE;

    bool defaultNameFilterDisables() const Q_DECL_OVERRIDE;
    void setDirectory(const QUrl &directory) Q_DECL_OVERRIDE;
    QUrl directory() const Q_DECL_OVERRIDE;
    void selectFile(const QUrl &filename) Q_DECL_OVERRIDE;
    QList<QUrl> selectedFiles() const Q_DECL_OVERRIDE;
    void setFilter() Q_DECL_OVERRIDE;
    void selectNameFilter(const QString &filter) Q_DECL_OVERRIDE;
    QString selectedNameFilter() const Q_DECL_OVERRIDE;

private Q_SLOTS:
    void onAccepted();

private:
    static void onSelectionChanged(GtkDialog *dialog, QGtk3FileDialogHelper *helper);
    static void onCurrentFolderChanged(QGtk3FileDialogHelper *helper);
    void applyOptions();
    void setNameFilters(const QStringList &filters);

    QUrl _dir;
    QList<QUrl> _selection;
    QHash<QString, GtkFileFilter*> _filters;
    QHash<GtkFileFilter*, QString> _filterNames;
    QScopedPointer<QGtk3Dialog> d;
};

class QGtk3FontDialogHelper : public QPlatformFontDialogHelper
{
    Q_OBJECT

public:
    QGtk3FontDialogHelper();
    ~QGtk3FontDialogHelper();

    bool show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent) Q_DECL_OVERRIDE;
    void exec() Q_DECL_OVERRIDE;
    void hide() Q_DECL_OVERRIDE;

    void setCurrentFont(const QFont &font) Q_DECL_OVERRIDE;
    QFont currentFont() const Q_DECL_OVERRIDE;

private Q_SLOTS:
    void onAccepted();

private:
    void applyOptions();

    QScopedPointer<QGtk3Dialog> d;
};

QT_END_NAMESPACE

#endif // QGTK3DIALOGHELPERS_H
