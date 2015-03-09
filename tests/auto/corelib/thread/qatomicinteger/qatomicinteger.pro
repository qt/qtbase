TEMPLATE=subdirs
SUBDIRS=\
    char \
    char16_t \
    char32_t \
    int \
    long \
    qlonglong \
    qptrdiff \
    quintptr \
    qulonglong \
    schar \
    short \
    uchar \
    uint \
    ulong \
    ushort \
    wchar_t \


contains(QT_CONFIG, c++11)|msvc: SUBDIRS +=\
    cxx11/char \
    cxx11/char16_t \
    cxx11/char32_t \
    cxx11/int \
    cxx11/long \
    cxx11/qlonglong \
    cxx11/qptrdiff \
    cxx11/quintptr \
    cxx11/qulonglong \
    cxx11/schar \
    cxx11/short \
    cxx11/uchar \
    cxx11/uint \
    cxx11/ulong \
    cxx11/ushort \
    cxx11/wchar_t \


# The GCC-style atomics only support 32-bit and pointer-sized but add
# them all anyway so we ensure the macros are properly defined
gcc: SUBDIRS +=\
    gcc/char \
    gcc/char16_t \
    gcc/char32_t \
    gcc/int \
    gcc/long \
    gcc/qlonglong \
    gcc/qptrdiff \
    gcc/quintptr \
    gcc/qulonglong \
    gcc/schar \
    gcc/short \
    gcc/uchar \
    gcc/uint \
    gcc/ulong \
    gcc/ushort \
    gcc/wchar_t \
