// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmcssstyle.h"

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
    outline: none;
}

.qt-window {
    box-shadow: rgb(0 0 0 / 20%) 0px 10px 16px 0px, rgb(0 0 0 / 19%) 0px 6px 20px 0px;
    pointer-events: none;
    position: absolute;
    background-color: lightgray;
}

.qt-window.has-title-bar {
    border: var(--border-width) solid lightgray;
    caret-color: transparent;
}

.resize-outline {
    position: absolute;
    pointer-events: all;
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

.title-bar .image-button {
    width: 18px;
    height: 18px;
    display: flex;
    justify-content: center;
    user-select: none;
    align-items: center;
}

.title-bar .image-button span {
    width: 10px;
    height: 10px;
    user-select: none;
    pointer-events: none;
    -webkit-user-drag: none;
    background-size: 10px 10px;
}

.title-bar .image-button span[qt-builtin-image-type=x] {
    background-image: url("data:image/svg+xml;base64,$close_icon");
}

.title-bar .image-button span[qt-builtin-image-type=qt-logo] {
    background-image: url("qtlogo.svg");
}

.title-bar .image-button span[qt-builtin-image-type=restore] {
    background-image: url("data:image/svg+xml;base64,$restore_icon");
}

.title-bar .image-button span[qt-builtin-image-type=maximize] {
    background-image: url("data:image/svg+xml;base64,$maximize_icon");
}
.title-bar .action-button {
    pointer-events: all;
    align-self: end;
}

.qt-window.blocked .title-bar .action-button {
    pointer-events: none;
}

.title-bar .action-button span {
    transition: filter 0.08s ease-out;
}

.title-bar .action-button:hover span {
    filter: invert(0.45);
}

.title-bar .action-button:active span {
    filter: invert(0.6);
}

)css";

class Base64IconStore
{
public:
    enum class IconType {
        Maximize,
        First = Maximize,
        QtLogo,
        Restore,
        X,
        Size,
    };

    Base64IconStore()
    {
        QString iconSources[static_cast<size_t>(IconType::Size)] = {
            QStringLiteral(":/wasm-window/maximize.svg"),
            QStringLiteral(":/wasm-window/qtlogo.svg"), QStringLiteral(":/wasm-window/restore.svg"),
            QStringLiteral(":/wasm-window/x.svg")
        };

        for (size_t iconType = static_cast<size_t>(IconType::First);
             iconType < static_cast<size_t>(IconType::Size); ++iconType) {
            QFile svgFile(iconSources[static_cast<size_t>(iconType)]);
            if (!svgFile.open(QIODevice::ReadOnly))
                Q_ASSERT(false); // A resource should always be opened.
            m_storage[static_cast<size_t>(iconType)] = svgFile.readAll().toBase64();
        }
    }
    ~Base64IconStore() = default;

    std::string_view getIcon(IconType type) const { return m_storage[static_cast<size_t>(type)]; }

private:
    std::string m_storage[static_cast<size_t>(IconType::Size)];
};

void replace(std::string &str, const std::string &from, const std::string_view &to)
{
    str.replace(str.find(from), from.length(), to);
}
} // namespace

emscripten::val QWasmCSSStyle::createStyleElement(emscripten::val parent)
{
    Base64IconStore store;
    auto document = parent["ownerDocument"];
    auto screenStyle = document.call<emscripten::val>("createElement", emscripten::val("style"));
    auto text = std::string(Style);
    replace(text, "$close_icon", store.getIcon(Base64IconStore::IconType::X));
    replace(text, "$restore_icon", store.getIcon(Base64IconStore::IconType::Restore));
    replace(text, "$maximize_icon", store.getIcon(Base64IconStore::IconType::Maximize));

    screenStyle.set("textContent", text);
    return screenStyle;
}

QT_END_NAMESPACE
