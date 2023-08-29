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
    int m_rowIndex;
    int m_columnIndex;

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
        m_rowIndex = -1;
        m_columnIndex = -1;
        rows = nil;
        columns = nil;
        synthesizedRole = role;
        // table: if this is not created as an element managed by the table, then
        // it's either the table itself, or an element created for an already existing
        // cell interface (or an element that's not at all related to a table).
        if (!synthesizedRole) {
            if (QAccessibleInterface *iface = QAccessible::accessibleInterface(axid)) {
                if (iface->tableInterface()) {
                    [self updateTableModel];
                } else if (const auto *cell = iface->tableCellInterface()) {
                    // If we create an element for a table cell, initialize it with row/column
                    // and insert it into the corresponding row's columns array.
                    m_rowIndex = cell->rowIndex();
                    m_columnIndex = cell->columnIndex();
                    QAccessibleInterface *table = cell->table();
                    Q_ASSERT(table);
                    QAccessibleTableInterface *tableInterface = table->tableInterface();
                    if (tableInterface) {
                        auto *tableElement = [QMacAccessibilityElement elementWithInterface:table];
                        Q_ASSERT(tableElement);
                        Q_ASSERT(tableElement->rows);
                        Q_ASSERT(int(tableElement->rows.count) > m_rowIndex);
                        auto *rowElement = tableElement->rows[m_rowIndex];
                        if (!rowElement->columns) {
                            rowElement->columns = [rowElement populateTableRow:rowElement->columns
                                                              count:tableInterface->columnCount()];
                        }
                        rowElement->columns[m_columnIndex] = self;
                    }
                }
            }
        }
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
        Q_ASSERT(QAccessible::accessibleInterface(anId));
        element = [[self alloc] initWithId:anId];
        cache->insertElement(anId, element);
    }
    return element;
}

