// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <AppKit/AppKit.h>

#include "qcocoaaccessibilityelement.h"
#include "qcocoaaccessibility.h"
#include "qcocoahelpers.h"
#include "qcocoawindow.h"
#include "qcocoascreen.h"

#include <QtGui/private/qaccessiblecache_p.h>
#include <QtGui/private/qaccessiblebridgeutils_p.h>
#include <QtGui/qaccessible.h>

QT_USE_NAMESPACE

#if QT_CONFIG(accessibility)

/**
 * Converts between absolute character offsets and line numbers of a
 * QAccessibleTextInterface. Works in exactly one of two modes:
 *
 *  - Pass *line == -1 in order to get a line containing character at the given
 *    *offset
 *  - Pass *offset == -1 in order to get the offset of first character of the
 *    given *line
 *
 * You can optionally also pass non-NULL `start` and `end`, which will in both
 * modes be filled with the offset of the first and last characters of the
 * relevant line.
 */
static void convertLineOffset(QAccessibleTextInterface *text, int *line, int *offset, NSUInteger *start = 0, NSUInteger *end = 0)
{
    Q_ASSERT(*line == -1 || *offset == -1);
    Q_ASSERT(*line != -1 || *offset != -1);
    Q_ASSERT(*offset <= text->characterCount());

    int curLine = -1;
    int curStart = 0, curEnd = 0;

    do {
        curStart = curEnd;
        text->textAtOffset(curStart, QAccessible::LineBoundary, &curStart, &curEnd);
        // If the text is empty then we just return
        if (curStart == -1 || curEnd == -1) {
            if (start)
                *start = 0;
            if (end)
                *end = 0;
            return;
        }
        ++curLine;
        {
            // check for a case where a single word longer than the text edit's width and gets wrapped
            // in the middle of the word; in this case curEnd will be an offset belonging to the next line
            // and therefore nextEnd will not be equal to curEnd
            int nextStart;
            int nextEnd;
            text->textAtOffset(curEnd, QAccessible::LineBoundary, &nextStart, &nextEnd);
            if (nextEnd == curEnd)
                ++curEnd;
        }
    } while ((*line == -1 || curLine < *line) && (*offset == -1 || (curEnd <= *offset)) && curEnd <= text->characterCount());

    curEnd = qMin(curEnd, text->characterCount());

    if (*line == -1)
        *line = curLine;
    if (*offset == -1)
        *offset = curStart;

    Q_ASSERT(curStart >= 0);
    Q_ASSERT(curEnd >= 0);
    if (start)
        *start = curStart;
    if (end)
        *end = curEnd;
}

@implementation QMacAccessibilityElement {
    QAccessible::Id axid;

    // used by NSAccessibilityTable
    NSMutableArray<QMacAccessibilityElement *> *rows;       // corresponds to accessibilityRows
    NSMutableArray<QMacAccessibilityElement *> *columns;    // corresponds to accessibilityColumns

    // If synthesizedRole is set, this means that this objects does not have a corresponding
    // QAccessibleInterface, but it is synthesized by the cocoa plugin in order to meet the
    // NSAccessibility requirements.
    // The ownership is controlled by the parent object identified with the axid member variable.
    // (Therefore, if this member is set, this objects axid member is the same as the parents axid
    // member)
    NSString *synthesizedRole;
}

- (instancetype)initWithId:(QAccessible::Id)anId
{
    return [self initWithId:anId role:nil];
}

- (instancetype)initWithId:(QAccessible::Id)anId role:(NSAccessibilityRole)role
{
    Q_ASSERT((int)anId < 0);
    self = [super init];
    if (self) {
        axid = anId;
        rows = nil;
        columns = nil;
        synthesizedRole = role;
        QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
        if (iface && iface->tableInterface() && !synthesizedRole)
            [self updateTableModel];
    }

    return self;
}

+ (instancetype)elementWithId:(QAccessible::Id)anId
{
    Q_ASSERT(anId);
    if (!anId)
        return nil;

    QAccessibleCache *cache = QAccessibleCache::instance();

    QMacAccessibilityElement *element = cache->elementForId(anId);
    if (!element) {
        QAccessibleInterface *iface = QAccessible::accessibleInterface(anId);
        Q_ASSERT(iface);
        if (!iface || !iface->isValid())
            return nil;
        element = [[self alloc] initWithId:anId];
        cache->insertElement(anId, element);
    }
    return element;
}

