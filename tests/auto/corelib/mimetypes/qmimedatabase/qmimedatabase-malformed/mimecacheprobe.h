// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MIMECACHEPROBE_H
#define MIMECACHEPROBE_H

#include <QtCore/QString>

// Query kinds understood by the probe process. Each drives a different family
// of lookups in QMimeBinaryProvider:
//   content  - MatchContent sniff          -> findByMagic()/matchMagicRule()
//   filename - MatchDefault lookup by name -> addFileNameMatches()
//   name     - mimeTypeForName() plus every QMimeType property that consults a
//              provider again -> resolveAlias(), addParents(), addAliases(),
//              icon(), genericIcon()
// Only "content" and "filename" need a file on disk to look at.
namespace QueryCase {
static constexpr QLatin1StringView Content("content");
static constexpr QLatin1StringView FileName("filename");
static constexpr QLatin1StringView Name("name");
} // namespace QueryCase

// The mimetype the "name" case looks up. Crafted cache entries that need to be
// found by that lookup must use the same string.
static constexpr char ProbedMimeTypeName[] = "application/octet-stream";

#endif   // MIMECACHEPROBE_H
