/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qwinrtdrag.h"

#include <QtCore/qglobal.h>
#include <QtCore/QMimeData>
#include <QtCore/QStringList>
#include <QtGui/QGuiApplication>
#include <qpa/qwindowsysteminterface.h>

#include <qfunctions_winrt.h>
#include <private/qeventdispatcher_winrt_p.h>

#include <Windows.ApplicationModel.datatransfer.h>
#include <windows.ui.xaml.h>
#include <windows.foundation.collections.h>
#include <windows.graphics.imaging.h>
#include <windows.storage.streams.h>
#include <functional>
#include <robuffer.h>

using namespace ABI::Windows::ApplicationModel::DataTransfer;
using namespace ABI::Windows::ApplicationModel::DataTransfer::DragDrop;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::Graphics::Imaging;
using namespace ABI::Windows::Storage;
using namespace ABI::Windows::Storage::Streams;
using namespace ABI::Windows::UI::Xaml;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaMime, "qt.qpa.mime")

ComPtr<IBuffer> createIBufferFromData(const char *data, qint32 size)
{
    static ComPtr<IBufferFactory> bufferFactory;
    HRESULT hr;
    if (!bufferFactory) {
        hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_Streams_Buffer).Get(),
                                    IID_PPV_ARGS(&bufferFactory));
        Q_ASSERT_SUCCEEDED(hr);
    }

    ComPtr<IBuffer> buffer;
    const UINT32 length = size;
    hr = bufferFactory->Create(length, &buffer);
    Q_ASSERT_SUCCEEDED(hr);
    hr = buffer->put_Length(length);
    Q_ASSERT_SUCCEEDED(hr);

    ComPtr<Windows::Storage::Streams::IBufferByteAccess> byteArrayAccess;
    hr = buffer.As(&byteArrayAccess);
    Q_ASSERT_SUCCEEDED(hr);

    byte *bytes;
    hr = byteArrayAccess->Buffer(&bytes);
    Q_ASSERT_SUCCEEDED(hr);
    memcpy(bytes, data, length);
    return buffer;
}

class DragThreadTransferData : public QObject
{
    Q_OBJECT
public slots:
    void handleDrag();
public:
    explicit DragThreadTransferData(QObject *parent = nullptr);
    QWindow *window;
    QWinRTInternalMimeData *mime;
    QPoint point;
    Qt::DropActions actions;
    bool dropAction;
    ComPtr<IDragEventArgs> nativeArgs;
    ComPtr<IDragOperationDeferral> deferral;
};

inline QString hStringToQString(const HString &hString)
{
    quint32 l;
    const wchar_t *raw = hString.GetRawBuffer(&l);
    return (QString::fromWCharArray(raw, l));
}

inline HString qStringToHString(const QString &qString)
{
    HString h;
    h.Set(reinterpret_cast<const wchar_t*>(qString.utf16()), qString.size());
    return h;
}

namespace NativeFormatStrings {
    static ComPtr<IStandardDataFormatsStatics> dataStatics;
    static HSTRING text; // text/plain
    static HSTRING html; // text/html
    static HSTRING storage; // text/uri-list
}

static inline DataPackageOperation translateFromQDragDropActions(const Qt::DropAction action)
{
    switch (action) {
    case Qt::CopyAction:
        return DataPackageOperation_Copy;
    case Qt::MoveAction:
        return DataPackageOperation_Move;
    case Qt::LinkAction:
        return DataPackageOperation_Link;
    case Qt::IgnoreAction:
    default:
        return DataPackageOperation_None;
    }
}

static inline Qt::DropActions translateToQDragDropActions(const DataPackageOperation op)
{
    Qt::DropActions actions = Qt::IgnoreAction;
    // None needs to be interpreted as the sender being able to handle
    // anything and let the receiver decide
    if (op == DataPackageOperation_None)
        actions = Qt::LinkAction | Qt::CopyAction | Qt::MoveAction;
    if (op & DataPackageOperation_Link)
        actions |= Qt::LinkAction;
    if (op & DataPackageOperation_Copy)
        actions |= Qt::CopyAction;
    if (op & DataPackageOperation_Move)
        actions |= Qt::MoveAction;
    return actions;
}

