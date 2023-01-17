// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef PROPERTYFIELD_H
#define PROPERTYFIELD_H

#include <QLineEdit>
#include <QMetaProperty>
#include <QElapsedTimer>

/*!
    A QLineEdit for viewing the text form of a property on an object.
    Automatically stays up-to-date when the property changes.
    (This is rather like a QML TextField bound to a property.)
 */
class PropertyField : public QLineEdit
{
    Q_OBJECT
public:
    explicit PropertyField(QObject* subject, const QMetaProperty& prop, QWidget *parent = nullptr);

signals:

public slots:
    void propertyChanged();

protected:
    QString valueToString(QVariant val);

private:
    QObject* m_subject;
    QString m_lastText;
    QString m_lastTextShowing;
    QElapsedTimer m_lastChangeTime;
    const QMetaProperty m_prop;
    QBrush m_defaultBrush;
};

#endif // PROPERTYFIELD_H
