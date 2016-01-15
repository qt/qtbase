/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef PROPERTY_WATCHER_H
#define PROPERTY_WATCHER_H

#include <QWidget>

class QLineEdit;
class QFormLayout;

class PropertyWatcher : public QWidget
{
    Q_OBJECT

public:
    explicit PropertyWatcher(QObject* subject = Q_NULLPTR, QString annotation = QString(), QWidget *parent = Q_NULLPTR);

    QFormLayout *formLayout() { return m_formLayout; }

    QObject *subject() const { return m_subject; }
    void setSubject(QObject *s, const QString &annotation = QString());

public slots:
    void updateAllFields();
    void subjectDestroyed();

signals:
    void updatedAllFields(PropertyWatcher* sender);

private:
    QObject* m_subject;
    QFormLayout * m_formLayout;
};

#endif // PROPERTY_WATCHER_H
