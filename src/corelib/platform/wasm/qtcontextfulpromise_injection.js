`Copyright (C) 2022 The Qt Company Ltd.
Copyright (C) 2016 Intel Corporation.
SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0`;

if (window.qtContextfulPromiseSupport) {
    ++window.qtContextfulPromiseSupport.refs;
} else {
    window.qtContextfulPromiseSupport = {
        refs: 1,
        removeRef: () => {
            --window.qtContextfulPromiseSupport.refs, 0 === window.qtContextfulPromiseSupport.refs && delete window.qtContextfulPromiseSupport;
        },
        makePromise: (a, b, c) => new window.qtContextfulPromiseSupport.ContextfulPromise(a, b, c),
    };

    window.qtContextfulPromiseSupport.ContextfulPromise = class {
        constructor(a, b, c) {
            (this.wrappedPromise = a), (this.context = b), (this.callbackThunk = c);
        }
        then() {
            return (this.wrappedPromise = this.wrappedPromise.then((a) => { this.callbackThunk("then", this.context, a); })), this;
        }
        catch() {
            return (this.wrappedPromise = this.wrappedPromise.catch((a) => { this.callbackThunk("catch", this.context, a); })), this;
        }
        finally() {
            return (this.wrappedPromise = this.wrappedPromise.finally(() => this.callbackThunk("finally", this.context, undefined))), this;
        }
    };
}

document.querySelector("[qtinjection=contextfulpromise]")?.remove();
