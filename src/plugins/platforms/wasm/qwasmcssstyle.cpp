// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmcssstyle.h"

#include "qwasmbase64iconstore.h"

#include <QtCore/qstring.h>
#include <QtCore/qfile.h>

QT_BEGIN_NAMESPACE

namespace {
const char *Style = R"css(
.qt-screen {
    --border-width: 4px;
    --resize-outline-width: 8px;
    --resize-outline-half-width: var(--resize-outline-width) / 2;

    position: relative;
    border: none;
    caret-color: transparent;
    cursor: default;
    width: 100%;
    height: 100%;
    overflow: hidden;
}

.qt-window {
    box-shadow: rgb(0 0 0 / 20%) 0px 10px 16px 0px, rgb(0 0 0 / 19%) 0px 6px 20px 0px;
    position: absolute;
    background-color: lightgray;
}

.qt-window.has-title-bar {
    border: var(--border-width) solid lightgray;
    caret-color: transparent;
}

.resize-outline {
    position: absolute;
    display: none;
}

.qt-window.has-title-bar:not(.maximized) .resize-outline {
    display: block;
}

.resize-outline.nw {
    left: calc(-1 * var(--resize-outline-half-width) - var(--border-width));
    top: calc(-1 * var(--resize-outline-half-width) - var(--border-width));
    width: var(--resize-outline-width);
    height: var(--resize-outline-width);
    cursor: nwse-resize;
}

.resize-outline.n {
    left: var(--resize-outline-half-width);
    top: calc(-1 * var(--resize-outline-half-width) - var(--border-width));
    height: var(--resize-outline-width);
    width: calc(100% + 2 * var(--border-width) - var(--resize-outline-width));
    cursor: ns-resize;
}

.resize-outline.ne {
    left: calc(100% + var(--border-width) - var(--resize-outline-half-width));
    top: calc(-1 * var(--resize-outline-half-width) - var(--border-width));
    width: var(--resize-outline-width);
    height: var(--resize-outline-width);
    cursor: nesw-resize;
}

.resize-outline.w {
    left: calc(-1 * var(--resize-outline-half-width) - var(--border-width));
    top: 0;
    height: calc(100% + 2 * var(--border-width) - var(--resize-outline-width));
    width: var(--resize-outline-width);
    cursor: ew-resize;
}

.resize-outline.e {
    left: calc(100% + var(--border-width) - var(--resize-outline-half-width));
    top: 0;
    height: calc(100% + 2 * var(--border-width) - var(--resize-outline-width));
    width: var(--resize-outline-width);
    cursor: ew-resize;
}

.resize-outline.sw {
    left: calc(-1 * var(--resize-outline-half-width) - var(--border-width));
    top: calc(100% + var(--border-width) - var(--resize-outline-half-width));
    width: var(--resize-outline-width);
    height: var(--resize-outline-width);
    cursor: nesw-resize;
}

.resize-outline.s {
    left: var(--resize-outline-half-width);
    top: calc(100% + var(--border-width) - var(--resize-outline-half-width));
    height: var(--resize-outline-width);
    width: calc(100% + 2 * var(--border-width) - var(--resize-outline-width));
    cursor: ns-resize;
}

.resize-outline.se {
    left: calc(100% + var(--border-width) - var(--resize-outline-half-width));
    top: calc(100% + var(--border-width) - var(--resize-outline-half-width));
    width: var(--resize-outline-width);
    height: var(--resize-outline-width);
    cursor: nwse-resize;
}

.title-bar {
    display: none;
    align-items: center;
    overflow: hidden;
    height: 18px;
    padding-bottom: 4px;
}

.qt-window.has-title-bar .title-bar {
    display: flex;
}

.title-bar .window-name {
    font-family: 'Lucida Grande';
    white-space: nowrap;
    user-select: none;
    overflow: hidden;
}

.title-bar .spacer {
    flex-grow: 1
}

.qt-window.inactive .title-bar {
    opacity: 0.35;
}

.qt-window-canvas-container {
    display: flex;
}

.qt-window-a11y-container {
    position: absolute;
    z-index: -1;
}

.title-bar div {
    pointer-events: none;
}

.title-bar .image-button {
    width: 18px;
    height: 18px;
    display: flex;
    justify-content: center;
    user-select: none;
    align-items: center;
}

.title-bar .image-button img {
    width: 10px;
    height: 10px;
    user-select: none;
    pointer-events: none;
    -webkit-user-drag: none;
    background-size: 10px 10px;
}

.title-bar .image-button img[qt-builtin-image-type=x] {
    background-image: url("data:image/svg+xml;base64,$close_icon");
}

.title-bar .image-button img[qt-builtin-image-type=qt-logo] {
    background-image: url("qtlogo.svg");
}

.title-bar .image-button img[qt-builtin-image-type=restore] {
    background-image: url("data:image/svg+xml;base64,$restore_icon");
}

.title-bar .image-button img[qt-builtin-image-type=maximize] {
    background-image: url("data:image/svg+xml;base64,$maximize_icon");
}
.title-bar .action-button {
    pointer-events: all;
}

.qt-window.blocked div {
    pointer-events: none;
}

.title-bar .action-button img {
    transition: filter 0.08s ease-out;
}

.title-bar .action-button:hover img {
    filter: invert(0.45);
}

.title-bar .action-button:active img {
    filter: invert(0.6);
}

)css";

void replace(std::string &str, const std::string &from, const std::string_view &to)
{
    str.replace(str.find(from), from.length(), to);
}
} // namespace

emscripten::val QWasmCSSStyle::createStyleElement(emscripten::val parent)
{
    auto document = parent["ownerDocument"];
    auto screenStyle = document.call<emscripten::val>("createElement", emscripten::val("style"));
    auto text = std::string(Style);

    using IconType = Base64IconStore::IconType;
    replace(text, "$close_icon", Base64IconStore::get()->getIcon(IconType::X));
    replace(text, "$restore_icon", Base64IconStore::get()->getIcon(IconType::Restore));
    replace(text, "$maximize_icon", Base64IconStore::get()->getIcon(IconType::Maximize));

    screenStyle.set("textContent", text);
    return screenStyle;
}

QT_END_NAMESPACE
