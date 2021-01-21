/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include <qpa/qplatformdrag.h>

#include <QtCore/QLoggingCategory>
#include <QtCore/QMimeData>
#include <QtGui/private/qdnd_p.h>
#include <QtGui/private/qinternalmimedata_p.h>

#include <wrl.h>

namespace ABI {
    namespace Windows {
        namespace ApplicationModel {
            namespace DataTransfer {
                struct IDataPackageView;
            }
        }
        namespace UI {
            namespace Xaml {
                struct IUIElement;
                struct IDragEventArgs;
                struct IDragOperationDeferral;
                //struct IDataPackageView;
            }
        }
    }
}

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaMime)

class QtDragEventHandlerEnter;
class QtDragEventHandlerOver;
class QtDragEventHandlerLeave;
class QtDragEventHandlerDrop;
class QWinRTInternalMimeData;

class QWinRTInternalMimeData : public QInternalMimeData {
public:
    QWinRTInternalMimeData();
    ~QWinRTInternalMimeData() override = default;

    bool hasFormat_sys(const QString &mimetype) const override;
    QStringList formats_sys() const override;
    QVariant retrieveData_sys(const QString &mimetype, QVariant::Type preferredType) const override;

    void setDataView(const Microsoft::WRL::ComPtr<ABI::Windows::ApplicationModel::DataTransfer::IDataPackageView> &d);
private:
    Microsoft::WRL::ComPtr<ABI::Windows::ApplicationModel::DataTransfer::IDataPackageView> dataView;
    mutable QStringList formats;
};

class QWinRTDrag : public QPlatformDrag {
public:
    QWinRTDrag();
    ~QWinRTDrag() override;
    static QWinRTDrag *instance();

    Qt::DropAction drag(QDrag *) override;

    void setDropTarget(QWindow *target);

    // Native integration and registration
    void setUiElement(Microsoft::WRL::ComPtr<ABI::Windows::UI::Xaml::IUIElement> &element);

    void handleNativeDragEvent(IInspectable *sender, ABI::Windows::UI::Xaml::IDragEventArgs *e, bool drop = false);
private:
    Microsoft::WRL::ComPtr<ABI::Windows::UI::Xaml::IUIElement> m_ui;
    QWindow *m_dragTarget;
    QtDragEventHandlerEnter *m_enter;
    QtDragEventHandlerOver *m_over;
    QtDragEventHandlerLeave *m_leave;
    QtDragEventHandlerDrop *m_drop;
    QWinRTInternalMimeData *m_mimeData;
};

QT_END_NAMESPACE