- (void)invalidate {
    axid = 0;
    rows = nil;
    columns = nil;
    synthesizedRole = nil;

    NSAccessibilityPostNotification(self, NSAccessibilityUIElementDestroyedNotification);
    [self release];
}

- (void)dealloc {
    if (rows)
        [rows release]; // will also release all entries first
    if (columns)
        [columns release]; // will also release all entries first
    [super dealloc];
}

- (BOOL)isEqual:(id)object {
    if ([object isKindOfClass:[QMacAccessibilityElement class]]) {
        QMacAccessibilityElement *other = object;
        return other->axid == axid && other->synthesizedRole == synthesizedRole;
    } else {
        return NO;
    }
}

- (NSUInteger)hash {
    return axid;
}

- (BOOL)isManagedByParent {
    return synthesizedRole != nil;
}

- (NSMutableArray *)populateTableArray:(NSMutableArray *)array role:(NSAccessibilityRole)role count:(int)count
{
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (iface && iface->isValid()) {
        if (!array) {
            array = [NSMutableArray<QMacAccessibilityElement *> arrayWithCapacity:count];
            [array retain];
        } else {
            [array removeAllObjects];
        }
        Q_ASSERT(array);
        for (int n = 0; n < count; ++n) {
            // columns will have same axid as table (but not inserted in cache)
            QMacAccessibilityElement *element =
                    [[QMacAccessibilityElement alloc] initWithId:axid role:role];
            if (element) {
                [array addObject:element];
                [element release];
            } else {
                qWarning("QCocoaAccessibility: invalid child");
            }
        }
        return array;
    }
    return nil;
}


- (void)updateTableModel
{
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (iface && iface->isValid()) {
        if (QAccessibleTableInterface *table = iface->tableInterface()) {
            Q_ASSERT(!self.isManagedByParent);
            rows = [self populateTableArray:rows role:NSAccessibilityRowRole count:table->rowCount()];
            columns = [self populateTableArray:columns role:NSAccessibilityColumnRole count:table->columnCount()];
        }
    }
}

//
// accessibility protocol
//

- (BOOL)isAccessibilityFocused
{
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid()) {
        return false;
    }
    // Just check if the app thinks we're focused.
    id focusedElement = NSApp.accessibilityApplicationFocusedUIElement;
    return [focusedElement isEqual:self];
}

// attributes

+ (id) lineNumberForIndex: (int)index forText:(const QString &)text
{
    auto textBefore = QStringView(text).left(index);
    qsizetype newlines = textBefore.count(u'\n');
    return @(newlines);
}

- (BOOL) accessibilityNotifiesWhenDestroyed {
    return YES;
}

- (NSString *) accessibilityRole {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid())
        return NSAccessibilityUnknownRole;
    if (synthesizedRole)
        return synthesizedRole;
    return QCocoaAccessible::macRole(iface);
}

- (NSString *) accessibilitySubRole {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid())
        return NSAccessibilityUnknownRole;
    return QCocoaAccessible::macSubrole(iface);
}

- (NSString *) accessibilityRoleDescription {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid())
        return NSAccessibilityUnknownRole;
    return NSAccessibilityRoleDescription(self.accessibilityRole, self.accessibilitySubRole);
}