+ (instancetype)elementWithInterface:(QAccessibleInterface *)iface
{
    Q_ASSERT(iface);
    if (!iface)
        return nil;

    const QAccessible::Id anId = QAccessible::uniqueId(iface);
    return [self elementWithId:anId];
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
    if (QAccessibleInterface *iface = self.qtInterface) {
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
                if (role == NSAccessibilityRowRole)
                    element->m_rowIndex = n;
                else if (role == NSAccessibilityColumnRole)
                    element->m_columnIndex = n;
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

- (NSMutableArray *)populateTableRow:(NSMutableArray *)array count:(int)count
{
    Q_ASSERT(synthesizedRole == NSAccessibilityRowRole);
    if (!array) {
        array = [NSMutableArray<QMacAccessibilityElement *> arrayWithCapacity:count];
        [array retain];
        // When macOS asks for the children of a row, then we populate the row's column
        // array with synthetic elements as place holders. This way, we don't have to
        // create QAccessibleInterfaces for every cell before they are really needed.
        // We don't add those synthetic elements into the cache, and we give them the
        // same axid as the table. This way, we can get easily to the table, and from
        // there to the QAccessibleInterface for the cell, when we have to eventually
        // associate such an interface with the element (at which point it is no longer
        // a placeholder).
        for (int n = 0; n < count; ++n) {
            // columns will have same axid as table (but not inserted in cache)
            QMacAccessibilityElement *cell =
                    [[QMacAccessibilityElement alloc] initWithId:axid role:NSAccessibilityCellRole];
            if (cell) {
                cell->m_rowIndex = m_rowIndex;
                cell->m_columnIndex = n;
                [array addObject:cell];
            }
        }
    }
    Q_ASSERT(array);
    return array;
}

- (void)updateTableModel
{
    if (QAccessibleInterface *iface = self.qtInterface) {
        if (QAccessibleTableInterface *table = iface->tableInterface()) {
            Q_ASSERT(!self.isManagedByParent);
            rows = [self populateTableArray:rows role:NSAccessibilityRowRole count:table->rowCount()];
            columns = [self populateTableArray:columns role:NSAccessibilityColumnRole count:table->columnCount()];
        }
    }
}

- (QAccessibleInterface *)qtInterface
{
    QAccessibleInterface *iface = QAccessible::accessibleInterface(axid);
    if (!iface || !iface->isValid())
        return nullptr;

    // If this is a placeholder element for a table cell, associate it with the
    // cell interface (which will be created now, if needed). The current axid is
    // for the table to which the cell belongs, so iface is pointing at the table.
    if (synthesizedRole == NSAccessibilityCellRole) {
        // get the cell interface - there must be a valid one
        QAccessibleTableInterface *table = iface->tableInterface();
        Q_ASSERT(table);
        QAccessibleInterface *cell = table->cellAt(m_rowIndex, m_columnIndex);
        if (!cell)
            return nullptr;
        Q_ASSERT(cell->isValid());
        iface = cell;

        // no longer a placeholder
        axid = QAccessible::uniqueId(cell);
        synthesizedRole = nil;

        QAccessibleCache *cache = QAccessibleCache::instance();
        if (QMacAccessibilityElement *cellElement = cache->elementForId(axid)) {
            // there already is another, non-placeholder element in the cache
            Q_ASSERT(cellElement->synthesizedRole == nil);
            // we have to release it if it's not us
            if (cellElement != self) {
                // for the same cell position
                Q_ASSERT(cellElement->m_rowIndex == m_rowIndex && cellElement->m_columnIndex == m_columnIndex);
                [cellElement release];
            }
        }

        cache->insertElement(axid, self);
    }
    return iface;
}

//
// accessibility protocol
//

- (BOOL)isAccessibilityFocused
{
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
    // shortcut for cells, rows, and columns in a table
    if (synthesizedRole)
        return synthesizedRole;
    if (QAccessibleInterface *iface = self.qtInterface)
        return QCocoaAccessible::macRole(iface);
    return NSAccessibilityUnknownRole;
}

- (NSString *) accessibilitySubRole {
    if (QAccessibleInterface *iface = self.qtInterface)
        return QCocoaAccessible::macSubrole(iface);
    return NSAccessibilityUnknownRole;
}

- (NSString *) accessibilityRoleDescription {
    if (QAccessibleInterface *iface = self.qtInterface)
        return NSAccessibilityRoleDescription(self.accessibilityRole, self.accessibilitySubRole);
    return NSAccessibilityUnknownRole;
}

- (NSArray *) accessibilityChildren {
    // shortcut for cells
    if (synthesizedRole == NSAccessibilityCellRole)
        return nil;

    QAccessibleInterface *iface = self.qtInterface;
    if (!iface)
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
            Q_ASSERT(m_rowIndex >= 0);
            const int numColumns = table->columnCount();
            columns = [self populateTableRow:columns count:numColumns];
            return NSAccessibilityUnignoredChildren(columns);
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
    if (QAccessibleInterface *iface = self.qtInterface) {
        if (iface->role() == QAccessible::StaticText)
            return nil;
        if (self.isManagedByParent)
            return nil;
        return iface->text(QAccessible::Name).toNSString();
    }
    return nil;
}

- (BOOL) isAccessibilityEnabled {
    if (QAccessibleInterface *iface = self.qtInterface)
        return !iface->state().disabled;
    return false;
}

- (id)accessibilityParent {
    if (synthesizedRole == NSAccessibilityCellRole) {
        // a synthetic cell without interface - shortcut to the row
        QMacAccessibilityElement *tableElement =
                    [QMacAccessibilityElement elementWithId:axid];
        Q_ASSERT(tableElement && tableElement->rows);
        Q_ASSERT(int(tableElement->rows.count) > m_rowIndex);
        QMacAccessibilityElement *rowElement = tableElement->rows[m_rowIndex];
        return rowElement;
    }

    QAccessibleInterface *iface = self.qtInterface;
    if (!iface)
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
            QMacAccessibilityElement *tableElement =
                    [QMacAccessibilityElement elementWithInterface:parent];

            // parent of cell should be row
            int rowIndex = -1;
            if (m_rowIndex >= 0 && m_columnIndex >= 0)
                rowIndex = m_rowIndex;
            else if (QAccessibleTableCellInterface *cell = iface->tableCellInterface())
                rowIndex = cell->rowIndex();
            Q_ASSERT(tableElement->rows);
            if (rowIndex > int([tableElement->rows count]))
                return nil;
            QMacAccessibilityElement *rowElement = tableElement->rows[rowIndex];
            return NSAccessibilityUnignoredAncestor(rowElement);
        }
        if (parent->role() != QAccessible::Application)
            return NSAccessibilityUnignoredAncestor([QMacAccessibilityElement elementWithInterface: parent]);
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
    QAccessibleInterface *iface = self.qtInterface;
    if (!iface)
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

            NSUInteger trackIndex = self.accessibilityIndex;
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
    if (QAccessibleInterface *iface = self.qtInterface)
        return iface->text(QAccessible::Description).toNSString();
    qWarning() << "Called accessibilityLabel on invalid object: " << axid;
    return nil;
}

- (void)setAccessibilityLabel:(NSString*)label{
    if (QAccessibleInterface *iface = self.qtInterface)
        iface->setText(QAccessible::Description, QString::fromNSString(label));
}

- (id) accessibilityValue {
    if (QAccessibleInterface *iface = self.qtInterface) {
        // VoiceOver asks for the value attribute for all elements. Return nil
        // if we don't want the element to have a value attribute.
        if (QCocoaAccessible::hasValueAttribute(iface))
            return QCocoaAccessible::getValueAttribute(iface);
    }
    return nil;
}

- (NSInteger) accessibilityNumberOfCharacters {
    if (QAccessibleInterface *iface = self.qtInterface) {
        if (QAccessibleTextInterface *text = iface->textInterface())
            return text->characterCount();
    }
    return 0;
}

- (NSString *) accessibilitySelectedText {
    if (QAccessibleInterface *iface = self.qtInterface) {
        if (QAccessibleTextInterface *text = iface->textInterface()) {
            int start = 0;
            int end = 0;
            text->selection(0, &start, &end);
            return text->text(start, end).toNSString();
        }
    }
    return nil;
}

- (NSRange) accessibilitySelectedTextRange {
    QAccessibleInterface *iface = self.qtInterface;
    if (!iface)
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
    QAccessibleInterface *iface = self.qtInterface;
    if (!iface)
        return 0;
    if (QAccessibleTextInterface *text = iface->textInterface()) {
        QString textToPos = text->text(0, index);
        return textToPos.count('\n');
    }
    return 0;
}

- (NSRange)accessibilityVisibleCharacterRange {
    QAccessibleInterface *iface = self.qtInterface;
    if (!iface)
        return NSRange();
    // FIXME This is not correct and may impact performance for big texts
    if (QAccessibleTextInterface *text = iface->textInterface())
        return NSMakeRange(0, static_cast<uint>(text->characterCount()));
    return NSMakeRange(0, static_cast<uint>(iface->text(QAccessible::Name).length()));
}

- (NSInteger) accessibilityInsertionPointLineNumber {
    QAccessibleInterface *iface = self.qtInterface;
    if (!iface)
        return 0;
    if (QAccessibleTextInterface *text = iface->textInterface()) {
        int position = text->cursorPosition();
        return [self accessibilityLineForIndex:position];
    }
    return 0;
}

- (NSArray *)accessibilityParameterizedAttributeNames {

    QAccessibleInterface *iface = self.qtInterface;
    if (!iface) {
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
    QAccessibleInterface *iface = self.qtInterface;
    if (!iface) {
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
    QAccessibleInterface *iface = self.qtInterface;
    if (!iface)
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
    QAccessibleInterface *iface = self.qtInterface;
    if (!iface)
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
    QAccessibleInterface *iface = self.qtInterface;
    if (!iface)
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
    QAccessibleInterface *iface = self.qtInterface;
    if (!iface)
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
    if (QAccessibleInterface *iface = self.qtInterface) {
        const QString qtAction = QCocoaAccessible::translateAction(action, iface);
        QAccessibleBridgeUtils::performEffectiveAction(iface, qtAction);
    }
}

// misc

- (BOOL)accessibilityIsIgnored {
    // Short-cut for placeholders and synthesized elements. Working around a bug
    // that corrups lists returned by NSAccessibilityUnignoredChildren, otherwise
    // we could ignore rows and columns that are outside the table.
    if (self.isManagedByParent)
        return false;

    if (QAccessibleInterface *iface = self.qtInterface)
        return QCocoaAccessible::shouldBeIgnored(iface);
    return true;
}

- (id)accessibilityHitTest:(NSPoint)point {
    QAccessibleInterface *iface = self.qtInterface;
    if (!iface) {
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

    // hit a child, forward to child accessible interface.
    QMacAccessibilityElement *accessibleElement = [QMacAccessibilityElement elementWithInterface:childInterface];
    if (accessibleElement)
        return NSAccessibilityUnignoredAncestor(accessibleElement);
    return NSAccessibilityUnignoredAncestor(self);
}

- (id)accessibilityFocusedUIElement {
    QAccessibleInterface *iface = self.qtInterface;
    if (!iface) {
        qWarning("FocusedUIElement for INVALID");
        return nil;
    }

    QAccessibleInterface *childInterface = iface->focusChild();
    if (childInterface && childInterface->isValid()) {
        QMacAccessibilityElement *accessibleElement = [QMacAccessibilityElement elementWithInterface:childInterface];
        return NSAccessibilityUnignoredAncestor(accessibleElement);
    }

    return NSAccessibilityUnignoredAncestor(self);
}

- (NSString *) accessibilityHelp {
    if (QAccessibleInterface *iface = self.qtInterface) {
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
    if (synthesizedRole == NSAccessibilityCellRole)
        return m_columnIndex;
    if (QAccessibleInterface *iface = self.qtInterface) {
        if (self.isManagedByParent) {
            // axid matches the parent table axid so that we can easily find the parent table
            // children of row are cell/any items
            if (QAccessibleTableInterface *table = iface->tableInterface()) {
                if (m_rowIndex >= 0)
                    index = NSInteger(m_rowIndex);
                else if (m_columnIndex >= 0)
                    index = NSInteger(m_columnIndex);
            }
        }
    }
    return index;
}

- (NSArray *) accessibilityRows {
    if (!synthesizedRole && rows) {
        QAccessibleInterface *iface = self.qtInterface;
        if (iface && iface->tableInterface())
            return NSAccessibilityUnignoredChildren(rows);
    }
    return nil;
}

- (NSArray *) accessibilityColumns {
    if (!synthesizedRole && columns) {
        QAccessibleInterface *iface = self.qtInterface;
        if (iface && iface->tableInterface())
            return NSAccessibilityUnignoredChildren(columns);
    }
    return nil;
}

@end

#endif // QT_CONFIG(accessibility)