QWinRTInternalMimeData::QWinRTInternalMimeData()
    : QInternalMimeData()
{
    qCDebug(lcQpaMime) << __FUNCTION__;
    if (!NativeFormatStrings::dataStatics) {
        HRESULT hr;
        hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_ApplicationModel_DataTransfer_StandardDataFormats).Get(),
                                    IID_PPV_ARGS(&NativeFormatStrings::dataStatics));
        Q_ASSERT_SUCCEEDED(hr);
        hr = NativeFormatStrings::dataStatics->get_Text(&NativeFormatStrings::text);
        Q_ASSERT_SUCCEEDED(hr);
        hr = NativeFormatStrings::dataStatics->get_Html(&NativeFormatStrings::html);
        Q_ASSERT_SUCCEEDED(hr);
        hr = NativeFormatStrings::dataStatics->get_StorageItems(&NativeFormatStrings::storage);
        Q_ASSERT_SUCCEEDED(hr);
    }
}

QWinRTInternalMimeData::~QWinRTInternalMimeData()
{
}

bool QWinRTInternalMimeData::hasFormat_sys(const QString &mimetype) const
{
    qCDebug(lcQpaMime) << __FUNCTION__ << mimetype;

    if (!dataView)
        return false;

    return formats_sys().contains(mimetype);
}

QStringList QWinRTInternalMimeData::formats_sys() const
{
    qCDebug(lcQpaMime) << __FUNCTION__;

    if (!dataView)
        return QStringList();

    if (!formats.isEmpty())
        return formats;

    HRESULT hr;
    hr = QEventDispatcherWinRT::runOnXamlThread([this]() {
        boolean contains;
        HRESULT hr;
        hr = dataView->Contains(NativeFormatStrings::text, &contains);
        if (SUCCEEDED(hr) && contains)
            formats.append(QLatin1String("text/plain"));

        hr = dataView->Contains(NativeFormatStrings::html, &contains);
        if (SUCCEEDED(hr) && contains)
            formats.append(QLatin1String("text/html"));

        hr = dataView->Contains(NativeFormatStrings::storage, &contains);
        if (SUCCEEDED(hr) && contains)
            formats.append(QLatin1String("text/uri-list"));

        // We need to add any additional format as well, for legacy windows
        // reasons, but also in case someone adds custom formats.
        ComPtr<IVectorView<HSTRING>> availableFormats;
        hr = dataView->get_AvailableFormats(&availableFormats);
        RETURN_OK_IF_FAILED("Could not query available formats.");

        quint32 size;
        hr = availableFormats->get_Size(&size);
        RETURN_OK_IF_FAILED("Could not query format vector size.");
        for (quint32 i = 0; i < size; ++i) {
            HString str;
            hr = availableFormats->GetAt(i, str.GetAddressOf());
            if (FAILED(hr))
                continue;
            formats.append(hStringToQString(str));
        }
        return S_OK;
    });
    Q_ASSERT_SUCCEEDED(hr);

    return formats;
}