- (NSArray *) accessibilityChildren {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid())
        return nil;
    if (QAccessibleTableInterface *table = iface->tableInterface()) {
        // either a table or table rows/columns
        if (!synthesizedRole) {
            // This is the table element, parent of all rows and columns
            /*
             * Typical 2x2 table hierarchy as can be observed in a table found under
             *   Apple -> System Settings -> General -> Login Items    (macOS 13)
             *
             *  (AXTable)
             *  | Columns: NSArray* (2 items)
             *  | Rows: NSArray* (2 items)
             *  | Visible Columns: NSArray* (2 items)
             *  | Visible Rows: NSArray* (2 items)
             *  | Children: NSArray* (5 items)
        +----<--| Header: (AXGroup)
        |    *  +-- (AXRow)
        |    *  |   +-- (AXText)
        |    *  |   +-- (AXTextField)
        |    *  +-- (AXRow)
        |    *  |   +-- (AXText)
        |    *  |   +-- (AXTextField)
        |    *  +-- (AXColumn)
        |    *  |     Header: "Item" (sort button)
        |    *  |     Index: 0
        |    *  |     Rows: NSArray* (2 items)
        |    *  |     Visible Rows: NSArray* (2 items)
        |    *  +-- (AXColumn)
        |    *  |     Header: "Kind" (sort button)
        |    *  |     Index: 1
        |    *  |     Rows: NSArray* (2 items)
        |    *  |     Visible Rows: NSArray* (2 items)
        +---->  +-- (AXGroup)
             *      +-- (AXButton/AXSortButton) Item [NSAccessibilityTableHeaderCellProxy]
             *      +-- (AXButton/AXSortButton) Kind [NSAccessibilityTableHeaderCellProxy]
             */
            NSArray *rs = [self accessibilityRows];
            NSArray *cs = [self accessibilityColumns];
            const int rCount = int([rs count]);
            const int cCount = int([cs count]);
            int childCount = rCount + cCount;
            NSMutableArray<QMacAccessibilityElement *> *tableChildren =
                    [NSMutableArray<QMacAccessibilityElement *> arrayWithCapacity:childCount];
            for (int i = 0; i < rCount; ++i) {
                [tableChildren addObject:[rs objectAtIndex:i]];
            }
            for (int i = 0; i < cCount; ++i) {
                [tableChildren addObject:[cs objectAtIndex:i]];
            }
            return NSAccessibilityUnignoredChildren(tableChildren);
        } else if (synthesizedRole == NSAccessibilityColumnRole) {
            return nil;
        } else if (synthesizedRole == NSAccessibilityRowRole) {
            // axid matches the parent table axid so that we can easily find the parent table
            // children of row are cell/any items
            QMacAccessibilityElement *tableElement = [QMacAccessibilityElement elementWithId:axid];
            Q_ASSERT(tableElement->rows);
            NSUInteger rowIndex = [tableElement->rows indexOfObjectIdenticalTo:self];
            Q_ASSERT(rowIndex != NSNotFound);
            int numColumns = table->columnCount();
            NSMutableArray<QMacAccessibilityElement *> *cells =
                    [NSMutableArray<QMacAccessibilityElement *> arrayWithCapacity:numColumns];
            for (int i = 0; i < numColumns; ++i) {
                QAccessibleInterface *cell = table->cellAt(rowIndex, i);
                if (cell && cell->isValid()) {
                    QAccessible::Id cellId = QAccessible::uniqueId(cell);
                    QMacAccessibilityElement *element =
                            [QMacAccessibilityElement elementWithId:cellId];
                    if (element) {
                        [cells addObject:element];
                    }
                }
            }
            return NSAccessibilityUnignoredChildren(cells);
        }
    }
    return QCocoaAccessible::unignoredChildren(iface);
}

- (id) accessibilityWindow {
    // We're in the same window as our parent.
    return [self.accessibilityParent accessibilityWindow];
}

- (id) accessibilityTopLevelUIElementAttribute {
    // We're in the same top level element as our parent.
    return [self.accessibilityParent accessibilityTopLevelUIElementAttribute];
}

- (NSString *) accessibilityTitle {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid())
        return nil;
    if (iface->role() == QAccessible::StaticText)
        return nil;
    if (self.isManagedByParent)
        return nil;
    return iface->text(QAccessible::Name).toNSString();
}

- (BOOL) isAccessibilityEnabled {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid())
        return false;
    return !iface->state().disabled;
}

