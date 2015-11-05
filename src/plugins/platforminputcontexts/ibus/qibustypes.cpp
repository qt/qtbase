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

#include "qibustypes.h"
#include <QtDBus>
#include <QHash>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qtQpaInputMethods, "qt.qpa.input.methods")
Q_LOGGING_CATEGORY(qtQpaInputMethodsSerialize, "qt.qpa.input.methods.serialize")

QIBusSerializable::QIBusSerializable()
{
}

QIBusSerializable::~QIBusSerializable()
{
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QIBusSerializable &object)
{
    argument >> object.name;

    argument.beginMap();
    while (!argument.atEnd()) {
        argument.beginMapEntry();
        QString key;
        QDBusVariant value;
        argument >> key;
        argument >> value;
        argument.endMapEntry();
        object.attachments[key] = value.variant().value<QDBusArgument>();
    }
    argument.endMap();
    return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const QIBusSerializable &object)
{
    argument << object.name;

    argument.beginMap(qMetaTypeId<QString>(), qMetaTypeId<QDBusVariant>());

    QHashIterator<QString, QDBusArgument> i(object.attachments);
    while (i.hasNext()) {
        i.next();

        argument.beginMapEntry();
        argument << i.key();

        QDBusVariant variant(i.value().asVariant());

        argument << variant;
        argument.endMapEntry();
    }
    argument.endMap();
    return argument;
}

QIBusAttribute::QIBusAttribute()
    : type(Invalid),
      value(0),
      start(0),
      end(0)
{
    name = "IBusAttribute";
}

QIBusAttribute::~QIBusAttribute()
{
}

QDBusArgument &operator<<(QDBusArgument &argument, const QIBusAttribute &attribute)
{
    argument.beginStructure();

    argument << static_cast<const QIBusSerializable &>(attribute);

    quint32 t = (quint32) attribute.type;
    argument << t;
    argument << attribute.value;
    argument << attribute.start;
    argument << attribute.end;

    argument.endStructure();

    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QIBusAttribute &attribute)
{
    argument.beginStructure();

    argument >> static_cast<QIBusSerializable &>(attribute);

    quint32 t;
    argument >> t;
    attribute.type = (QIBusAttribute::Type) t;
    argument >> attribute.value;
    argument >> attribute.start;
    argument >> attribute.end;

    argument.endStructure();

    return argument;
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

QIBusAttributeList::~QIBusAttributeList()
{
}

QDBusArgument &operator<<(QDBusArgument &argument, const QIBusAttributeList &attrList)
{
    argument.beginStructure();

    argument << static_cast<const QIBusSerializable &>(attrList);

    argument.beginArray(qMetaTypeId<QDBusVariant>());
    for (int i = 0; i < attrList.attributes.size(); ++i) {
        QVariant variant;
        variant.setValue(attrList.attributes.at(i));
        argument << QDBusVariant (variant);
    }
    argument.endArray();

    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, QIBusAttributeList &attrList)
{
    qCDebug(qtQpaInputMethodsSerialize) << "QIBusAttributeList::fromDBusArgument()" << arg.currentSignature();
    arg.beginStructure();

    arg >> static_cast<QIBusSerializable &>(attrList);

    arg.beginArray();
    while (!arg.atEnd()) {
        QDBusVariant var;
        arg >> var;

        QIBusAttribute attr;
        var.variant().value<QDBusArgument>() >> attr;
        attrList.attributes.append(attr);
    }
    arg.endArray();

    arg.endStructure();
    return arg;
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

QIBusText::~QIBusText()
{
}

QDBusArgument &operator<<(QDBusArgument &argument, const QIBusText &text)
{
    argument.beginStructure();

    argument << static_cast<const QIBusSerializable &>(text);

    argument << text.text << text.attributes;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QIBusText &text)
{
    qCDebug(qtQpaInputMethodsSerialize) << "QIBusText::fromDBusArgument()" << argument.currentSignature();
    argument.beginStructure();

    argument >> static_cast<QIBusSerializable &>(text);

    argument >> text.text;
    QDBusVariant variant;
    argument >> variant;
    variant.variant().value<QDBusArgument>() >> text.attributes;

    argument.endStructure();
    return argument;
}

QIBusEngineDesc::QIBusEngineDesc()
    : engine_name(""),
      longname(""),
      description(""),
      language(""),
      license(""),
      author(""),
      icon(""),
      layout(""),
      rank(0),
      hotkeys(""),
      symbol(""),
      setup(""),
      layout_variant(""),
      layout_option(""),
      version(""),
      textdomain(""),
      iconpropkey("")
{
    name = "IBusEngineDesc";
}

QIBusEngineDesc::~QIBusEngineDesc()
{
}

QDBusArgument &operator<<(QDBusArgument &argument, const QIBusEngineDesc &desc)
{
    argument.beginStructure();

    argument << static_cast<const QIBusSerializable &>(desc);

    argument << desc.engine_name;
    argument << desc.longname;
    argument << desc.description;
    argument << desc.language;
    argument << desc.license;
    argument << desc.author;
    argument << desc.icon;
    argument << desc.layout;
    argument << desc.rank;
    argument << desc.hotkeys;
    argument << desc.symbol;
    argument << desc.setup;
    argument << desc.layout_variant;
    argument << desc.layout_option;
    argument << desc.version;
    argument << desc.textdomain;
    argument << desc.iconpropkey;

    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QIBusEngineDesc &desc)
{
    qCDebug(qtQpaInputMethodsSerialize) << "QIBusEngineDesc::fromDBusArgument()" << argument.currentSignature();
    argument.beginStructure();

    argument >> static_cast<QIBusSerializable &>(desc);

    argument >> desc.engine_name;
    argument >> desc.longname;
    argument >> desc.description;
    argument >> desc.language;
    argument >> desc.license;
    argument >> desc.author;
    argument >> desc.icon;
    argument >> desc.layout;
    argument >> desc.rank;
    argument >> desc.hotkeys;
    argument >> desc.symbol;
    argument >> desc.setup;
    // Previous IBusEngineDesc supports the arguments between engine_name
    // and setup.
    if (argument.currentSignature() == "") {
        argument.endStructure();
        return argument;
    }
    argument >> desc.layout_variant;
    argument >> desc.layout_option;
    // Previous IBusEngineDesc supports the arguments between engine_name
    // and layout_option.
    if (argument.currentSignature() == "") {
        argument.endStructure();
        return argument;
    }
    argument >> desc.version;
    if (argument.currentSignature() == "") {
        argument.endStructure();
        return argument;
    }
    argument >> desc.textdomain;
    if (argument.currentSignature() == "") {
        argument.endStructure();
        return argument;
    }
    argument >> desc.iconpropkey;

    argument.endStructure();
    return argument;
}

QT_END_NAMESPACE
