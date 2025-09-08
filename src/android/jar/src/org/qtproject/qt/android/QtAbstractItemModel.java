// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import java.util.HashMap;

/**
 * QtAbstractItemModel is a base class for implementing custom models in Java,
 * similar to the <a href="https://doc.qt.io/qt-6/qabstractitemmodel.html">QAbstractItemModel</a>
 * class.
 *
 * The QtAbstractItemModel class defines the standard interface that item
 * models must use to be able to interoperate with other components in the
 * model/view architecture. It is not supposed to be instantiated directly.
 * Instead, you should extend it to create new models.
 *
 * A QtAbstractItemModel can be used as the underlying data model for the
 * item view elements in QML.
 *
 * If you need a model to use with an item view, such as QML's ListView element,
 * you should consider extending {@link QtAbstractListModel} instead of this class.
 *
 * The underlying data model is exposed to views and delegates as a hierarchy
 * of tables. If you do not use the hierarchy, the model is a simple table of
 * rows and columns. Each item has a unique index specified by a {@link QtModelIndex}.
 *
 *
 * Every item of data that can be accessed via a model has an associated model
 * index. You can obtain this model index using the {@link #index(int, int, QtModelIndex)} method.
 * Each index may have a {@link #sibling(int, int, QtModelIndex)} index; child items have a
 * {@link #parent(QtModelIndex)} index.
 *
 * Each item has data elements associated with it, and they can be
 * retrieved by specifying a role to the model's {@link #data(QtModelIndex, int)} function.
 *
 * If an item has child objects, {@link #hasChildren(QtModelIndex)} returns true for the
 * corresponding index.
 *
 * The model has a {@link #rowCount(QtModelIndex)} and a {@link #columnCount(QtModelIndex)}
 * for each level of the hierarchy.
 *
 * Extending QtAbstractItemModel:
 *
 * Some general guidelines for sub-classing models are available in the
 * <a href="https://doc.qt.io/qt-6/model-view-programming.html#model-subclassing-reference">model sub-classing reference</a>.
 *
 * When sub-classing QtAbstractItemModel, at the very least, you must implement
 * {@link #index(int, int, QtModelIndex)}, {@link #parent(QtModelIndex)},
 * {@link #rowCount(QtModelIndex)}, {@link #columnCount(QtModelIndex)}, and
 * {@link #data(QtModelIndex, int)}. These abstract methods are used in all models.
 *
 * You can also re-implement {@link #hasChildren(QtModelIndex)} to provide special behavior for
 * models where the implementation of {@link #rowCount(QtModelIndex)} is expensive. This makes it
 * possible for models to restrict the amount of data requested by views and
 * can be used as a way to implement the population of model data.
 *
 * Custom models need to create model indexes for other components to use. To
 * do this, call {@link #createIndex(int, int, long)} with suitable row and column numbers for the
 * item, and a long type identifier for it.
 * The combination of these values must be unique for each item. Custom models
 * typically use these unique identifiers in other re-implemented functions to
 * retrieve item data and access information about the item's parents and
 * children.
 *
 * To create models that populate incrementally, you can re-implement
 * {@link #fetchMore(QtModelIndex)} and {@link #canFetchMore(QtModelIndex)}.
 * If the re-implementation of {@link #fetchMore(QtModelIndex)} adds
 * rows to the model, {@link QtAbstractItemModel#beginInsertRows(QtModelIndex, int, int)} and
 * {@link QtAbstractItemModel#endInsertRows()} must be called.
 * @since 6.8
*/
public abstract class QtAbstractItemModel
{
    /**
     * Constructs a new QtAbstractItemModel.
     */
    public QtAbstractItemModel(){}

