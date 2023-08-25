#!/bin/env python3
# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
"""Digest cbor-tags.xml file into code for insertion into main.cpp

See main.cpp's comment on how to regenerate its GENERATED CODE.
See ./cbortag.py --help for further details on how to invoke.
You can import this is a module without invoking the script.
"""

def firstChild(parent, tag):
    """Return parent's first child element with the given tag."""
    return next(node for node in parent.childNodes
                if node.nodeType == parent.ELEMENT_NODE and node.nodeName == tag)

def nodeAttrIs(node, attr, seek):
    """Checks whether the node has a given value for an attribute

    Takes the node to check, the name of the attribute and the value
    to check against.  Returns true if the node does have that value
    for the named attribute."""
    if node.nodeType != node.ELEMENT_NODE:
        return False
    if node.attributes is None or attr not in node.attributes:
        return False
    return node.attributes[attr].value == seek

def getRfcValue(node):
    """Extract RFC reference from an <xref type="rfc" ...> element

    Some of these have a reference including section details as the
    body of the element, otherwise the data attribute should identify
    the RFC. If neither is found, an empty string is returned."""
    if node.childNodes:
        return node.childNodes[0].nodeValue # Maybe accumulate several children ?
    if node.attributes is None or 'data' not in node.attributes:
        return ''
    return node.attributes['data'].value

def readRegistry(filename):
    """Handles the XML parsing and returns the relevant parts.

    Single argument is the path to the cbor-tags.xml file; returns a
    twople of the title element's text and an interator over the
    record nodes.  Checks some things are as expected while doing so."""
    from xml.dom.minidom import parse
    doc = parse(filename).documentElement
    assert nodeAttrIs(doc, 'id', 'cbor-tags')
    title = firstChild(doc, 'title').childNodes[0].nodeValue
    registry = firstChild(doc, 'registry')
    assert nodeAttrIs(registry, 'id', 'tags')
    records = (node for node in registry.childNodes if node.nodeName == 'record')
    return title, records

def digest(record):
    """Digest a single record from cbor-tags.xml

    If the record is not of interest, returns the twople (None, None).
    For records of interest, returns (n, t) where n is the numeric tag
    code of the record and t is a text describing it. If the record,
    or its semantics field, has an xref child with type="rfc", the RFC
    mentioned there is included with the text of the semantics; such a
    record is of interest, provided it has a semantics field and no
    dash in its value.  Records with a value field containing a dash
    (indicating a range) are not of interest. Records with a value of
    256 or above are only of interest if they include an RFC."""
    data = {}
    for kid in record.childNodes:
        if kid.nodeName == 'xref':
            if not nodeAttrIs(kid, 'type', 'rfc'):
                continue
            rfc = getRfcValue(kid)
            if rfc:
                # Potentially stomping one taken from semantics
                data['rfc'] = rfc
        elif kid.nodeName == 'semantics':
            text = rfc = ''
            for part in kid.childNodes:
                if part.nodeType == kid.TEXT_NODE:
                    text += part.nodeValue
                elif part.nodeType == kid.ELEMENT_NODE:
                    if part.nodeName != 'xref' or not nodeAttrIs(part, 'type', 'rfc'):
                        continue # potentially append content to text
                    assert not rfc, ('Duplicate RFC ?', rfc, part)
                    rfc = getRfcValue(part)
            if rfc:
                if text.endswith('()'):
                    text = text[:-2].rstrip()
                if 'rfc' not in data:
                    data['rfc'] = rfc
            data['semantics'] = ' '.join(text.split())
        elif kid.nodeName == 'value':
            data['value'] = kid.childNodes[0].nodeValue
    text = data.get('semantics')
    if not text or 'value' not in data or '-' in data['value']:
        return None, None
    value = int(data['value'])
    if 'rfc' in data:
        rfc = data["rfc"].replace('rfc', 'RFC')
        text = f'{text} [{rfc}]'
    elif value >= 256:
        return None, None
    return value, text

def entries(records):
    """Digest each record of interest into a value and text.

    The value and text form the raw material of the tagDescriptions
    array in main.cpp; see digest for which records are retained."""
    for record in records:
        value, text = digest(record)
        if value is not None:
            yield value, text

def marginBound(text, prior, left, right):
    """Split up a string literal for tidy display.

    The first parameter, text, is the content of the string literal;
    quotes shall be added.  It may be split into several fragments,
    each quoted, so as to abide by line length constraints.

    The remaining parameters are integers: prior is the text already
    present on the line before text is to be added; left is the width
    of the left margin for all subsequent lines; and right is the
    right margin to stay within, where possible. The returned string
    is either a space with the whole quoted text following, to fit on
    the line already started to length prior, or a sequence of quoted
    strings, each preceded by a newline and indent of width left."""
    if prior + 3 + len(text) < right: # 1 for space, 2 for quotes
        return f' "{text}"'
    width = right - left - 2 # 2 for the quotes
    words = iter(text.split(' '))
    lines, current = [''], [next(words)]
    for word in words:
        if len(word) + sum(len(w) + 1 for w in current) > width:
            line = ' '.join(current)
            lines.append(f'"{line}"')
            current = ['', word]
        else:
            current.append(word)
    line = ' '.join(current)
    lines.append(f'"{line}"')
    return ('\n' + ' ' * left).join(lines)

def main(argv, speak):
    """Takes care of driving the process.

    Takes the command-line argument list (whose first entry is the
    name of this script) and standard output (or compatible stream of
    your choosing) to which to write data. If the --out option is
    specified in the arguments, the file it names is used in place of
    this output stream."""
    from argparse import ArgumentParser, ArgumentDefaultsHelpFormatter
    parser = ArgumentParser(
        description='Digest cbor-tags.xml into code to insert in main.cpp',
        formatter_class=ArgumentDefaultsHelpFormatter)
    parser.add_argument('path', help='path of the cbor-tags.xml file',
                        default='cbor-tags.xml')
    parser.add_argument('--out', help='file to write instead of standard output')
    args = parser.parse_args(argv[1:])
    emit = (open(args.out) if args.out else speak).write

    title, records = readRegistry(args.path)
    emit(f"""\
struct CborTagDescription
{{
    QCborTag tag;
    const char *description; // with space and parentheses
}};

// {title}
static const CborTagDescription tagDescriptions[] = {{
    // from https://www.iana.org/assignments/cbor-tags/cbor-tags.xhtml
""")

    for value, text in sorted(entries(records)):
        prior = f'    {{ QCborTag({value}),'
        body = marginBound(f' ({text})', len(prior), 6, 96)
        emit(f"{prior}{body} }},\n")

    emit("""\
    { QCborTag(-1), nullptr }
};
""")

if __name__ == '__main__':
    import sys
    sys.exit(main(sys.argv, sys.stdout))