QVariant QWinRTInternalMimeData::retrieveData_sys(const QString &mimetype, QVariant::Type preferredType) const
{
    qCDebug(lcQpaMime) << __FUNCTION__ << mimetype << preferredType;

    if (!dataView || !formats.contains(mimetype))
        return QVariant();

    QVariant result;
    HRESULT hr;
    if (mimetype == QLatin1String("text/plain")) {
        hr = QEventDispatcherWinRT::runOnXamlThread([this, &result]() {
            HRESULT hr;
            ComPtr<IAsyncOperation<HSTRING>> op;
            HString res;
            hr = dataView->GetTextAsync(&op);
            Q_ASSERT_SUCCEEDED(hr);
            hr = QWinRTFunctions::await(op, res.GetAddressOf());
            Q_ASSERT_SUCCEEDED(hr);
            result.setValue(hStringToQString(res));
            return S_OK;
        });
    } else if (mimetype == QLatin1String("text/uri-list")) {
        hr = QEventDispatcherWinRT::runOnXamlThread([this, &result]() {
            HRESULT hr;
            ComPtr<IAsyncOperation<IVectorView<IStorageItem*>*>> op;
            hr = dataView->GetStorageItemsAsync(&op);
            Q_ASSERT_SUCCEEDED(hr);
            ComPtr<IVectorView<IStorageItem*>> nativeItems;
            hr = QWinRTFunctions::await(op, nativeItems.GetAddressOf());
            Q_ASSERT_SUCCEEDED(hr);
            QList<QVariant> items;
            quint32 count;
            hr = nativeItems->get_Size(&count);
            for (quint32 i = 0; i < count; ++i) {
                ComPtr<IStorageItem> item;
                hr = nativeItems->GetAt(i, &item);
                Q_ASSERT_SUCCEEDED(hr);
                HString path;
                hr = item->get_Path(path.GetAddressOf());
                Q_ASSERT_SUCCEEDED(hr);
                items.append(QUrl::fromLocalFile(hStringToQString(path)));
            }
            result.setValue(items);
            return S_OK;
        });
    } else if (mimetype == QLatin1String("text/html")) {
        hr = QEventDispatcherWinRT::runOnXamlThread([this, &result]() {
            HRESULT hr;
            ComPtr<IAsyncOperation<HSTRING>> op;
            HString res;
            hr = dataView->GetHtmlFormatAsync(&op);
            Q_ASSERT_SUCCEEDED(hr);
            hr = QWinRTFunctions::await(op, res.GetAddressOf());
            Q_ASSERT_SUCCEEDED(hr);
            result.setValue(hStringToQString(res));
            return S_OK;
        });
    } else {
        // Asking for custom data
        hr = QEventDispatcherWinRT::runOnXamlThread([this, &result, mimetype]() {
            HRESULT hr;
            ComPtr<IAsyncOperation<IInspectable*>> op;
            ComPtr<IInspectable> res;
            HString type;
            type.Set(reinterpret_cast<const wchar_t*>(mimetype.utf16()), mimetype.size());
            hr = dataView->GetDataAsync(type.Get(), &op);
            RETURN_OK_IF_FAILED("Could not query custom drag data.");
            hr = QWinRTFunctions::await(op, res.GetAddressOf());
            if (FAILED(hr) || !res) {
                qCDebug(lcQpaMime) << "Custom drop data operation returned no results or failed.";
                return S_OK;
            }

            // Test for properties
            ComPtr<IPropertyValue> propertyValue;
            hr = res.As(&propertyValue);
            if (SUCCEEDED(hr)) {
                // We need to check which type of custom data we are receiving
                PropertyType propertyType;
                propertyValue->get_Type(&propertyType);
                switch (propertyType) {
                case PropertyType_UInt8: {
                    quint8 v;
                    hr = propertyValue->GetUInt8(&v);
                    Q_ASSERT_SUCCEEDED(hr);
                    result.setValue(v);
                    return S_OK;
                }
                case PropertyType_Int16: {
                    qint16 v;
                    hr = propertyValue->GetInt16(&v);
                    Q_ASSERT_SUCCEEDED(hr);
                    result.setValue(v);
                    return S_OK;
                }
                case PropertyType_UInt16: {
                    quint16 v;
                    hr = propertyValue->GetUInt16(&v);
                    Q_ASSERT_SUCCEEDED(hr);
                    result.setValue(v);
                    return S_OK;
                }
                case PropertyType_Int32: {
                    qint32 v;
                    hr = propertyValue->GetInt32(&v);
                    Q_ASSERT_SUCCEEDED(hr);
                    result.setValue(v);
                    return S_OK;
                }
                case PropertyType_UInt32: {
                    quint32 v;
                    hr = propertyValue->GetUInt32(&v);
                    Q_ASSERT_SUCCEEDED(hr);
                    result.setValue(v);
                    return S_OK;
                }
                case PropertyType_Int64: {
                    qint64 v;
                    hr = propertyValue->GetInt64(&v);
                    Q_ASSERT_SUCCEEDED(hr);
                    result.setValue(v);
                    return S_OK;
                }
                case PropertyType_UInt64: {
                    quint64 v;
                    hr = propertyValue->GetUInt64(&v);
                    Q_ASSERT_SUCCEEDED(hr);
                    result.setValue(v);
                    return S_OK;
                }
                case PropertyType_Single: {
                    float v;
                    hr = propertyValue->GetSingle(&v);
                    Q_ASSERT_SUCCEEDED(hr);
                    result.setValue(v);
                    return S_OK;
                }
                case PropertyType_Double: {
                    double v;
                    hr = propertyValue->GetDouble(&v);
                    Q_ASSERT_SUCCEEDED(hr);
                    result.setValue(v);
                    return S_OK;
                }
                case PropertyType_Char16: {
                    wchar_t v;
                    hr = propertyValue->GetChar16(&v);
                    Q_ASSERT_SUCCEEDED(hr);
                    result.setValue(QString::fromWCharArray(&v, 1));
                    return S_OK;
                }
                case PropertyType_Boolean: {
                    boolean v;
                    hr = propertyValue->GetBoolean(&v);
                    Q_ASSERT_SUCCEEDED(hr);
                    result.setValue(v);
                    return S_OK;
                }
                case PropertyType_String: {
                    HString stringProperty;
                    hr = propertyValue->GetString(stringProperty.GetAddressOf());
                    Q_ASSERT_SUCCEEDED(hr);
                    result.setValue(hStringToQString(stringProperty));
                    return S_OK;
                }
                default:
                    qCDebug(lcQpaMime) << "Unknown property type dropped:" << propertyType;
                }
                return S_OK;
            }

            // Custom data can be read via input streams
            ComPtr<IRandomAccessStream> randomAccessStream;
            hr = res.As(&randomAccessStream);
            if (SUCCEEDED(hr)) {
                UINT64 size;
                hr = randomAccessStream->get_Size(&size);
                Q_ASSERT_SUCCEEDED(hr);
                ComPtr<IInputStream> stream;
                hr = randomAccessStream.As(&stream);
                Q_ASSERT_SUCCEEDED(hr);

                ComPtr<IBufferFactory> bufferFactory;
                hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_Streams_Buffer).Get(),
                                            IID_PPV_ARGS(&bufferFactory));
                Q_ASSERT_SUCCEEDED(hr);

                UINT32 length = qBound(quint64(0), quint64(size), quint64(UINT_MAX));
                ComPtr<IBuffer> buffer;
                hr = bufferFactory->Create(length, &buffer);
                Q_ASSERT_SUCCEEDED(hr);

                ComPtr<IAsyncOperationWithProgress<IBuffer *, UINT32>> readOp;
                hr = stream->ReadAsync(buffer.Get(), length, InputStreamOptions_None, &readOp);

                ComPtr<IBuffer> effectiveBuffer;
                hr = QWinRTFunctions::await(readOp, effectiveBuffer.GetAddressOf());

                hr = effectiveBuffer->get_Length(&length);

                ComPtr<Windows::Storage::Streams::IBufferByteAccess> byteArrayAccess;
                hr = effectiveBuffer.As(&byteArrayAccess);

                byte *bytes;
                hr = byteArrayAccess->Buffer(&bytes);
                QByteArray array((char *)bytes, length);
                result.setValue(array);
                return S_OK;
            }

            HSTRING runtimeClass;
            hr = res->GetRuntimeClassName(&runtimeClass);
            Q_ASSERT_SUCCEEDED(hr);
            HString converted;
            converted.Set(runtimeClass);
            qCDebug(lcQpaMime) << "Unknown drop data type received (" << hStringToQString(converted)
                               << "). Ignoring...";
            return S_OK;
        });
    }
    return result;
}

