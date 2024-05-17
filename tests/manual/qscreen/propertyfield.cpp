// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "propertyfield.h"
#include <QDebug>

PropertyField::PropertyField(QObject* subject, const QMetaProperty& prop, QWidget *parent)
  : QLineEdit(parent), m_subject(subject), m_lastChangeTime(), m_prop(prop)
  , m_defaultBrush(palette().brush(QPalette::Active, QPalette::Text))
{
    setReadOnly(true);
    if (prop.hasNotifySignal()) {
        QMetaMethod signal = prop.notifySignal();
        QMetaMethod updateSlot = metaObject()->method(metaObject()->indexOfSlot("propertyChanged()"));
        connect(m_subject, signal, this, updateSlot);
    }
    propertyChanged();
}

QString PropertyField::valueToString(QVariant val)
{
    if (m_prop.isEnumType())
        return QString::fromUtf8(m_prop.enumerator().valueToKey(val.toInt()));

    QString text;
    switch (val.type()) {
    case QVariant::Double:
        text = QString("%1").arg(val.toReal(), 0, 'f', 4);
        break;
    case QVariant::Size:
        text = QString("%1 x %2").arg(val.toSize().width()).arg(val.toSize().height());
        break;
    case QVariant::SizeF:
        text = QString("%1 x %2").arg(val.toSizeF().width()).arg(val.toSizeF().height());
        break;
    case QVariant::Rect: {
        QRect rect = val.toRect();
        text = QString("%1 x %2 %3%4 %5%6").arg(rect.width())
                .arg(rect.height()).arg(rect.x() < 0 ? "" : "+").arg(rect.x())
                .arg(rect.y() < 0 ? "" : "+").arg(rect.y());
        } break;
    default:
        text = val.toString();
    }
    return text;
}

void PropertyField::propertyChanged()
{
    if (m_prop.isReadable()) {
        QVariant val = m_prop.read(m_subject);
        QString text = valueToString(val);
        QPalette modPalette = palette();

        // If we are seeing a value for the first time,
        // pretend it was that way for a while already.
        if (m_lastText.isEmpty()) {
            m_lastText = text;
            m_lastTextShowing = text;
        }

        qDebug() << "  " << QString::fromUtf8(m_prop.name()) << ':' << val;
        // If the value has recently changed, show the change
        if (text != m_lastText || m_lastChangeTime.elapsed() < 1000) {
            setText(m_lastTextShowing + " -> " + text);
            modPalette.setBrush(QPalette::Text, Qt::red);
            m_lastChangeTime.start();
            m_lastText = text;
        }
        // If the value hasn't changed recently, just show the current value
        else {
            setText(text);
            m_lastText = text;
            m_lastTextShowing = text;
            modPalette.setBrush(QPalette::Text, m_defaultBrush);
        }
        setPalette(modPalette);
    }
}
