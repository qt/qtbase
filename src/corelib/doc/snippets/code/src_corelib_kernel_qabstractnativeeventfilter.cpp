// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
class MyXcbEventFilter : public QAbstractNativeEventFilter
{
public:
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *) override
    {
        if (eventType == "xcb_generic_event_t") {
            xcb_generic_event_t* ev = static_cast<xcb_generic_event_t *>(message);
            // ...
        }
        return false;
    }
};
//! [0]
