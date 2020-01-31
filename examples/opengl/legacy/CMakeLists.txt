# Generated from legacy.pro.

if(NOT TARGET Qt::OpenGL)
    return()
endif()
if(NOT QT_FEATURE_dynamicgl AND NOT QT_FEATURE_opengles2)
    add_subdirectory(grabber)
    add_subdirectory(hellogl)
    add_subdirectory(overpainting)
    add_subdirectory(pbuffers)
    add_subdirectory(framebufferobject2)
    add_subdirectory(samplebuffers)
endif()