void QWinRTInternalMimeData::setDataView(const Microsoft::WRL::ComPtr<IDataPackageView> &d)
{
    dataView = d;
    formats.clear();
}

static HRESULT qt_drag_enter(IInspectable *sender, ABI::Windows::UI::Xaml::IDragEventArgs *e)
{
    QWinRTDrag::instance()->handleNativeDragEvent(sender, e);
    return S_OK;
}

static HRESULT qt_drag_over(IInspectable *sender, ABI::Windows::UI::Xaml::IDragEventArgs *e)
{
    QWinRTDrag::instance()->handleNativeDragEvent(sender, e);
    return S_OK;
}

static HRESULT qt_drag_leave(IInspectable *sender, ABI::Windows::UI::Xaml::IDragEventArgs *e)
{
    // Qt internally checks for new drags and auto sends leave events
    // Also there is no QPA function for handling leave
    Q_UNUSED(sender);
    Q_UNUSED(e);
    return S_OK;
}

static HRESULT qt_drop(IInspectable *sender, ABI::Windows::UI::Xaml::IDragEventArgs *e)
{
    QWinRTDrag::instance()->handleNativeDragEvent(sender, e, true);
    return S_OK;
}

#define Q_DECLARE_DRAGHANDLER(name,func) \
class QtDragEventHandler##name : public IDragEventHandler \
{ \
public: \
    virtual ~QtDragEventHandler##name() {\
    }\
    STDMETHODIMP Invoke(IInspectable *sender, \
                        ABI::Windows::UI::Xaml::IDragEventArgs *e) \
    { \
        return qt_##func(sender, e);\
    } \
 \
    STDMETHODIMP \
    QueryInterface(REFIID riid, void FAR* FAR* ppvObj) \
    { \
        if (riid == IID_IUnknown || riid == IID_IDragEventHandler) { \
            *ppvObj = this; \
            AddRef(); \
            return NOERROR; \
        } \
        *ppvObj = NULL; \
        return ResultFromScode(E_NOINTERFACE); \
    } \
 \
    STDMETHODIMP_(ULONG) \
    AddRef(void) \
    { \
        return ++m_refs; \
    } \
 \
    STDMETHODIMP_(ULONG) \
    Release(void) \
    { \
        if (--m_refs == 0) { \
            delete this; \
            return 0; \
        } \
        return m_refs; \
    } \
