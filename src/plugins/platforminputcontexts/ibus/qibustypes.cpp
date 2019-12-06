/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qibustypes.h"
#include <QtDBus>
#include <QHash>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qtQpaInputMethods, "qt.qpa.input.methods")
Q_LOGGING_CATEGORY(qtQpaInputMethodsSerialize, "qt.qpa.input.methods.serialize")

QIBusSerializable::QIBusSerializable()
{
}

void QIBusSerializable::deserializeFrom(const QDBusArgument &argument)
{
    argument >> name;

    argument.beginMap();
    while (!argument.atEnd()) {
        argument.beginMapEntry();
        QString key;
        QDBusVariant value;
        argument >> key;
        argument >> value;
        argument.endMapEntry();
        attachments[key] = qvariant_cast<QDBusArgument>(value.variant());
    }
    argument.endMap();
}

void QIBusSerializable::serializeTo(QDBusArgument &argument) const
{
    argument << name;

    argument.beginMap(qMetaTypeId<QString>(), qMetaTypeId<QDBusVariant>());

    for (auto i = attachments.begin(), end = attachments.end(); i != end; ++i) {
        argument.beginMapEntry();
        argument << i.key();

        QDBusVariant variant(i.value().asVariant());

        argument << variant;
        argument.endMapEntry();
    }
    argument.endMap();
}

QIBusAttribute::QIBusAttribute()
    : type(Invalid),
      value(0),
      start(0),
      end(0)
{
    name = "IBusAttribute";
}

void QIBusAttribute::serializeTo(QDBusArgument &argument) const
{
    argument.beginStructure();

    QIBusSerializable::serializeTo(argument);

    quint32 t = (quint32) type;
    argument << t;
    argument << value;
    argument << start;
    argument << end;

    argument.endStructure();
}

void QIBusAttribute::deserializeFrom(const QDBusArgument &argument)
{
    argument.beginStructure();

    QIBusSerializable::deserializeFrom(argument);

    quint32 t;
    argument >> t;
    type = (QIBusAttribute::Type) t;
    argument >> value;
    argument >> start;
    argument >> end;

    argument.endStructure();
}

QTextCharFormat QIBusAttribute::format() const
{
    QTextCharFormat fmt;
    switch (type) {
    case Invalid:
        break;
    case Underline: {
        QTextCharFormat::UnderlineStyle style = QTextCharFormat::NoUnderline;

        switch (value) {
        case UnderlineNone:
            break;
        case UnderlineSingle:
            style = QTextCharFormat::SingleUnderline;
            break;
        case UnderlineDouble:
            style = QTextCharFormat::DashUnderline;
            break;
        case UnderlineLow:
            style = QTextCharFormat::DashDotLine;
            break;
        case UnderlineError:
            style = QTextCharFormat::WaveUnderline;
            fmt.setUnderlineColor(Qt::red);
            break;
        }

        fmt.setUnderlineStyle(style);
        break;
    }
    case Foreground:
        fmt.setForeground(QColor(value));
        break;
    case Background:
        fmt.setBackground(QColor(value));
        break;
    }
    return fmt;
}

QIBusAttributeList::QIBusAttributeList()
{
    name = "IBusAttrList";
}

void QIBusAttributeList::serializeTo(QDBusArgument &argument) const
{
    argument.beginStructure();

    QIBusSerializable::serializeTo(argument);

    argument.beginArray(qMetaTypeId<QDBusVariant>());
    for (int i = 0; i < attributes.size(); ++i) {
        QVariant variant;
        variant.setValue(attributes.at(i));
        argument << QDBusVariant (variant);
    }
    argument.endArray();

    argument.endStructure();
}

void QIBusAttributeList::deserializeFrom(const QDBusArgument &arg)
{
    qCDebug(qtQpaInputMethodsSerialize) << "QIBusAttributeList::fromDBusArgument()" << arg.currentSignature();

    arg.beginStructure();

    QIBusSerializable::deserializeFrom(arg);

    arg.beginArray();
    while (!arg.atEnd()) {
        QDBusVariant var;
        arg >> var;

        QIBusAttribute attr;
        qvariant_cast<QDBusArgument>(var.variant()) >> attr;
        attributes.append(std::move(attr));
    }
    arg.endArray();

    arg.endStructure();
}

