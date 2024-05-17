// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

export function parseQuery() {
    const trimmed = window.location.search.substring(1);
    return new Map(
        trimmed.length === 0 ?
            [] :
            trimmed.split('&').map(paramNameAndValue => {
                const [name, value] = paramNameAndValue.split('=');
                return [decodeURIComponent(name), value ? decodeURIComponent(value) : ''];
            }));
}

export class EventSource {
    #listeners = [];

    constructor(receivePrivateInterface) {
        receivePrivateInterface({
            fireEvent: (arg0, arg1) => this.#fireEvent(arg0, arg1)
        });
    }

    addEventListener(listener) {
        this.#listeners.push(listener);
    }

    #fireEvent(arg0, arg1) {
        this.#listeners.forEach(listener => listener(arg0, arg1));
    }
}