private: \
ULONG m_refs{0}; \
};

Q_DECLARE_DRAGHANDLER(Enter, drag_enter)
Q_DECLARE_DRAGHANDLER(Over, drag_over)
Q_DECLARE_DRAGHANDLER(Leave, drag_leave)
Q_DECLARE_DRAGHANDLER(Drop, drop)

#define Q_INST_DRAGHANDLER(name) QtDragEventHandler##name()

Q_GLOBAL_STATIC(QWinRTDrag, gDrag);

extern ComPtr<ABI::Windows::UI::Input::IPointerPoint> qt_winrt_lastPointerPoint; // qwinrtscreen.cpp

QWinRTDrag::QWinRTDrag()
    : QPlatformDrag()
    , m_dragTarget(0)
{
    qCDebug(lcQpaMime) << __FUNCTION__;
    m_enter = new Q_INST_DRAGHANDLER(Enter);
    m_over = new Q_INST_DRAGHANDLER(Over);
    m_leave = new Q_INST_DRAGHANDLER(Leave);
    m_drop = new Q_INST_DRAGHANDLER(Drop);
    m_mimeData = new QWinRTInternalMimeData;
}

QWinRTDrag::~QWinRTDrag()
{
    qCDebug(lcQpaMime) << __FUNCTION__;
    delete m_enter;
    delete m_over;
    delete m_leave;
    delete m_drop;
    delete m_mimeData;
}

QWinRTDrag *QWinRTDrag::instance()
{
    return gDrag;
}

inline HRESULT resetUiElementDrag(ComPtr<IUIElement3> &elem3, EventRegistrationToken startingToken)
{
    return QEventDispatcherWinRT::runOnXamlThread([elem3, startingToken]() {
        HRESULT hr;
        hr = elem3->put_CanDrag(false);
        Q_ASSERT_SUCCEEDED(hr);
        hr = elem3->remove_DragStarting(startingToken);
        Q_ASSERT_SUCCEEDED(hr);
        return S_OK;
    });
}

