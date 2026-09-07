#!/usr/bin/env python3
# Copyright (C) 2024 Klaralvdalens Datakonsult AB (KDAB)
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# The tika database at https://raw.githubusercontent.com/apache/tika/main/tika-core/src/main/resources/org/apache/tika/mime/tika-mimetypes.xml
# uses a format that is *close* to the freedesktop.org shared-mime-info format, but not quite. They extended it with
# - match types such as stringignorecase, regex, unicodeLE
# - 0x syntax for strings
# - making the type and offset attributes optional
# - <match minShouldMatch="2"> to specify that at least two of the submatches should match
# It's also missing some crucial mimetypes that we need.
# This script fixes all that.

# Usage: download tika-mimetypes.xml as tika-mimetypes.xml.orig and run this script to produce Qt's version of tika-mimetypes.xml
# This is automated by update_tika_from_upstream.sh

import sys, re

inputfile = "tika-mimetypes.xml.orig"
file = "tika-mimetypes.xml"

def wrap_with_comment(line):
    if not '<!--' in line:
        return "<!-- DISABLED FOR QT: " + line.rstrip('\n') + "-->\n"
    return line

def transform_hex_value(line):
    match = re.search(r'value="0x([0-9a-fA-F]+)"', line)
    if match:
        hex_value = match.group(1)
        # Insert \\x before every pair of hex characters
        transformed_value = ''.join(f'\\x{hex_value[i:i+2]}' for i in range(0, len(hex_value), 2))
        line = re.sub(r'value="0x[0-9a-fA-F]+"', lambda m: f'value="{transformed_value}"', line)
    return line

def open_input_file(inputfile):
    try:
        return open(inputfile, "r")
    except FileNotFoundError:
        sys.exit(f"{inputfile} not found")
    except OSError as e:
        sys.exit(f"OS error when opening {inputfile}: {e}")

# Remove the commented-out parts of a line, so that disabled elements are ignored.
def strip_comments(line, in_comment):
    out = ''
    while line:
        if in_comment:
            end = line.find('-->')
            if end < 0:
                return out, True
            line = line[end + 3:]
            in_comment = False
        else:
            start = line.find('<!--')
            if start < 0:
                return out + line, False
            out += line[:start]
            line = line[start + 4:]
            in_comment = True
    return out, in_comment


# The spec says a magic rule for a generic type should use a lower priority than
# the rules of its subtypes, so that the subtype wins. Tika does not do that: many
# types share the default priority 50 with their sub-class-of children (e.g.
# application/xml and image/svg+xml, or application/x-elf and application/x-sharedlib),
# which makes magic matching pick either one. Lower each parent below its children.
def lower_generic_priorities(lines):
    magic_re = re.compile(r'<magic(?:\s+priority\s*=\s*"(\d+)")?\s*>')
    type_re = re.compile(r'<mime-type type="([^"]+)"')
    parent_re = re.compile(r'<sub-class-of type="([^"]+)"')
    alias_re = re.compile(r'<alias type="([^"]+)"')

    # Qt additions are appended as one multi-line string, re-split to index by line
    lines = ''.join(lines).splitlines(keepends=True)

    magics = {}   # mime type -> line indexes of its <magic> elements
    parents = {}  # mime type -> parent types
    aliases = {}  # alias -> mime type
    priority = {} # line index -> priority
    current = ''
    in_comment = False
    for i, full_line in enumerate(lines):
        line, in_comment = strip_comments(full_line, in_comment)
        match = type_re.search(line)
        if match:
            current = match.group(1)
        match = alias_re.search(line)
        if match:
            aliases[match.group(1)] = current
        match = parent_re.search(line)
        if match:
            parents.setdefault(current, []).append(match.group(1))
        match = magic_re.search(line)
        if match:
            magics.setdefault(current, []).append(i)
            priority[i] = int(match.group(1)) if match.group(1) else 50

    children = {}
    for mime_type, mime_parents in parents.items():
        for parent in mime_parents:
            children.setdefault(aliases.get(parent, parent), []).append(mime_type)

    def max_priority(mime_type):
        return max((priority[i] for i in magics.get(mime_type, [])), default=None)

    changed = True
    while changed:
        changed = False
        for mime_type, indexes in magics.items():
            child_priorities = [max_priority(c) for c in children.get(mime_type, [])]
            child_priorities = [p for p in child_priorities if p is not None]
            if not child_priorities:
                continue
            cap = max(min(child_priorities) - 1, 0)
            for i in indexes:
                if priority[i] > cap:
                    priority[i] = cap
                    changed = True

    for i, prio in priority.items():
        match = magic_re.search(lines[i])
        if (int(match.group(1)) if match.group(1) else 50) == prio:
            continue
        lines[i] = magic_re.sub('<magic priority="%d"> <!-- priority lowered by Qt -->' % prio,
                                lines[i], count=1)
    return lines


