#############################################################################
##
## Copyright (C) 2020 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is part of the test suite of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:GPL-EXCEPT$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3 as published by the Free Software
## Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################
"""Parsing the Locale Data Markup Language

It's an XML format, so the raw parsing of XML is, of course, delegated
to xml.dom.minidom; but it has its own specific schemata and some
funky rules for combining data from various files (inheritance between
locales). The use of it we're interested in is extraction of CLDR's
data, so some of the material here is specific to CLDR; see cldr.py
for how it is mainly used.

Provides various classes to wrap xml.dom's objects, specifically those
returned by minidom.parse() and their child-nodes:
  Node -- wraps any node in the DOM tree
  XmlScanner -- wraps the root element of a stand-alone XML file
  Supplement -- specializes XmlScanner for supplemental data files

See individual classes for further detail.
"""
from localetools import Error

class Node (object):
    """Wrapper for an arbitrary DOM node.

    Provides various ways to select chldren of a node. Selected child
    nodes are returned wrapped as Node objects.  A Node exposes the
    raw DOM node it wraps via its .dom attribute."""

    def __init__(self, elt):
        """Wraps a DOM node for ease of access.

        Single argument, elt, is the DOM node to wrap."""
        self.dom = elt

    def findAllChildren(self, tag, wanted = None):
        """All children that do have the given tag and attributes.

        First argument is the tag: children with any other tag are
        ignored.

        Optional second argument, wanted, should either be None or map
        attribute names to the values they must have. Only child nodes
        with these attributes set to the given values are yielded."""

        cutoff = 4 # Only accept approved, for now
        for child in self.dom.childNodes:
            if child.nodeType != child.ELEMENT_NODE:
                continue
            if child.nodeName != tag:
                continue

            try:
                draft = child.attributes['draft']
            except KeyError:
                pass
            else:
                if self.__draftScores.get(draft, 0) < cutoff:
                    continue

            if wanted is not None:
                try:
                    if wanted and any(child.attributes[k].nodeValue != v for k, v in wanted.items()):
                        continue
                except KeyError: # Some wanted attribute is missing
                    continue

            yield Node(child)

    __draftScores = dict(true = 0, unconfirmed = 1, provisional = 2,
                         contributed = 3, approved = 4, false = 4)

def _parseXPath(selector):
    # Split "tag[attr=val][...]" into tag-name and attribute mapping
    attrs = selector.split('[')
    name = attrs.pop(0)
    if attrs:
        attrs = [x.strip() for x in attrs]
        assert all(x.endswith(']') for x in attrs)
        attrs = [x[:-1].split('=') for x in attrs]
        assert all(len(x) in (1, 2) for x in attrs)
        attrs = (('type', x[0]) if len(x) == 1 else x for x in attrs)
    return name, dict(attrs)

def _iterateEach(iters):
    # Flatten a two-layer iterator.
    for it in iters:
        for item in it:
            yield item

class XmlScanner (object):
    """Wrap an XML file to enable XPath access to its nodes.
    """
    def __init__(self, node):
        self.root = node

    def findNodes(self, xpath):
        """Return all nodes under self.root matching this xpath"""
        elts = (self.root,)
        for selector in xpath.split('/'):
            tag, attrs = _parseXPath(selector)
            elts = tuple(_iterateEach(e.findAllChildren(tag, attrs) for e in elts))
            if not elts:
                break
        return elts

class Supplement (XmlScanner):
    # Replaces xpathlite.findTagsInFile()
    def find(self, xpath):
        elts = self.findNodes(xpath)
        for elt in _iterateEach(e.dom.childNodes if e.dom.childNodes else (e.dom,)
                                for e in elts):
            if elt.attributes:
                yield (elt.nodeName,
                       dict((k, v if isinstance(v, basestring) else v.nodeValue)
                            for k, v in elt.attributes.items()))
