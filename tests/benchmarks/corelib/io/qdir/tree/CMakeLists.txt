# Generated from tree.pro.

#####################################################################
## bench_qdir_tree Binary:
#####################################################################

qt_internal_add_benchmark(bench_qdir_tree
    SOURCES
        bench_qdir_tree.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

# Resources:
set(bench_qdir_tree_resource_files
    "4.6.0-list.txt"
)

qt_internal_add_resource(bench_qdir_tree "bench_qdir_tree"
    PREFIX
        "/"
    FILES
        ${bench_qdir_tree_resource_files}
)


#### Keys ignored in scope 1:.:.:tree.pro:<TRUE>:
# TEMPLATE = "app"
