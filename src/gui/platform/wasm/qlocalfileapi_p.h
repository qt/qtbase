// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QLOCALFILEAPI_P_H
#define QLOCALFILEAPI_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qglobal_p.h>
#include <qstringview.h>
#include <emscripten/val.h>
#include <cstdint>
#include <functional>

QT_BEGIN_NAMESPACE

namespace LocalFileApi {
class Q_CORE_EXPORT Type {
public:
    class Accept {
    public:
        class MimeType {
        public:
            class Extension {
            public:
                static std::optional<Extension> fromQt(QStringView extension);

                ~Extension();

                emscripten::val asVal() const;

            private:
                explicit Extension(QStringView extension);

                emscripten::val m_storage;
            };

            MimeType();
            ~MimeType();

            void addExtension(Extension type);

            emscripten::val asVal() const;

        private:
            emscripten::val m_storage;
        };

        static std::optional<Accept> fromQt(QStringView type);

        ~Accept();

        void addMimeType(MimeType mimeType);

        emscripten::val asVal() const;

    private:
        Accept();
        emscripten::val m_storage;
    };

    Type(QStringView description, std::optional<Accept> accept);
    ~Type();

    static std::optional<Type> fromQt(QStringView type);
    emscripten::val asVal() const;

private:
    emscripten::val m_storage;
};

Q_CORE_EXPORT emscripten::val makeOpenFileOptions(const QStringList &filterList, bool acceptMultiple);
Q_CORE_EXPORT emscripten::val makeSaveFileOptions(const QStringList &filterList, const std::string& suggestedName);

}  // namespace LocalFileApi
QT_END_NAMESPACE

#endif // QLOCALFILEAPI_P_H
