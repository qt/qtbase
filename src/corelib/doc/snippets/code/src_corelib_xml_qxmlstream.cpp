// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
  QXmlStreamReader xml;
  ...
  while (!xml.atEnd()) {
        xml.readNext();
        ... // do processing
  }
  if (xml.hasError()) {
        ... // do error handling
  }
//! [0]


//! [1]
        writeStartElement(qualifiedName);
        writeCharacters(text);
        writeEndElement();
//! [1]


//! [2]
        writeStartElement(namespaceUri, name);
        writeCharacters(text);
        writeEndElement();
//! [2]


