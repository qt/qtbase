#=============================================================================
# Copyright 2005-2011 Kitware, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of Kitware, Inc. nor the names of its
#   contributors may be used to endorse or promote products derived
#   from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

######################################
#
#       Macros for building Qt files
#
######################################

include(MacroAddFileDependencies)

MACRO (QT5_EXTRACT_OPTIONS _qt5_files _qt5_options)
  SET(${_qt5_files})
  SET(${_qt5_options})
  SET(_QT5_DOING_OPTIONS FALSE)
  FOREACH(_currentArg ${ARGN})
    IF ("${_currentArg}" STREQUAL "OPTIONS")
      SET(_QT5_DOING_OPTIONS TRUE)
    ELSE ("${_currentArg}" STREQUAL "OPTIONS")
      IF(_QT5_DOING_OPTIONS)
        LIST(APPEND ${_qt5_options} "${_currentArg}")
      ELSE(_QT5_DOING_OPTIONS)
        LIST(APPEND ${_qt5_files} "${_currentArg}")
      ENDIF(_QT5_DOING_OPTIONS)
    ENDIF ("${_currentArg}" STREQUAL "OPTIONS")
  ENDFOREACH(_currentArg)
ENDMACRO (QT5_EXTRACT_OPTIONS)


# QT5_WRAP_UI(outfiles inputfile ... )

MACRO (QT5_WRAP_UI outfiles )
  QT5_EXTRACT_OPTIONS(ui_files ui_options ${ARGN})

  FOREACH (it ${ui_files})
    GET_FILENAME_COMPONENT(outfile ${it} NAME_WE)
    GET_FILENAME_COMPONENT(infile ${it} ABSOLUTE)
    SET(outfile ${CMAKE_CURRENT_BINARY_DIR}/ui_${outfile}.h)
    ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
      COMMAND ${QT_UIC_EXECUTABLE}
      ARGS ${ui_options} -o ${outfile} ${infile}
      MAIN_DEPENDENCY ${infile})
    SET(${outfiles} ${${outfiles}} ${outfile})
  ENDFOREACH (it)

ENDMACRO (QT5_WRAP_UI)