Qt::DropAction QWinRTDrag::drag(QDrag *drag)
{
    qCDebug(lcQpaMime) << __FUNCTION__ << drag;

    if (!qt_winrt_lastPointerPoint) {
        Q_ASSERT_X(qt_winrt_lastPointerPoint, Q_FUNC_INFO, "No pointerpoint known");
        return Qt::IgnoreAction;
    }

    ComPtr<IUIElement3> elem3;
    HRESULT hr = m_ui.As(&elem3);
    Q_ASSERT_SUCCEEDED(hr);

    ComPtr<IAsyncOperation<ABI::Windows::ApplicationModel::DataTransfer::DataPackageOperation>> op;
    EventRegistrationToken startingToken;

    hr = QEventDispatcherWinRT::runOnXamlThread([drag, &op, &hr, elem3, &startingToken, this]() {

        hr = elem3->put_CanDrag(true);
        Q_ASSERT_SUCCEEDED(hr);

        auto startingCallback = Callback<ITypedEventHandler<UIElement*, DragStartingEventArgs*>> ([drag](IInspectable *, IDragStartingEventArgs *args) {
            qCDebug(lcQpaMime) << "Drag starting" << args;

            ComPtr<IDataPackage> dataPackage;
            HRESULT hr;
            hr = args->get_Data(dataPackage.GetAddressOf());
            Q_ASSERT_SUCCEEDED(hr);
            Qt::DropAction action = drag->defaultAction();
            hr = dataPackage->put_RequestedOperation(translateFromQDragDropActions(action));
            Q_ASSERT_SUCCEEDED(hr);

#ifndef QT_WINRT_LIMITED_DRAGANDDROP
            ComPtr<IDragStartingEventArgs2> args2;
            hr = args->QueryInterface(IID_PPV_ARGS(&args2));
            Q_ASSERT_SUCCEEDED(hr);

            Qt::DropActions actions = drag->supportedActions();
            DataPackageOperation allowedOperations = DataPackageOperation_None;
            if (actions & Qt::CopyAction)
                allowedOperations |= DataPackageOperation_Copy;
            if (actions & Qt::MoveAction)
                allowedOperations |= DataPackageOperation_Move;
            if (actions & Qt::LinkAction)
                allowedOperations |= DataPackageOperation_Link;
            hr = args2->put_AllowedOperations(allowedOperations);
            Q_ASSERT_SUCCEEDED(hr);
#endif // QT_WINRT_LIMITED_DRAGANDDROP
            QMimeData *mimeData = drag->mimeData();
            if (mimeData->hasText()) {
                hr = dataPackage->SetText(qStringToHString(mimeData->text()).Get());
                Q_ASSERT_SUCCEEDED(hr);
            }
            if (mimeData->hasHtml()) {
                hr = dataPackage->SetHtmlFormat(qStringToHString(mimeData->html()).Get());
                Q_ASSERT_SUCCEEDED(hr);
            }
            // ### TODO: Missing: weblink, image

            if (!drag->pixmap().isNull()) {
                const QImage image2 = drag->pixmap().toImage();
                const QImage image = image2.convertToFormat(QImage::Format_ARGB32);
                if (!image.isNull()) {
                    // Create IBuffer containing image
                    ComPtr<IBuffer> imageBuffer = createIBufferFromData(reinterpret_cast<const char*>(image.bits()), image.byteCount());

                    ComPtr<ISoftwareBitmapFactory> bitmapFactory;
                    hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Graphics_Imaging_SoftwareBitmap).Get(),
                                                IID_PPV_ARGS(&bitmapFactory));
                    Q_ASSERT_SUCCEEDED(hr);

                    ComPtr<ISoftwareBitmap> bitmap;
                    hr = bitmapFactory->Create(BitmapPixelFormat_Rgba8, image.width(), image.height(), &bitmap);
                    Q_ASSERT_SUCCEEDED(hr);

                    hr = bitmap->CopyFromBuffer(imageBuffer.Get());
                    Q_ASSERT_SUCCEEDED(hr);

                    ComPtr<IDragUI> dragUi;
                    hr = args->get_DragUI(dragUi.GetAddressOf());
                    Q_ASSERT_SUCCEEDED(hr);

                    hr = dragUi->SetContentFromSoftwareBitmap(bitmap.Get());
                    Q_ASSERT_SUCCEEDED(hr);
                }
            }

            const QStringList formats = mimeData->formats();
            for (auto item : formats) {
                QByteArray data = mimeData->data(item);

                ComPtr<IBuffer> buffer = createIBufferFromData(data.constData(), data.size());

                // We cannot push the buffer to the data package as the result on
                // recipient side is different from native events. It still sends a
                // buffer, but that potentially cannot be parsed. Hence we need to create
                // a IRandomAccessStream which gets forwarded as is to the drop side.
                ComPtr<IRandomAccessStream> ras;
                hr = RoActivateInstance(HString::MakeReference(RuntimeClass_Windows_Storage_Streams_InMemoryRandomAccessStream).Get(), &ras);
                Q_ASSERT_SUCCEEDED(hr);

                hr = ras->put_Size(data.size());
                ComPtr<IOutputStream> outputStream;
                hr = ras->GetOutputStreamAt(0, &outputStream);
                Q_ASSERT_SUCCEEDED(hr);

                ComPtr<IAsyncOperationWithProgress<UINT32,UINT32>> writeOp;
                hr = outputStream->WriteAsync(buffer.Get(), &writeOp);
                Q_ASSERT_SUCCEEDED(hr);

                UINT32 result;
                hr = QWinRTFunctions::await(writeOp, &result);
                Q_ASSERT_SUCCEEDED(hr);

                unsigned char flushResult;
                ComPtr<IAsyncOperation<bool>> flushOp;
                hr = outputStream->FlushAsync(&flushOp);
                Q_ASSERT_SUCCEEDED(hr);

                hr = QWinRTFunctions::await(flushOp, &flushResult);
                Q_ASSERT_SUCCEEDED(hr);

                hr = dataPackage->SetData(qStringToHString(item).Get(), ras.Get());
                Q_ASSERT_SUCCEEDED(hr);
            }
            return S_OK;
        });

        hr = elem3->add_DragStarting(startingCallback.Get(), &startingToken);
        Q_ASSERT_SUCCEEDED(hr);

        hr = elem3->StartDragAsync(qt_winrt_lastPointerPoint.Get(), &op);
        Q_ASSERT_SUCCEEDED(hr);

        return hr;
    });
    if (!op || FAILED(hr)) {
        qCDebug(lcQpaMime) << "Drag failed:" << hr;
        hr = resetUiElementDrag(elem3, startingToken);
        Q_ASSERT_SUCCEEDED(hr);
        return Qt::IgnoreAction;
    }

    DataPackageOperation nativeOperationType;
    // Do not yield, as that can cause deadlocks when dropping inside the same app
    hr = QWinRTFunctions::await(op, &nativeOperationType, QWinRTFunctions::ProcessThreadEvents);
    Q_ASSERT_SUCCEEDED(hr);

    hr = resetUiElementDrag(elem3, startingToken);
    Q_ASSERT_SUCCEEDED(hr);

    Qt::DropAction resultAction;
    switch (nativeOperationType) {
    case DataPackageOperation_Link:
        resultAction = Qt::LinkAction;
        break;
    case DataPackageOperation_Copy:
        resultAction = Qt::CopyAction;
        break;
    case DataPackageOperation_Move:
        resultAction = Qt::MoveAction;
        break;
    case DataPackageOperation_None:
    default:
        resultAction = Qt::IgnoreAction;
        break;
    }

    return resultAction;
}

