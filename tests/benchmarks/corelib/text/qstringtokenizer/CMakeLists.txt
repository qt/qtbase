#####################################################################
## tst_bench_qstringtokenizer Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qstringtokenizer
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

qt_add_resources(tst_bench_qstringtokenizer "qstringtokenizer_bench_data"
    PREFIX
        "/"
    FILES
        "./data/lorem.txt"
)
