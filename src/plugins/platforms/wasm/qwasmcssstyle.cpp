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

.qt-screen div {
    touch-action: none;
}

.qt-decorated-window {
    position: absolute;
    background-color: lightgray;
}

.qt-window {
    overflow: hidden;
    position: relative;
}

.qt-decorated-window.transparent-for-input {
    pointer-events: none;
}

.qt-decorated-window.has-shadow {
    box-shadow: rgb(0 0 0 / 20%) 0px 10px 16px 0px, rgb(0 0 0 / 19%) 0px 6px 20px 0px;
}

.qt-decorated-window.has-border {
    border: var(--border-width) solid lightgray;
    caret-color: transparent;
}

.qt-decorated-window.frameless {
    background-color: transparent;
}

.resize-outline {
    position: absolute;
    display: none;
}

.qt-decorated-window.no-resize > .resize-outline { display: none; }

.qt-decorated-window.has-border:not(.maximized):not(.no-resize) > .resize-outline {
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

.qt-decorated-window.has-border > .title-bar {
    display: flex;
}

.title-bar .window-name {
    display: none;
    font-family: 'Lucida Grande';
    white-space: nowrap;
    user-select: none;
    overflow: hidden;
}


.qt-decorated-window.has-title .title-bar .window-name {
    display: block;
}

.title-bar .spacer {
    flex-grow: 1
}

.qt-decorated-window.inactive .title-bar {
    opacity: 0.35;
}

.qt-window {
    display: flex;
}

.title-bar div {
    pointer-events: none;
}

.qt-window-a11y-container {
    position: absolute;
    z-index: -1;
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

.title-bar .action-button {
    pointer-events: all;
}

.qt-decorated-window.blocked div {
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

/* This will clip the content within 50% frame in 1x1 pixel area, preventing it
    from being rendered on the page, but it should still be read by modern
    screen readers */
.hidden-visually-read-by-screen-reader {
    visibility: visible;
    clip: rect(1px, 1px, 1px, 1px);
    clip-path: inset(50%);
    height: 100%;
    width: 100%;
    margin: -1px;
    overflow: hidden;
    padding: 0;
    position: absolute;
}

)css";

} // namespace

emscripten::val QWasmCSSStyle::createStyleElement(emscripten::val parent)
{
    auto document = parent["ownerDocument"];
    auto screenStyle = document.call<emscripten::val>("createElement", emscripten::val("style"));

    screenStyle.set("textContent", std::string(Style));
    return screenStyle;
}

QT_END_NAMESPACE