    /**
     * Returns the number of columns for the children of the given parent.
     * In most subclasses, the number of columns is independent of the parent.
     *
     * For example:
     * <pre>
     * &#64;Override
     * int columnCount(const QtModelIndex parent)
     * {
     *   return 3;
     * }
     * </pre>
     * When implementing a table-based model, columnCount() should return 0,
     * when the parent is valid.
     *
     * @param parent The parent index.
     * @return The number of columns.
     * @see #rowCount(QtModelIndex)
     */
    public abstract int columnCount(QtModelIndex parent);
    /**
     * Returns the data for the given index and role.
     * Types conversions are:
     * Java -&gt; QML
     * Integer -&gt; int
     * String -&gt; string
     * Double -&gt; double
     * Double -&gt; real
     * Boolean -&gt; bool
     *
     * @param index The index.
     * @param role The role.
     * @return The data object.
     */
    public abstract Object data(QtModelIndex index, int role);
    /**
     * Returns the index for the specified row and column for the supplied parent index.
     * When re-implementing this function in a subclass, call createIndex() to generate model
     * indexes that other components can use to refer to items in your model.
     *
     * @param row The row.
     * @param column The column.
     * @param parent The parent index.
     * @return The index.
     * @see #createIndex(int row, int column, long id)
     */
    public abstract QtModelIndex index(int row, int column, QtModelIndex parent);
    /**
     * Returns the parent of the model item with the given index. If the item
     * has no parent, then an invalid QtModelIndex is returned.
     *
     * A common convention used in models that expose tree data structures is that
     * only items in the first column have children. For that case, when
     * re-implementing this function in a subclass, the column of the returned
     * QtModelIndex would be 0.

     * When re-implementing this function in a subclass, be careful to avoid
     * calling QtModelIndex member functions, such as {@link #parent(QtModelIndex)},
     * since indexes belonging to your model will call your implementation,
     * leading to infinite recursion.
     *
     * @param index The index.
     * @return The parent index.
     * @see #createIndex(int row, int column, long id)
     */
    public abstract QtModelIndex parent(QtModelIndex index);
    /**
     * Returns the number of rows under the given parent. When the parent is
     * valid, it means that rowCount is returning the number of children of the parent.

     * Note: when implementing a table-based model, rowCount() should return 0,
     * when the parent is valid.
     *
     * @param parent The parent index.
     * @return The number of rows.
     * @see #columnCount(QtModelIndex parent)
     */
    public abstract int rowCount(QtModelIndex parent);
    /**
     * Returns whether more items can be fetched for the given parent index.
     *
     * @param parent The parent index.
     * @return True if more items can be fetched, false otherwise.
     */
    public native boolean canFetchMore(QtModelIndex parent);
    /**
     *  Fetches any available data for the items with the parent specified by the
     *  parent index.
     *
     * @param parent The parent index.
     */
    public native void fetchMore(QtModelIndex parent);
    /**
     * Returns true if the parent has any children; otherwise, returns false.
     * Use rowCount() on the parent to get the number of children.
     *
     * @param parent The parent index.
     * @return True if the parent has children, false otherwise.
     * @see #parent(QtModelIndex index)
     * @see #index(int row, int column, QtModelIndex parent)
     */
    public native boolean hasChildren(QtModelIndex parent);
    /**
     * Returns true if the model returns a valid QModelIndex for row and
     * column with parent, otherwise returns \c{false}.
     *
     * @param row The row.
     * @param column The column.
     * @param parent The parent index.
     * @return True if the index exists, false otherwise.
     */
    public native boolean hasIndex(int row, int column, QtModelIndex parent);
    /**
     * Returns a map of role names.
     * You must override this to provide your own role names or the
     * <a href="https://doc.qt.io/qt-6/qabstractitemmodel.html#roleNames">defaults</a>
     * will be used.
     *
     * @return The role names map.
     */
    @SuppressWarnings("unchecked")
    public HashMap<Integer, String> roleNames()
    {
        return (HashMap<Integer, String>)jni_roleNames();
    }
    /**
     * Returns the sibling at row and column for the item at index or an
     * invalid QModelIndex if there is no sibling at that location.
     *
     * sibling() is just a convenience function that finds the item's parent and
     * uses it to retrieve the index of the child item in the specified row and
     * column.
     *
     * This method can optionally be overridden to optimize a specific implementation.
     *
     * @param row The row.
     * @param column The column.
     * @param parent The parent index.
     * @return The sibling index.
     */
    public QtModelIndex sibling(int row, int column, QtModelIndex parent)
    {
        return (QtModelIndex)jni_sibling(row, column, parent);
    }
    /**
     * Sets the role data for the item at {@code index} to value.
     *
     * Returns {@code true} if successful; otherwise returns false.
     *
     * The {@link #dataChanged(QtModelIndex, QtModelIndex, int[])} method should be called
     * if the data was successfully set.
     *
     * The base class implementation returns {@code false}. This method and
     * {@link #data(QtModelIndex, int)} must be reimplemented for editable models.
     *
     * @param index The index of the item
     * @param value The data value
     * @param role The role
     * @return True if successful, otherwise return false.
     */
    public boolean setData(QtModelIndex index, Object value, int role) {
        return jni_setData(index, value, role);
    }
    /**
     * This method must be called whenever the data in an existing item changes.
     *
     * If the items have the same parent, the affected ones are inclusively those between
     * {@code topLeft} and {@code bottomRight}. If the items do not have the same
     * parent, the behavior is undefined.
     *
     * When reimplementing the {@link #setData(QtModelIndex, Object, int)} method, this method
     * must be called explicitly.
     *
     * The {@code roles} argument specifies which data roles have actually
     * been modified. An empty array in the roles argument means that all roles should be
     * considered modified. The order of elements in the {@code roles} argument does not have any
     * relevance.
     *
     * @param topLeft The top-left index of changed items
     * @param bottomRight The bottom-right index of changed items
     * @param roles Changed roles; Empty array indicates all roles
     * @see #setData(QtModelIndex index, Object value, int role)
     */
    public void dataChanged(QtModelIndex topLeft, QtModelIndex bottomRight, int[] roles) {
        jni_dataChanged(topLeft, bottomRight, roles);
    }
    /**
     * Interface for a callback to be invoked when the data in an existing item changes.
     */
    public interface OnDataChangedListener {
        /**
         * Called when the data in an existing item changes.
         *
         * @param topLeft The top-left index of changed items
         * @param bottomRight The bottom-right index of changed items
         * @param roles Changed roles; Empty array indicates all roles
         */
        void onDataChanged(QtModelIndex topLeft, QtModelIndex bottomRight, int[] roles);
    }
    /**
     * Register a callback to be invoked when the data in an existing item changes.
     *
     * @param listener The data change listener
     */
    public void setOnDataChangedListener(OnDataChangedListener listener) {
        m_OnDataChangedListener = listener;
    }
    /**
     * Begins a column insertion operation.

     * The parent index corresponds to the parent into which the new columns
     * are inserted; first and last are the column numbers of the new
     * columns will have after they have been inserted.
     *
     * @see #endInsertColumns()
    */
    protected final void beginInsertColumns(QtModelIndex parent, int first, int last)
    {
        jni_beginInsertColumns(parent, first, last);
    }
    /**
     * Begins a row insertion operation.
     * Parent index corresponds to the parent into which the new rows
     * are inserted; first and last are the row numbers the new rows will have
     * after they have been inserted.

     * @see #endInsertRows()
    */
    protected final void beginInsertRows(QtModelIndex parent, int first, int last)
    {
        jni_beginInsertRows(parent, first, last);
    }
    /**
     Begins a column move operation.

     When re-implementing a subclass, this method simplifies moving
     entities in your model. This method is responsible for moving
     persistent indexes in the model.

     The sourceParent index corresponds to the parent from which the
     columns are moved; sourceFirst and sourceLast are the first and last
     column numbers of the columns to be moved. The destinationParent index
     corresponds to the parent into which those columns are moved. The
     destinationChild is the column to which the columns will be moved.  That
     is, the index at column sourceFirst in sourceParent will become
     column destinationChild in destinationParent, followed by all other
     columns up to sourceLast.

     However, when moving columns down in the same parent (sourceParent
     and destinationParent are equal), the columns will be placed before the
     destinationChild index. If you wish to move columns 0 and 1 so
     they will become columns 1 and 2, destinationChild should be 3. In this
     case, the new index for the source column i (which is between
     sourceFirst and sourceLast) is equal to (destinationChild-sourceLast-1+i).

     Note that if sourceParent and destinationParent are the same,
     you must ensure that the destinationChild is not within the range
     of sourceFirst and sourceLast + 1.  You must also ensure that you
     do not attempt to move a column to one of its own children or ancestors.
     This method returns false if either condition is true, in which case you
     should abort your move operation.

     @see endMoveColumns()
    */
    protected final boolean beginMoveColumns(QtModelIndex sourceParent, int sourceFirst,
                                             int sourceLast, QtModelIndex destinationParent,
                                             int destinationChild)
    {
        return jni_beginMoveColumns(sourceParent, sourceFirst, sourceLast, destinationParent,
                                    destinationChild);
    }
    /**
     * Begins a row move operation.

     * When re-implementing a subclass, this method simplifies moving
     * entities in your model.

     * The  sourceParent index corresponds to the parent from which the
     * rows are moved; sourceFirst and sourceLast are the first and last
     * row numbers of the rows to be moved. The destinationParent index
     * corresponds to the parent into which those rows are moved. The
     * destinationChild is the row to which the rows will be moved. That
     * is, the index at row  sourceFirst in  sourceParent will become
     * row destinationChild in destinationParent, followed by all other
     * rows up to  sourceLast.

     * However, when moving rows down in the same parent (sourceParent
     * and  destinationParent are equal), the rows will be placed before the
     *  destinationChild index. That is, if you wish to move rows 0 and 1 so
     * they will become rows 1 and 2, destinationChild should be 3. In this
     * case, the new index for the source row i (which is between
     * sourceFirst and  sourceLast) is equal to
     * (destinationChild-sourceLast-1+i).

     * Note that if  sourceParent and  destinationParent are the same,
     * you must ensure that the  destinationChild is not within the range
     * of  sourceFirst and  sourceLast + 1. You must also ensure that you
     * do not attempt to move a row to one of its own children or ancestors.
     * This method returns false if either condition is true, in which case you
     * should abort your move operation.

     * @see <a href="https://doc.qt.io/qt-6/qabstractitemmodel.html#beginMoveRows">PossibleOps</a>
     * @see #endMoveRows()
    */
    protected final boolean beginMoveRows(QtModelIndex sourceParent, int sourceFirst,
                                          int sourceLast, QtModelIndex destinationParent,
                                          int destinationChild)
    {
        return jni_beginMoveRows(sourceParent, sourceFirst, sourceLast, destinationParent,
                              destinationChild);
    }
    /**
     * Begins a column removal operation.

     * When re-implementing removeColumns() in a subclass, you must call this
     * function before removing data from the model's underlying data store.

     * The  parent index corresponds to the parent from which the new columns
     * are removed;  first and  last are the column numbers of the first and
     * last columns to be removed.
     *
     * @see <a href="https://doc.qt.io/qt-6/qabstractitemmodel.html#beginRemoveColumns">RemoveColums</a>
     * @see #endRemoveColumns()
    */
    protected final void beginRemoveColumns(QtModelIndex parent, int first, int last)
    {
        jni_beginRemoveColumns(parent, first, last);
    }
    /**
     * Begins a row removal operation.

     * When re-implementing removeRows() in a subclass, you must call this
     * function before removing data from the model's underlying data store.

     * The  parent index corresponds to the parent from which the new rows are
     * removed;  first and  last are the row numbers of the rows to be.

     * @see #endRemoveRows()
     * @see <a href="https://doc.qt.io/qt-6/qabstractitemmodel.html#beginRemoveRows">RemoveRows</a>
    */
    protected final void beginRemoveRows(QtModelIndex parent, int first, int last)
    {
        jni_beginRemoveRows(parent, first, last);
    }
    /**
     * Begins a model reset operation.

     * A reset operation resets the model to its current state in any attached views.

     * Note: any views attached to this model will also be reset.

     * When a model is reset, any previous data reported from the
     * model is now invalid and has to be queried again. This also means that
     * the current and any selected items will become invalid.

     * When a model radically changes its data, it can sometimes be easier to just
     * call this function rather than emit dataChanged() to inform other
     * components when the underlying data source, or its structure, has changed.

     * You must call this function before resetting any internal data structures
      in your model.

     * @see #endResetModel()
     * @see <a href="https://doc.qt.io/qt-6/qabstractitemmodel.html#modelAboutToBeReset">modelAboutToBeReset</a>
     * @see <a href="https://doc.qt.io/qt-6/qabstractitemmodel.html#modelReset">modelReset</a>
    */
    protected final void beginResetModel() { jni_beginResetModel(); }