void QWinRTDrag::setDropTarget(QWindow *target)
{
    qCDebug(lcQpaMime) << __FUNCTION__ << target;
    m_dragTarget = target;
}

void QWinRTDrag::setUiElement(ComPtr<ABI::Windows::UI::Xaml::IUIElement> &element)
{
    qCDebug(lcQpaMime) << __FUNCTION__;
    m_ui = element;
    // We set the element to always accept drops and then evaluate during
    // runtime
    HRESULT hr = element->put_AllowDrop(TRUE);
    EventRegistrationToken tok;
    hr = element->add_DragEnter(m_enter, &tok);
    RETURN_VOID_IF_FAILED("Failed to add DragEnter handler.");
    hr = element->add_DragOver(m_over, &tok);
    RETURN_VOID_IF_FAILED("Failed to add DragOver handler.");
    hr = element->add_DragLeave(m_leave, &tok);
    RETURN_VOID_IF_FAILED("Failed to add DragLeave handler.");
    hr = element->add_Drop(m_drop, &tok);
    RETURN_VOID_IF_FAILED("Failed to add Drop handler.");
}

void QWinRTDrag::handleNativeDragEvent(IInspectable *sender, ABI::Windows::UI::Xaml::IDragEventArgs *e, bool drop)
{
    Q_UNUSED(sender);

    if (!m_dragTarget)
        return;

    HRESULT hr;
    Point relativePoint;
    hr = e->GetPosition(m_ui.Get(), &relativePoint);
    RETURN_VOID_IF_FAILED("Could not query drag position.");
    const QPoint p(relativePoint.X, relativePoint.Y);

    ComPtr<IDragEventArgs2> e2;
    hr = e->QueryInterface(IID_PPV_ARGS(&e2));
    RETURN_VOID_IF_FAILED("Could not convert drag event args");

    DragDropModifiers modifiers;
    hr = e2->get_Modifiers(&modifiers);

#ifndef QT_WINRT_LIMITED_DRAGANDDROP
    ComPtr<IDragEventArgs3> e3;
    hr = e->QueryInterface(IID_PPV_ARGS(&e3));
    Q_ASSERT_SUCCEEDED(hr);

    DataPackageOperation dataOp;
    hr = e3->get_AllowedOperations(&dataOp);
    if (FAILED(hr))
        qCDebug(lcQpaMime) << __FUNCTION__ << "Could not query drag operations";

    const Qt::DropActions actions = translateToQDragDropActions(dataOp);
#else // !QT_WINRT_LIMITED_DRAGANDDROP
    const Qt::DropActions actions = Qt::LinkAction | Qt::CopyAction | Qt::MoveAction;;
#endif // !QT_WINRT_LIMITED_DRAGANDDROP

    ComPtr<IDataPackageView> dataView;
    hr = e2->get_DataView(&dataView);
    Q_ASSERT_SUCCEEDED(hr);

    m_mimeData->setDataView(dataView);

    // We use deferral as we need to jump to the Qt thread to handle
    // the drag event
    ComPtr<IDragOperationDeferral> deferral;
    hr = e2->GetDeferral(&deferral);
    Q_ASSERT_SUCCEEDED(hr);

    DragThreadTransferData *transferData = new DragThreadTransferData;
    transferData->moveToThread(qGuiApp->thread());
    transferData->window = m_dragTarget;
    transferData->point = p;
    transferData->mime = m_mimeData;
    transferData->actions = actions;
    transferData->dropAction = drop;
    transferData->nativeArgs = e;
    transferData->deferral = deferral;
    QMetaObject::invokeMethod(transferData, "handleDrag", Qt::QueuedConnection);
}

