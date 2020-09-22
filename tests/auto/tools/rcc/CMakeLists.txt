# Generated from rcc.pro.

#####################################################################
## tst_rcc Test:
#####################################################################

qt_internal_add_test(tst_rcc
    SOURCES
        tst_rcc.cpp
)

# Resources:
set(images_resource_files
    "images/circle.png"
    "images/square.png"
    "images/subdir/triangle.png"
)

qt_internal_add_resource(tst_rcc "images"
    PREFIX
        "/"
    BASE
        "data/images"
    FILES
        ${images_resource_files}
)
set(size-0_resource_files
    "data/data-0.txt"
)

qt_internal_add_resource(tst_rcc "size-0"
    PREFIX
        "/"
    BASE
        "data/sizes"
    FILES
        ${size-0_resource_files}
)
set(size-2-0-35-1_resource_files
    "data/data-0.txt"
    "data/data-1.txt"
    "data/data-2.txt"
    "data/data-35.txt"
)

qt_internal_add_resource(tst_rcc "size-2-0-35-1"
    PREFIX
        "/"
    BASE
        "data/sizes"
    FILES
        ${size-2-0-35-1_resource_files}
)
set(size-1_resource_files
    "data/data-1.txt"
)

qt_internal_add_resource(tst_rcc "size-1"
    PREFIX
        "/"
    BASE
        "data/sizes"
    FILES
        ${size-1_resource_files}
)