    /**
     * Creates a model index for the given row and column with the internal
     * identifier id.

     * This function provides a consistent interface, which model sub classes must
     * use to create model indexes.
    */
    protected final QtModelIndex createIndex(int row, int column, long id)
    {
        return (QtModelIndex)jni_createIndex(row, column, id);
    }
    /**
    * Ends a column insertion operation.

    * When re-implementing insertColumns() in a subclass, you must call this
    * function after inserting data into the model's underlying data
    * store.

    * @see #beginInsertColumns(QtModelIndex, int, int)
    */
    protected final void endInsertColumns() { jni_endInsertColumns(); }
    /**
     * Ends a row insertion operation.

     * When re-implementing insertRows() in a subclass, you must call this function
     * after inserting data into the model's underlying data store.

     * @see #beginInsertRows(QtModelIndex, int, int)
    */
    protected final void endInsertRows() { jni_endInsertRows(); }
    /**
    * Ends a column move operation.

    * When implementing a subclass, you must call this
    * function after moving data within the model's underlying data
    * store.

    * @see #beginMoveColumns(QtModelIndex, int, int, QtModelIndex, int)
    */
    protected final void endMoveColumns() { jni_endMoveColumns(); }
    /**
    Ends a row move operation.

    When implementing a subclass, you must call this
    function after moving data within the model's underlying data
    store.

    @see #beginMoveRows(QtModelIndex, int, int, QtModelIndex, int)
    */
    protected final void endMoveRows() { jni_endMoveRows(); }
    /**
     * Ends a column removal operation.

     * When reimplementing removeColumns() in a subclass, you must call this
     * function after removing data from the model's underlying data store.

     * @see #beginRemoveColumns(QtModelIndex, int, int)
    */
    protected final void endRemoveColumns() { jni_endRemoveColumns(); }
    /**
     * Ends a row move operation.

     * When implementing a subclass, you must call this
     * function after moving data within the model's underlying data
     * store.

     * @see #beginMoveRows(QtModelIndex, int, int , QtModelIndex, int)
    */
    protected final void endRemoveRows() { jni_endRemoveRows(); }
    /**
     * Completes a model reset operation.

     * You must call this function after resetting any internal data structure in your model.

     * @see #beginResetModel()
    */
    protected final void endResetModel() { jni_endResetModel(); }

