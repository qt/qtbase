// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <iostream>
#include <cstring>
#include <QFile>
#include <QDomDocument>
#include <QDomImplementation>

void NodeElements();
void DomText();
void FirstElement();
void FileContent();
void DocAppend();
void XML_snippet_main();

using namespace std;
//! [0]
void XML_snippet_main()
{
QDomDocument doc;
QDomImplementation impl;
// This will create the element, but the resulting XML document will
// be invalid, because '~' is not a valid character in a tag name.
impl.setInvalidDataPolicy(QDomImplementation::AcceptInvalidChars);
QDomElement elt1 = doc.createElement("foo~bar");

// This will create an element with the tag name "foobar".
impl.setInvalidDataPolicy(QDomImplementation::DropInvalidChars);
QDomElement elt2 = doc.createElement("foo~bar");

// This will create a null element.
impl.setInvalidDataPolicy(QDomImplementation::ReturnNullNode);
QDomElement elt3 = doc.createElement("foo~bar");
}
//! [0]

void NodeElements()
{
//! [1]
QDomDocument d;
QString someXML;

d.setContent(someXML);
QDomNode n = d.firstChild();
while (!n.isNull()) {
    if (n.isElement()) {
        QDomElement e = n.toElement();
        cout << "Element name: " << qPrintable(e.tagName()) << '\n';
        break;
    }
    n = n.nextSibling();
}
//! [1]

//! [2]
QDomDocument document;
QDomElement element1 = document.documentElement();
QDomElement element2 = element1;
//! [2]

//! [3]
QDomElement element3 = document.createElement("MyElement");
QDomElement element4 = document.createElement("MyElement");
//! [3]

//! [8]
QDomElement e;
//...
QDomAttr a = e.attributeNode("href");
cout << qPrintable(a.value()) << '\n';   // prints "http://qt-project.org"
a.setValue("http://qt-project.org/doc"); // change the node's attribute
QDomAttr a2 = e.attributeNode("href");
cout << qPrintable(a2.value()) << '\n';  // prints "http://qt-project.org/doc"
//! [8]
}

void DomText()
{
QDomDocument doc;
//! [10]
QString text;
QDomElement element = doc.documentElement();
for(QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling())
{
    QDomText t = n.toText();
    if (!t.isNull())
        text += t.data();
}
//! [10]

}

void FileContent()
{
//! [16]
QDomDocument doc("mydocument");
QFile file("mydocument.xml");
if (!file.open(QIODevice::ReadOnly))
    return;
if (!doc.setContent(&file)) {
    file.close();
    return;
}
file.close();

// print out the element names of all elements that are direct children
// of the outermost element.
QDomElement docElem = doc.documentElement();

QDomNode n = docElem.firstChild();
while(!n.isNull()) {
    QDomElement e = n.toElement(); // try to convert the node to an element.
    if(!e.isNull()) {
        cout << qPrintable(e.tagName()) << '\n'; // the node really is an element.
    }
    n = n.nextSibling();
}

// Here we append a new element to the end of the document
QDomElement elem = doc.createElement("img");
elem.setAttribute("src", "myimage.png");
docElem.appendChild(elem);
//! [16]
}

void DocAppend()
{
//! [17]
QDomDocument doc;
QDomElement root = doc.createElement("MyML");
doc.appendChild(root);

QDomElement tag = doc.createElement("Greeting");
root.appendChild(tag);

QDomText t = doc.createTextNode("Hello World");
tag.appendChild(t);

QString xml = doc.toString();
//! [17]
}