- (id)accessibilityParent {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid())
        return nil;

    if (self.isManagedByParent) {
        // axid is the same for the parent element
        return NSAccessibilityUnignoredAncestor([QMacAccessibilityElement elementWithId:axid]);
    }

    // macOS expects that the hierarchy is:
    // App -> Window -> Children
    // We don't actually have the window reflected properly in QAccessibility.
    // Check if the parent is the application and then instead return the native window.

    if (QAccessibleInterface *parent = iface->parent()) {
        if (parent->tableInterface()) {
            if (QAccessibleTableCellInterface *cell = iface->tableCellInterface()) {
                // parent of cell should be row
                QAccessible::Id parentId = QAccessible::uniqueId(parent);
                QMacAccessibilityElement *tableElement =
                        [QMacAccessibilityElement elementWithId:parentId];

                const int rowIndex = cell->rowIndex();
                if (tableElement->rows && int([tableElement->rows count]) > rowIndex) {
                    QMacAccessibilityElement *rowElement = tableElement->rows[rowIndex];
                    return NSAccessibilityUnignoredAncestor(rowElement);
                }
            }
        }
        if (parent->role() != QAccessible::Application) {
            QAccessible::Id parentId = QAccessible::uniqueId(parent);
            return NSAccessibilityUnignoredAncestor([QMacAccessibilityElement elementWithId: parentId]);
        }
    }

    if (QWindow *window = iface->window()) {
        QPlatformWindow *platformWindow = window->handle();
        if (platformWindow) {
            QCocoaWindow *win = static_cast<QCocoaWindow*>(platformWindow);
            return NSAccessibilityUnignoredAncestor(qnsview_cast(win->view()));
        }
    }
    return nil;
}

- (NSRect)accessibilityFrame {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid())
        return NSZeroRect;

    QRect rect;
    if (self.isManagedByParent) {
        if (QAccessibleTableInterface *table = iface->tableInterface()) {
            // Construct the geometry of the Row/Column by looking at the individual table cells
            // ### Assumes that cells logical coordinates have spatial ordering (e.g finds the
            // rows width by taking the union between the leftmost item and the rightmost item in
            // a row).
            // Otherwise, we have to iterate over *all* cells in a row/columns to
            // find out the Row/Column geometry
            const bool isRow = synthesizedRole == NSAccessibilityRowRole;
            QPoint cellPos;
            int &row = isRow ? cellPos.ry() : cellPos.rx();
            int &col = isRow ? cellPos.rx() : cellPos.ry();

            QMacAccessibilityElement *tableElement =
                    [QMacAccessibilityElement elementWithId:axid];
            NSArray *tracks = isRow ? tableElement->rows : tableElement->columns;
            NSUInteger trackIndex = [tracks indexOfObjectIdenticalTo:self];
            if (trackIndex != NSNotFound) {
                row = int(trackIndex);
                if (QAccessibleInterface *firstCell = table->cellAt(cellPos.y(), cellPos.x())) {
                    rect = firstCell->rect();
                    col = isRow ? table->columnCount() : table->rowCount();
                    if (col > 1) {
                        --col;
                        if (QAccessibleInterface *lastCell =
                                    table->cellAt(cellPos.y(), cellPos.x()))
                            rect = rect.united(lastCell->rect());
                    }
                }
            }
        }
    } else {
        rect = iface->rect();
    }

    return QCocoaScreen::mapToNative(rect);
}

- (NSString*)accessibilityLabel {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid()) {
        qWarning() << "Called accessibilityLabel on invalid object: " << axid;
        return nil;
    }
    return iface->text(QAccessible::Description).toNSString();
}

- (void)setAccessibilityLabel:(NSString*)label{
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid())
        return;
    iface->setText(QAccessible::Description, QString::fromNSString(label));
}

- (id) accessibilityMinValue:(QAccessibleInterface*)iface {
    if (QAccessibleValueInterface *val = iface->valueInterface())
        return @(val->minimumValue().toDouble());
    return nil;
}

- (id) accessibilityMaxValue:(QAccessibleInterface*)iface {
    if (QAccessibleValueInterface *val = iface->valueInterface())
        return @(val->maximumValue().toDouble());
    return nil;
}

- (id) accessibilityValue {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid())
        return nil;

    // VoiceOver asks for the value attribute for all elements. Return nil
    // if we don't want the element to have a value attribute.
    if (!QCocoaAccessible::hasValueAttribute(iface))
        return nil;

    return QCocoaAccessible::getValueAttribute(iface);
}

- (NSInteger) accessibilityNumberOfCharacters {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid())
        return 0;
    if (QAccessibleTextInterface *text = iface->textInterface())
        return text->characterCount();
    return 0;
}