DragThreadTransferData::DragThreadTransferData(QObject *parent)
    : QObject(parent)
    , dropAction(false)
{
}

void DragThreadTransferData::handleDrag()
{
    bool accepted = false;
    Qt::DropAction acceptedAction;
    if (dropAction) {
        QPlatformDropQtResponse response = QWindowSystemInterface::handleDrop(window, mime, point, actions);
        accepted = response.isAccepted();
        acceptedAction = response.acceptedAction();
    } else {
        QPlatformDragQtResponse response = QWindowSystemInterface::handleDrag(window, mime, point, actions);
        accepted = response.isAccepted();
        acceptedAction = response.acceptedAction();
    }

    HRESULT hr;
    hr = QEventDispatcherWinRT::runOnXamlThread([accepted, acceptedAction, this]() {
        HRESULT hr;
        hr = nativeArgs->put_Handled(accepted);
        if (acceptedAction != Qt::IgnoreAction) {
            ComPtr<IDragEventArgs2> e2;
            hr = nativeArgs.As(&e2);
            if (SUCCEEDED(hr))
                hr = e2->put_AcceptedOperation(translateFromQDragDropActions(acceptedAction));
        }
        deferral->Complete();
        return S_OK;
    });
    Q_ASSERT_SUCCEEDED(hr);
    deleteLater();
}

QT_END_NAMESPACE

#include "qwinrtdrag.moc"
