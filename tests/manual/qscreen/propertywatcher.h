// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef PROPERTY_WATCHER_H
#define PROPERTY_WATCHER_H

#include <QWidget>

class QLineEdit;
class QFormLayout;

class PropertyWatcher : public QWidget
{
    Q_OBJECT

public:
    explicit PropertyWatcher(QObject* subject = nullptr, QString annotation = QString(), QWidget *parent = nullptr);

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
