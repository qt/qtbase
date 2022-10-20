/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QVULKANINSTANCE_H
#define QVULKANINSTANCE_H

#include <QtGui/qtguiglobal.h>

#if 0
#pragma qt_no_master_include
#pragma qt_sync_skip_header_check
#endif

#if QT_CONFIG(vulkan) || defined(Q_CLANG_QDOC)

#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif
#if !defined(Q_CLANG_QDOC) && __has_include(<vulkan/vulkan.h>)
#include <vulkan/vulkan.h>
#else
// QT_CONFIG(vulkan) implies vulkan.h being available at Qt build time, but it
// does not guarantee vulkan.h is available at *application* build time. Both
// for qdoc and for apps built on systems without Vulkan SDK we provide a set
// of typedefs to keep things compiling since this header may be included from
// Qt Quick and elsewhere just to get types like VkImage and friends defined.

typedef void* PFN_vkVoidFunction;
// non-dispatchable handles (64-bit regardless of arch)
typedef quint64 VkSurfaceKHR;
typedef quint64 VkImage;
typedef quint64 VkImageView;
// dispatchable handles (32 or 64-bit depending on arch)
typedef void* VkInstance;
typedef void* VkPhysicalDevice;
typedef void* VkDevice;
// enums
typedef int VkResult;
typedef int VkImageLayout;
typedef int VkDebugReportFlagsEXT;
typedef int VkDebugReportObjectTypeEXT;
#endif

// QVulkanInstance itself is only applicable if vulkan.h is available, or if
// it's qdoc. An application that is built on a vulkan.h-less system against a
// Vulkan-enabled Qt gets the dummy typedefs but not QVulkan*.
#if __has_include(<vulkan/vulkan.h>) || defined(Q_CLANG_QDOC)

#include <QtCore/qbytearraylist.h>
#include <QtCore/qdebug.h>
#include <QtCore/qhashfunctions.h>
#include <QtCore/qlist.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qversionnumber.h>

QT_BEGIN_NAMESPACE

class QVulkanInstancePrivate;
class QPlatformVulkanInstance;
class QVulkanFunctions;
class QVulkanDeviceFunctions;
class QWindow;

struct QVulkanLayer
{
    QByteArray name;
    uint32_t version;
    QVersionNumber specVersion;
    QByteArray description;
};
Q_DECLARE_TYPEINFO(QVulkanLayer, Q_RELOCATABLE_TYPE);

inline bool operator==(const QVulkanLayer &lhs, const QVulkanLayer &rhs) noexcept
{
    return lhs.name == rhs.name && lhs.version == rhs.version && lhs.specVersion == rhs.specVersion;
}
inline bool operator!=(const QVulkanLayer &lhs, const QVulkanLayer &rhs) noexcept
{ return !(lhs == rhs); }

inline size_t qHash(const QVulkanLayer &key, size_t seed = 0) noexcept
{
    QtPrivate::QHashCombine hash;
    seed = hash(seed, key.name);
    seed = hash(seed, key.version);
    seed = hash(seed, key.specVersion);
    return seed;
}

struct QVulkanExtension
{
    QByteArray name;
    uint32_t version;
};
Q_DECLARE_TYPEINFO(QVulkanExtension, Q_RELOCATABLE_TYPE);

inline bool operator==(const QVulkanExtension &lhs, const QVulkanExtension &rhs) noexcept
{
    return lhs.name == rhs.name && lhs.version == rhs.version;
}
inline bool operator!=(const QVulkanExtension &lhs, const QVulkanExtension &rhs) noexcept
{ return !(lhs == rhs); }

inline size_t qHash(const QVulkanExtension &key, size_t seed = 0) noexcept
{
    QtPrivate::QHashCombine hash;
    seed = hash(seed, key.name);
    seed = hash(seed, key.version);
    return seed;
}

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QVulkanLayer &);
Q_GUI_EXPORT QDebug operator<<(QDebug, const QVulkanExtension &);
#endif

template<typename T>
class QVulkanInfoVector : public QList<T>
{
public:
    bool contains(const QByteArray &name) const {
        return std::any_of(this->cbegin(), this->cend(), [&](const T &entry) {
            return entry.name == name; });
    }
    bool contains(const QByteArray &name, int minVersion) const {
        return std::any_of(this->cbegin(), this->cend(), [&](const T &entry) {
            return entry.name == name && entry.version >= minVersion; });
    }
};

class Q_GUI_EXPORT QVulkanInstance
{
public:
    QVulkanInstance();
    ~QVulkanInstance();

    enum Flag {
        NoDebugOutputRedirect = 0x01
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    // ### Qt 7: remove non-const overloads
    QVulkanInfoVector<QVulkanLayer> supportedLayers();
    inline QVulkanInfoVector<QVulkanLayer> supportedLayers() const
    { return const_cast<QVulkanInstance*>(this)->supportedLayers(); }
    QVulkanInfoVector<QVulkanExtension> supportedExtensions();
    inline QVulkanInfoVector<QVulkanExtension> supportedExtensions() const
    { return const_cast<QVulkanInstance*>(this)->supportedExtensions(); }
    QVersionNumber supportedApiVersion() const;

    void setVkInstance(VkInstance existingVkInstance);

    void setFlags(Flags flags);
    void setLayers(const QByteArrayList &layers);
    void setExtensions(const QByteArrayList &extensions);
    void setApiVersion(const QVersionNumber &vulkanVersion);

    bool create();
    void destroy();
    bool isValid() const;
    VkResult errorCode() const;

    VkInstance vkInstance() const;

    Flags flags() const;
    QByteArrayList layers() const;
    QByteArrayList extensions() const;
    QVersionNumber apiVersion() const;

    PFN_vkVoidFunction getInstanceProcAddr(const char *name);

    QPlatformVulkanInstance *handle() const;

    QVulkanFunctions *functions() const;
    QVulkanDeviceFunctions *deviceFunctions(VkDevice device);
    void resetDeviceFunctions(VkDevice device);

    static VkSurfaceKHR surfaceForWindow(QWindow *window);

    bool supportsPresent(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, QWindow *window);

    void presentAboutToBeQueued(QWindow *window);
    void presentQueued(QWindow *window);

    typedef bool (*DebugFilter)(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object,
                                size_t location, int32_t messageCode, const char *pLayerPrefix, const char *pMessage);
    void installDebugOutputFilter(DebugFilter filter);
    void removeDebugOutputFilter(DebugFilter filter);

private:
    QScopedPointer<QVulkanInstancePrivate> d_ptr;
    Q_DISABLE_COPY(QVulkanInstance)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QVulkanInstance::Flags)

QT_END_NAMESPACE

#endif // __has_include(<vulkan/vulkan.h>) || defined(Q_CLANG_QDOC)

#endif // QT_CONFIG(vulkan) || defined(Q_CLANG_QDOC)

#endif // QVULKANINSTANCE_H