- (NSString *) accessibilitySelectedText {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid())
        return nil;
    if (QAccessibleTextInterface *text = iface->textInterface()) {
        int start = 0;
        int end = 0;
        text->selection(0, &start, &end);
        return text->text(start, end).toNSString();
    }
    return nil;
}

- (NSRange) accessibilitySelectedTextRange {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid())
        return NSRange();
    if (QAccessibleTextInterface *text = iface->textInterface()) {
        int start = 0;
        int end = 0;
        if (text->selectionCount() > 0) {
            text->selection(0, &start, &end);
        } else {
            start = text->cursorPosition();
            end = start;
        }
        return NSMakeRange(quint32(start), quint32(end - start));
    }
    return NSMakeRange(0, 0);
}

- (NSInteger)accessibilityLineForIndex:(NSInteger)index {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid())
        return 0;
    if (QAccessibleTextInterface *text = iface->textInterface()) {
        QString textToPos = text->text(0, index);
        return textToPos.count('\n');
    }
    return 0;
}

- (NSRange)accessibilityVisibleCharacterRange {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid())
        return NSRange();
    // FIXME This is not correct and may impact performance for big texts
    if (QAccessibleTextInterface *text = iface->textInterface())
        return NSMakeRange(0, static_cast<uint>(text->characterCount()));
    return NSMakeRange(0, static_cast<uint>(iface->text(QAccessible::Name).length()));
}

- (NSInteger) accessibilityInsertionPointLineNumber {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid())
        return 0;
    if (QAccessibleTextInterface *text = iface->textInterface()) {
        int position = text->cursorPosition();
        return [self accessibilityLineForIndex:position];
    }
    return 0;
}

- (NSArray *)accessibilityParameterizedAttributeNames {

    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid()) {
        qWarning() << "Called attribute on invalid object: " << axid;
        return nil;
    }

    if (iface->textInterface()) {
        return @[
            NSAccessibilityStringForRangeParameterizedAttribute,
            NSAccessibilityLineForIndexParameterizedAttribute,
            NSAccessibilityRangeForLineParameterizedAttribute,
            NSAccessibilityRangeForPositionParameterizedAttribute,
//          NSAccessibilityRangeForIndexParameterizedAttribute,
            NSAccessibilityBoundsForRangeParameterizedAttribute,
//          NSAccessibilityRTFForRangeParameterizedAttribute,
            NSAccessibilityStyleRangeForIndexParameterizedAttribute,
            NSAccessibilityAttributedStringForRangeParameterizedAttribute
        ];
    }

    return nil;
}

