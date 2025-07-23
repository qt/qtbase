# Record the sbom option values before they might be modified by an sbom_setup call, due to
# missing python dependencies.
set(original_QT_GENERATE_SBOM "${QT_GENERATE_SBOM}")
set(original_QT_SBOM_GENERATE_SPDX_V2 "${QT_SBOM_GENERATE_SPDX_V2}")
set(original_QT_SBOM_GENERATE_CYDX_V1_6 "${QT_SBOM_GENERATE_CYDX_V1_6}")

# Explicitly set these because none case only has QT_GENERATE_SBOM passed as OFF.
# In this case, the defaults for the formats is to remain ON.
if(NOT QT_GENERATE_SBOM)
    set(original_QT_SBOM_GENERATE_SPDX_V2 ON)
    set(original_QT_SBOM_GENERATE_CYDX_V1_6 ON)
endif()