QList<QInputMethodEvent::Attribute> QIBusAttributeList::imAttributes() const
{
    QHash<QPair<int, int>, QTextCharFormat> rangeAttrs;
    const int numAttributes = attributes.size();

    // Merge text fomats for identical ranges into a single QTextFormat.
    for (int i = 0; i < numAttributes; ++i) {
        const QIBusAttribute &attr = attributes.at(i);
        const QTextCharFormat &format = attr.format();

        if (format.isValid()) {
            const QPair<int, int> range(attr.start, attr.end);
            rangeAttrs[range].merge(format);
        }
    }

    // Assemble list in original attribute order.
    QList<QInputMethodEvent::Attribute> imAttrs;
    imAttrs.reserve(numAttributes);

    for (int i = 0; i < numAttributes; ++i) {
        const QIBusAttribute &attr = attributes.at(i);
        const QTextFormat &format = attr.format();

        imAttrs += QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat,
            attr.start,
            attr.end - attr.start,
            format.isValid() ? rangeAttrs[QPair<int, int>(attr.start, attr.end)] : format);
    }

    return imAttrs;
}

QIBusText::QIBusText()
{
    name = "IBusText";
}

void QIBusText::serializeTo(QDBusArgument &argument) const
{
    argument.beginStructure();

    QIBusSerializable::serializeTo(argument);

    argument << text << attributes;
    argument.endStructure();
}

void QIBusText::deserializeFrom(const QDBusArgument &argument)
{
    qCDebug(qtQpaInputMethodsSerialize) << "QIBusText::fromDBusArgument()" << argument.currentSignature();

    argument.beginStructure();

    QIBusSerializable::deserializeFrom(argument);

    argument >> text;
    QDBusVariant variant;
    argument >> variant;
    qvariant_cast<QDBusArgument>(variant.variant()) >> attributes;

    argument.endStructure();
}

QIBusEngineDesc::QIBusEngineDesc()
    : rank(0)
{
    name = "IBusEngineDesc";
}

void QIBusEngineDesc::serializeTo(QDBusArgument &argument) const
{
    argument.beginStructure();

    QIBusSerializable::serializeTo(argument);

    argument << engine_name;
    argument << longname;
    argument << description;
    argument << language;
    argument << license;
    argument << author;
    argument << icon;
    argument << layout;
    argument << rank;
    argument << hotkeys;
    argument << symbol;
    argument << setup;
    argument << layout_variant;
    argument << layout_option;
    argument << version;
    argument << textdomain;
    argument << iconpropkey;

    argument.endStructure();
}

void QIBusEngineDesc::deserializeFrom(const QDBusArgument &argument)
{
    qCDebug(qtQpaInputMethodsSerialize) << "QIBusEngineDesc::fromDBusArgument()" << argument.currentSignature();
    argument.beginStructure();

    QIBusSerializable::deserializeFrom(argument);

    argument >> engine_name;
    argument >> longname;
    argument >> description;
    argument >> language;
    argument >> license;
    argument >> author;
    argument >> icon;
    argument >> layout;
    argument >> rank;
    argument >> hotkeys;
    argument >> symbol;
    argument >> setup;
    // Previous IBusEngineDesc supports the arguments between engine_name
    // and setup.
    if (argument.currentSignature() == "")
        goto olderThanV2;
    argument >> layout_variant;
    argument >> layout_option;
    // Previous IBusEngineDesc supports the arguments between engine_name
    // and layout_option.
    if (argument.currentSignature() == "")
        goto olderThanV3;
    argument >> version;
    if (argument.currentSignature() == "")
        goto olderThanV4;
    argument >> textdomain;
    if (argument.currentSignature() == "")
        goto olderThanV5;
    argument >> iconpropkey;
    // <-- insert new member streaming here (1/2)
    goto newest;
olderThanV2:
    layout_variant.clear();
    layout_option.clear();
olderThanV3:
    version.clear();
olderThanV4:
    textdomain.clear();
olderThanV5:
    iconpropkey.clear();
    // <-- insert new members here (2/2)
newest:
    argument.endStructure();
}

QT_END_NAMESPACE