with open_input_file(inputfile) as f:
    with open(file, "w") as out:

        lines = []
        current_mime_type = ''
        skip_until = None
        skip_indent = None
        for line in f:

            line = line.replace('\t', '  ') # Untabify
            line = re.sub(r' +\n$', '\n', line) # Remove trailing whitespace

            match = re.search(r'<mime-type type="([^"]+)">', line)
            if match:
                current_mime_type = match.group(1)

            # remove mimetypes with regex matches having submatches, just commenting out the line would break the xml structure
            if current_mime_type == "application/marc" or current_mime_type == "application/x-touhou" or ';format' in current_mime_type:
                # line = wrap_with_comment(line) # leads to issues with -- inside comments
                continue

            if 'be a bit more flexible, but require one from each of these' in line:
              skip_until = '</magic>'
            if skip_until is None and 'minShouldMatch="' in line:
                skip_until = '</match>'
                skip_indent = len(line) - len(line.lstrip())
            if 'value="OggS\\000' in line: # off by one in mask length, it seems (audio/x-oggflac and following)
                skip_until = '</magic>'

            if skip_until is not None:
                if skip_until in line:
                    if skip_indent is not None: # Indentation tracking for nested elements
                        indent = len(line) - len(line.lstrip())
                        if indent <= skip_indent: # Match closing tag at same or outer indentation level
                            skip_indent = None
                            skip_until = None
                        line = wrap_with_comment(line)
                    else: # No indentation tracking
                        skip_until = None
                else:
                    line = wrap_with_comment(line)

            # uncomment "conflicting" globs
            pattern = re.compile(r'( *)<!-- (<glob pattern="[^"]*"/>)(.*?)conflicts with(.*?)-->')
            match = pattern.search(line)
            if match:
                line = match.group(1) + match.group(2) + ' <!-- Re-enabled by Qt -->\n'

            if not '<!--' in line:
                line = line.replace("_comment>", "comment>")
                line = line.replace('type="stringignorecase"', 'type="string"') # tika extension, not supported
                line = line.replace('value="P4" type="regex"', 'value="P4" type="string"') # no need to be a regex (image/x-portable-bitmap)
                line = line.replace('value="P5" type="regex"', 'value="P5" type="string"') # no need to be a regex (image/x-portable-graymap)
                line = line.replace('value="P6" type="regex"', 'value="P6" type="string"') # no need to be a regex (image/x-portable-pixmap)
                line = line.replace('value="(X|DKIM|ARC)-" type="regex"', 'value="X-" type="string"') # simplify message/rfc822 to avoid regex

                # No type specified, assumed string
                if '<match value=' in line and '>' in line and not 'type=' in line:
                    line = line.replace('<match ', '<match type="string" ')
                    line = line.replace('>', '> <!-- type added by Qt -->')

                # No offset specified, spell out 0
                if '<match value=' in line and '>' in line and not 'offset=' in line:
                    line = line.replace('<match ', '<match offset="0" ')
                    line = line.replace('>', '> <!-- offset added by Qt -->')

                # tika extensions, not supported
                if 'type="unicodeLE"' in line or 'type="regex"' in line or '<match value="(?:\\\\x' in line:
                    line = wrap_with_comment(line)

                if '<glob pattern="*.c"/>' in line.lower():
                    line = line.replace('/', ' case-sensitive="true"/')
                    line = line.replace('>', '> <!-- case-sensitive added by Qt -->')

                # Add aliases needed by the unittest
                if '<mime-type type="audio/mpeg"' in line:
                    line += '    <alias type="audio/mp3"/> <!-- added by Qt -->\n';
                if '<mime-type type="text/x-diff"' in line:
                    line += '    <alias type="text/x-patch"/> <!-- added by Qt -->\n';
                if '<mime-type type="application/x-sh"' in line:
                    line += '    <alias type="application/x-shellscript"/> <!-- added by Qt -->\n';
                    line += '    <sub-class-of type="application/x-executable"/> <!-- added by Qt -->\n';

                # Tika supports 0x syntax for strings, shared-mime-info requires escaping each char with \x
                if 'value="0x' in line and 'type="string"' in line:
                    line = transform_hex_value(line)

                # Need more tolerance in SVG magic
                if '&lt;svg' in line and 'offset="0"' in line:
                    line = line.replace('offset="0"', 'offset="0:100"')

            if '<mime-info xmlns' in line:
                line += """
  <!-- Qt additions START -->

  <mime-type type="application/x-zerosize">
    <comment>Empty document</comment>
  </mime-type>

  <mime-type type="application/x-desktop">
    <comment>Desktop file</comment>
     <glob pattern="*.desktop"/>
  </mime-type>

  <mime-type type="text/x-qml">
    <comment>Qt Markup Language file</comment>
    <sub-class-of type="text/plain"/>
    <magic>
      <match type="string" value="/bin/env qml" offset="2:16"/>
      <match type="string" value="import Qt" offset="0:3000">
        <match type="string" value="{" offset="9:3009"/>
      </match>
      <match type="string" value="import Qml" offset="0:3000">
        <match type="string" value="{" offset="9:3009"/>
      </match>
    </magic>
    <glob pattern="*.qml"/>
    <glob pattern="*.qmltypes"/>
    <glob pattern="*.qmlproject"/>
  </mime-type>

  <mime-type type="application/x-gzpostscript">
    <comment>Compressed postscript</comment>
     <glob pattern="*.ps.gz"/>
  </mime-type>

  <mime-type type="application/x-core">
    <comment>Core dump</comment>
    <glob pattern="core" case-sensitive="true"/>
  </mime-type>

  <mime-type type="application/x-bzip2-compressed-tar">
    <comment>BZip2 compressed tar file</comment>
    <glob pattern="*.tar.bz2"/>
  </mime-type>

  <mime-type type="inode/directory">
    <comment>Directory</comment>
    <generic-icon name="folder"/>
  </mime-type>

  <!-- Qt additions END -->
"""
            lines.append(line)

        for line in lower_generic_priorities(lines):
            out.write(line)
