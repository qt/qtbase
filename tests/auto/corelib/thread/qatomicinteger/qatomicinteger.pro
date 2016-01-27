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
    no-cxx11/char \
    no-cxx11/char16_t \
    no-cxx11/char32_t \
    no-cxx11/int \
    no-cxx11/long \
    no-cxx11/qlonglong \
    no-cxx11/qptrdiff \
    no-cxx11/quintptr \
    no-cxx11/qulonglong \
    no-cxx11/schar \
    no-cxx11/short \
    no-cxx11/uchar \
    no-cxx11/uint \
    no-cxx11/ulong \
    no-cxx11/ushort \
    no-cxx11/wchar_t \


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
