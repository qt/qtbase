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
class Q_AUTOTEST_EXPORT Type
{
public:
    class Accept {
    public:
        class MimeType {
        public:
            class Extension {
            public:
                static std::optional<Extension> fromQt(QStringView extension);

                ~Extension();

                const QStringView &value() const { return m_value; }

            private:
                explicit Extension(QStringView extension);

                QStringView m_value;
            };

            MimeType();
            ~MimeType();

            void addExtension(Extension type);

            const std::vector<Extension> &extensions() const { return m_extensions; }

        private:
            std::vector<Extension> m_extensions;
        };

        static std::optional<Accept> fromQt(QStringView type);

        ~Accept();

        void setMimeType(MimeType mimeType);

        const MimeType &mimeType() const { return m_mimeType; }

    private:
        Accept();
        MimeType m_mimeType;
    };

    Type(QStringView description, std::optional<Accept> accept);
    ~Type();

    static std::optional<Type> fromQt(QStringView type);
    const QStringView &description() const { return m_description; }
    const std::optional<Accept> &accept() const { return m_accept; }

private:
    QStringView m_description;
    std::optional<Accept> m_accept;
};

Q_AUTOTEST_EXPORT emscripten::val makeOpenFileOptions(const QStringList &filterList,
                                                      bool acceptMultiple);
Q_AUTOTEST_EXPORT emscripten::val makeSaveFileOptions(const QStringList &filterList,
                                                      const std::string &suggestedName);

Q_AUTOTEST_EXPORT std::string makeFileInputAccept(const QStringList &filterList);

}  // namespace LocalFileApi
QT_END_NAMESPACE

#endif // QLOCALFILEAPI_P_H
