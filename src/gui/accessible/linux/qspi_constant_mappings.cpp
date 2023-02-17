// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qspi_constant_mappings_p.h"

#include <qobject.h>
#include <qdebug.h>

// FIXME the assignment of roles is quite arbitrary, at some point go through this list and sort and check that it makes sense
//  "calendar" "check menu item"  "color chooser" "column header"    "dateeditor"  "desktop icon"  "desktop frame"
//  "directory pane"  "drawing area"  "file chooser" "fontchooser"  "frame"  "glass pane"  "html container"  "icon"
//  "internal frame"  "option pane"  "password text" "radio menu item"  "root pane"  "row header"    "scroll pane"
//  "tear off menu item"  "terminal" "toggle button" "tree table"  "unknown"  "viewport" "header"  "footer"  "paragraph"
//  "ruler"    "autocomplete"  "edit bar" "embedded component"  "entry"    "caption"
//  "heading"  "page"  "section"  "redundant object"  "form"  "input method window"  "menu"

#if QT_CONFIG(accessibility)
QT_BEGIN_NAMESPACE

quint64 spiStatesFromQState(QAccessible::State state)
{
    quint64 spiState = 0;

    if (state.active)
        setSpiStateBit(&spiState, ATSPI_STATE_ACTIVE);
    if (state.editable)
        setSpiStateBit(&spiState, ATSPI_STATE_EDITABLE);
    if (!state.disabled) {
        setSpiStateBit(&spiState, ATSPI_STATE_ENABLED);
        setSpiStateBit(&spiState, ATSPI_STATE_SENSITIVE);
    }
    if (state.selected)
        setSpiStateBit(&spiState, ATSPI_STATE_SELECTED);
    if (state.focused)
        setSpiStateBit(&spiState, ATSPI_STATE_FOCUSED);
    if (state.pressed)
        setSpiStateBit(&spiState, ATSPI_STATE_PRESSED);
    if (state.checked)
        setSpiStateBit(&spiState, ATSPI_STATE_CHECKED);
    if (state.checkStateMixed)
        setSpiStateBit(&spiState, ATSPI_STATE_INDETERMINATE);
    if (state.readOnly)
        setSpiStateBit(&spiState, ATSPI_STATE_READ_ONLY);
    //        if (state.HotTracked)
    if (state.defaultButton)
        setSpiStateBit(&spiState, ATSPI_STATE_IS_DEFAULT);
    if (state.expandable)
        setSpiStateBit(&spiState, ATSPI_STATE_EXPANDABLE);
    if (state.expanded)
        setSpiStateBit(&spiState, ATSPI_STATE_EXPANDED);
    if (state.collapsed)
        setSpiStateBit(&spiState, ATSPI_STATE_COLLAPSED);
    if (state.busy)
        setSpiStateBit(&spiState, ATSPI_STATE_BUSY);
    if (state.marqueed || state.animated)
        setSpiStateBit(&spiState, ATSPI_STATE_ANIMATED);
    if (!state.invisible && !state.offscreen) {
        setSpiStateBit(&spiState, ATSPI_STATE_SHOWING);
        setSpiStateBit(&spiState, ATSPI_STATE_VISIBLE);
    }
    if (state.sizeable)
        setSpiStateBit(&spiState, ATSPI_STATE_RESIZABLE);
    //        if (state.Movable)
    //        if (state.SelfVoicing)
    if (state.focusable)
        setSpiStateBit(&spiState, ATSPI_STATE_FOCUSABLE);
    if (state.selectable)
        setSpiStateBit(&spiState, ATSPI_STATE_SELECTABLE);
    //        if (state.Linked)
    if (state.traversed)
        setSpiStateBit(&spiState, ATSPI_STATE_VISITED);
    if (state.multiSelectable)
        setSpiStateBit(&spiState, ATSPI_STATE_MULTISELECTABLE);
    if (state.extSelectable)
        setSpiStateBit(&spiState, ATSPI_STATE_SELECTABLE);
    //        if (state.Protected)
    //        if (state.HasPopup)
    if (state.modal)
        setSpiStateBit(&spiState, ATSPI_STATE_MODAL);
    if (state.multiLine)
        setSpiStateBit(&spiState, ATSPI_STATE_MULTI_LINE);

    return spiState;
}

QSpiUIntList spiStateSetFromSpiStates(quint64 states)
{
    uint low = states & 0xFFFFFFFF;
    uint high = (states >> 32) & 0xFFFFFFFF;

    QSpiUIntList stateList;
    stateList.append(low);
    stateList.append(high);
    return stateList;
}

AtspiRelationType qAccessibleRelationToAtSpiRelation(QAccessible::Relation relation)
{
    // direction of the relation is "inversed" in Qt and AT-SPI
    switch (relation) {
    case QAccessible::Label:
        return ATSPI_RELATION_LABELLED_BY;
    case QAccessible::Labelled:
        return ATSPI_RELATION_LABEL_FOR;
    case QAccessible::Controller:
        return ATSPI_RELATION_CONTROLLED_BY;
    case QAccessible::Controlled:
        return ATSPI_RELATION_CONTROLLER_FOR;
    case QAccessible::DescriptionFor:
        return ATSPI_RELATION_DESCRIBED_BY;
    case QAccessible::Described:
        return ATSPI_RELATION_DESCRIPTION_FOR;
    case QAccessible::FlowsFrom:
        return ATSPI_RELATION_FLOWS_TO;
    case QAccessible::FlowsTo:
        return ATSPI_RELATION_FLOWS_FROM;
    default:
        qWarning() << "Cannot return AT-SPI relation for:" << relation;
    }
    return ATSPI_RELATION_NULL;
}

QT_END_NAMESPACE
#endif // QT_CONFIG(accessibility)