- (id)accessibilityAttributeValue:(NSString *)attribute forParameter:(id)parameter {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid()) {
        qWarning() << "Called attribute on invalid object: " << axid;
        return nil;
    }

    if (!iface->textInterface())
        return nil;

    if ([attribute isEqualToString: NSAccessibilityStringForRangeParameterizedAttribute]) {
        NSRange range = [parameter rangeValue];
        QString text = iface->textInterface()->text(range.location, range.location + range.length);
        return text.toNSString();
    }
    if ([attribute isEqualToString: NSAccessibilityLineForIndexParameterizedAttribute]) {
        int index = [parameter intValue];
        if (index < 0 || index > iface->textInterface()->characterCount())
            return nil;
        int line = 0; // true for all single line edits
        if (iface->state().multiLine) {
            line = -1;
            convertLineOffset(iface->textInterface(), &line, &index);
        }
        return @(line);
    }
    if ([attribute isEqualToString: NSAccessibilityRangeForLineParameterizedAttribute]) {
        int line = [parameter intValue];
        if (line < 0)
            return nil;
        int lineOffset = -1;
        NSUInteger startOffset = 0;
        NSUInteger endOffset = 0;
        convertLineOffset(iface->textInterface(), &line, &lineOffset, &startOffset, &endOffset);
        return [NSValue valueWithRange:NSMakeRange(startOffset, endOffset - startOffset)];
    }
    if ([attribute isEqualToString: NSAccessibilityBoundsForRangeParameterizedAttribute]) {
        NSRange range = [parameter rangeValue];
        QRect firstRect = iface->textInterface()->characterRect(range.location);
        QRectF rect;
        if (range.length > 0) {
            NSUInteger position = range.location + range.length - 1;
            if (position > range.location && iface->textInterface()->text(position, position + 1) == QStringLiteral("\n"))
                --position;
            QRect lastRect = iface->textInterface()->characterRect(position);
            rect = firstRect.united(lastRect);
        } else {
            rect = firstRect;
            rect.setWidth(1);
        }
        return [NSValue valueWithRect:QCocoaScreen::mapToNative(rect)];
    }
    if ([attribute isEqualToString: NSAccessibilityAttributedStringForRangeParameterizedAttribute]) {
        NSRange range = [parameter rangeValue];
        QString text = iface->textInterface()->text(range.location, range.location + range.length);
        return [[NSAttributedString alloc] initWithString:text.toNSString()];
    } else if ([attribute isEqualToString: NSAccessibilityRangeForPositionParameterizedAttribute]) {
        QPoint point = QCocoaScreen::mapFromNative([parameter pointValue]).toPoint();
        int offset = iface->textInterface()->offsetAtPoint(point);
        return [NSValue valueWithRange:NSMakeRange(static_cast<NSUInteger>(offset), 1)];
    } else if ([attribute isEqualToString: NSAccessibilityStyleRangeForIndexParameterizedAttribute]) {
        int start = 0;
        int end = 0;
        iface->textInterface()->attributes([parameter intValue], &start, &end);
        return [NSValue valueWithRange:NSMakeRange(static_cast<NSUInteger>(start), static_cast<NSUInteger>(end - start))];
    }
    return nil;
}

- (BOOL)accessibilityIsAttributeSettable:(NSString *)attribute {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid())
        return NO;

    if ([attribute isEqualToString:NSAccessibilityFocusedAttribute]) {
        return iface->state().focusable ? YES : NO;
    } else if ([attribute isEqualToString:NSAccessibilityValueAttribute]) {
        if (iface->textInterface() && iface->state().editable)
            return YES;
        if (iface->valueInterface())
            return YES;
        return NO;
    } else if ([attribute isEqualToString:NSAccessibilitySelectedTextRangeAttribute]) {
        return iface->textInterface() ? YES : NO;
    }
    return NO;
}

- (void)accessibilitySetValue:(id)value forAttribute:(NSString *)attribute {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid())
        return;
    if ([attribute isEqualToString:NSAccessibilityFocusedAttribute]) {
        if (QAccessibleActionInterface *action = iface->actionInterface())
            action->doAction(QAccessibleActionInterface::setFocusAction());
    } else if ([attribute isEqualToString:NSAccessibilityValueAttribute]) {
        if (iface->textInterface()) {
            QString text = QString::fromNSString((NSString *)value);
            iface->setText(QAccessible::Value, text);
        } else if (QAccessibleValueInterface *valueIface = iface->valueInterface()) {
            double val = [value doubleValue];
            valueIface->setCurrentValue(val);
        }
    } else if ([attribute isEqualToString:NSAccessibilitySelectedTextRangeAttribute]) {
        if (QAccessibleTextInterface *text = iface->textInterface()) {
            NSRange range = [value rangeValue];
            if (range.length > 0)
                text->setSelection(0, range.location, range.location + range.length);
            else
                text->setCursorPosition(range.location);
        }
    }
}

// actions

- (NSArray *)accessibilityActionNames {
    NSMutableArray *nsActions = [[NSMutableArray new] autorelease];
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid())
        return nsActions;

    const QStringList &supportedActionNames = QAccessibleBridgeUtils::effectiveActionNames(iface);
    for (const QString &qtAction : supportedActionNames) {
        NSString *nsAction = QCocoaAccessible::getTranslatedAction(qtAction);
        if (nsAction)
            [nsActions addObject : nsAction];
    }

    return nsActions;
}