    private void handleDataChanged(QtModelIndex topLeft, QtModelIndex bottomRight, int[] roles)
    {
        if (m_OnDataChangedListener != null) {
            QtNative.runAction(() -> {
                if (m_OnDataChangedListener != null)
                    m_OnDataChangedListener.onDataChanged(topLeft, bottomRight, roles);
            });
        }
    }
    private native void jni_beginInsertColumns(QtModelIndex parent, int first, int last);
    private native void jni_beginInsertRows(QtModelIndex parent, int first, int last);
    private native boolean jni_beginMoveColumns(QtModelIndex sourceParent, int sourceFirst,
                                                int sourceLast, QtModelIndex destinationParent,
                                                int destinationChild);
    private native boolean jni_beginMoveRows(QtModelIndex sourceParent, int sourceFirst,
                                             int sourceLast, QtModelIndex destinationParent,
                                             int destinationChild);
    private native void jni_beginRemoveColumns(QtModelIndex parent, int first, int last);
    private native void jni_beginRemoveRows(QtModelIndex parent, int first, int last);
    private native void jni_beginResetModel();
    private native Object jni_createIndex(int row, int column, long id);
    private native void jni_endInsertColumns();
    private native void jni_endInsertRows();
    private native void jni_endMoveColumns();
    private native void jni_endMoveRows();
    private native void jni_endRemoveColumns();
    private native void jni_endRemoveRows();
    private native void jni_endResetModel();
    private native Object jni_roleNames();
    private native Object jni_sibling(int row, int column, QtModelIndex parent);
    private native boolean jni_setData(QtModelIndex index, Object value, int role);
    private native void jni_dataChanged(QtModelIndex topLeft, QtModelIndex bottomRight,
                                        int[] roles);

    private long m_nativeReference = 0;
    private OnDataChangedListener m_OnDataChangedListener;
    private QtAbstractItemModel(long nativeReference) { m_nativeReference = nativeReference; }
    private void detachFromNative() { m_nativeReference = 0; }

    private long nativeReference() { return m_nativeReference; }
    private void setNativeReference(long nativeReference) { m_nativeReference = nativeReference; }
    private static boolean instanceOf(Object obj) { return (obj instanceof QtAbstractItemModel); }
}
