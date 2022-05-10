// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [1]
enum CustomEventPriority
{
    // An important event
    ImportantEventPriority = Qt::HighEventPriority,

    // A more important event
    MoreImportantEventPriority = ImportantEventPriority + 1,

    // A critical event
    CriticalEventPriority = 100 * MoreImportantEventPriority,

    // Not that important
    StatusEventPriority = Qt::LowEventPriority,

    // These are less important than Status events
    IdleProcessingDoneEventPriority = StatusEventPriority - 1
};
//! [1]