- (NSString *)accessibilityActionDescription:(NSString *)action {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid())
        return nil; // FIXME is that the right return type??
    QString qtAction = QCocoaAccessible::translateAction(action, iface);
    QString description;
    // Return a description from the action interface if this action is not known to the OS.
    if (qtAction.isEmpty()) {
        if (QAccessibleActionInterface *actionInterface = iface->actionInterface()) {
            qtAction = QString::fromNSString((NSString *)action);
            description = actionInterface->localizedActionDescription(qtAction);
        }
    } else {
        description = qAccessibleLocalizedActionDescription(qtAction);
    }
    return description.toNSString();
}

- (void)accessibilityPerformAction:(NSString *)action {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (iface && iface->isValid()) {
        const QString qtAction = QCocoaAccessible::translateAction(action, iface);
        QAccessibleBridgeUtils::performEffectiveAction(iface, qtAction);
    }
}

// misc

- (BOOL)accessibilityIsIgnored {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid())
        return true;
    return QCocoaAccessible::shouldBeIgnored(iface);
}

- (id)accessibilityHitTest:(NSPoint)point {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid()) {
//        qDebug("Hit test: INVALID");
        return NSAccessibilityUnignoredAncestor(self);
    }

    QPointF screenPoint = QCocoaScreen::mapFromNative(point);
    QAccessibleInterface *childInterface = iface->childAt(screenPoint.x(), screenPoint.y());
    // No child found, meaning we hit this element.
    if (!childInterface || !childInterface->isValid())
        return NSAccessibilityUnignoredAncestor(self);

    // find the deepest child at the point
    QAccessibleInterface *childOfChildInterface = nullptr;
    do {
        childOfChildInterface = childInterface->childAt(screenPoint.x(), screenPoint.y());
        if (childOfChildInterface && childOfChildInterface->isValid())
            childInterface = childOfChildInterface;
    } while (childOfChildInterface && childOfChildInterface->isValid());

    QAccessible::Id childId = QAccessible::uniqueId(childInterface);
    // hit a child, forward to child accessible interface.
    QMacAccessibilityElement *accessibleElement = [QMacAccessibilityElement elementWithId:childId];
    if (accessibleElement)
        return NSAccessibilityUnignoredAncestor(accessibleElement);
    return NSAccessibilityUnignoredAncestor(self);
}

- (id)accessibilityFocusedUIElement {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);

    if (!iface || !iface->isValid()) {
        qWarning("FocusedUIElement for INVALID");
        return nil;
    }

    QAccessibleInterface *childInterface = iface->focusChild();
    if (childInterface && childInterface->isValid()) {
        QAccessible::Id childAxid = QAccessible::uniqueId(childInterface);
        QMacAccessibilityElement *accessibleElement = [QMacAccessibilityElement elementWithId:childAxid];
        return NSAccessibilityUnignoredAncestor(accessibleElement);
    }

    return NSAccessibilityUnignoredAncestor(self);
}

- (NSString *) accessibilityHelp {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (iface && iface->isValid()) {
        const QString helpText = iface->text(QAccessible::Help);
        if (!helpText.isEmpty())
            return helpText.toNSString();
    }
    return nil;
}

/*
 * Support for table
 */
- (NSInteger) accessibilityIndex {
    NSInteger index = 0;
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (iface && iface->isValid()) {
        if (self.isManagedByParent) {
            // axid matches the parent table axid so that we can easily find the parent table
            // children of row are cell/any items
            if (QAccessibleTableInterface *table = iface->tableInterface()) {
                QMacAccessibilityElement *tableElement = [QMacAccessibilityElement elementWithId:axid];
                NSArray *track = synthesizedRole == NSAccessibilityRowRole
                               ? tableElement->rows : tableElement->columns;
                if (track) {
                    NSUInteger trackIndex = [track indexOfObjectIdenticalTo:self];
                    index = (NSInteger)trackIndex;
                }
            }
        }
    }
    return index;
}

- (NSArray *) accessibilityRows {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (iface && iface->isValid() && iface->tableInterface() && !synthesizedRole) {
        if (rows)
            return NSAccessibilityUnignoredChildren(rows);
    }
    return nil;
}

- (NSArray *) accessibilityColumns {
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (iface && iface->isValid() && iface->tableInterface() && !synthesizedRole) {
        if (columns)
            return NSAccessibilityUnignoredChildren(columns);
    }
    return nil;
}

@end

#endif // QT_CONFIG(accessibility)

