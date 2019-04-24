
# Logic derived from mkspecs/features/unix/opengl.prf: prefer GLESv2 over GL

if(TARGET WrapOpenGL)
    set(WrapOpenGL_FOUND ON)
    return()
endif()

add_library(WrapOpenGL INTERFACE IMPORTED)

if(QT_FEATURE_opengles2)
    find_package(GLESv2)
    target_link_libraries(WrapOpenGL INTERFACE GLESv2::GLESv2)
else()
    find_package(OpenGL)
    target_link_libraries(WrapOpenGL INTERFACE OpenGL::GL)
endif()
set(WrapOpenGL_FOUND ON)

set_property(TARGET WrapOpenGL PROPERTY IMPORTED_GLOBAL TRUE)
