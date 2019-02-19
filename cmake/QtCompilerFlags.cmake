# Set warnings. All compilers except MSVC support -Wall -Wextra
if (MSVC)
    add_compile_options(/W3)
else()
    add_compile_options(-Wall -Wextra)
endif()
