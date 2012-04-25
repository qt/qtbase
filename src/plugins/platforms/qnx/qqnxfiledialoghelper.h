/***************************************************************************
**
** Copyright (C) 2012 Research In Motion
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQNXFILEDIALOGHELPER_H
#define QQNXFILEDIALOGHELPER_H

#include <qpa/qplatformdialoghelper.h>

#include <bps/dialog.h>

QT_BEGIN_NAMESPACE

class QQnxBpsEventFilter;

class QQnxFileDialogHelper : public QPlatformFileDialogHelper
{
    Q_OBJECT
public:
    explicit QQnxFileDialogHelper(QQnxBpsEventFilter *eventFilter);
    ~QQnxFileDialogHelper();

    bool handleEvent(bps_event_t *event);

    void exec();

    bool show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent);
    void hide();

    bool defaultNameFilterDisables() const;
    void setDirectory(const QString &directory);
    QString directory() const;
    void selectFile(const QString &fileName);
    QStringList selectedFiles() const;
    void setFilter();
    void selectNameFilter(const QString &filter);
    QString selectedNameFilter() const;

    dialog_instance_t nativeDialog() const { return m_dialog; }

Q_SIGNALS:
    void dialogClosed();

private:
    void setNameFilter(const QString &filter);

    QQnxBpsEventFilter *m_eventFilter;
    dialog_instance_t m_dialog;
    QFileDialogOptions::AcceptMode m_acceptMode;
    QString m_selectedFilter;

    QPlatformDialogHelper::DialogCode m_result;
    QStringList m_paths;
};

QT_END_NAMESPACE

#endif // QQNXFILEDIALOGHELPER_H
