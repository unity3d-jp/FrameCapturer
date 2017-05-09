set(LIBRARY_PATHS
    /usr/lib
    /usr/local/lib
    ${VPX_DIR}/lib
)

find_path(VPX_INCLUDE_DIR
    vpx/vpx_codec.h
    PATHS ${VPX_DIR}/include
)

find_library(VPX_LIBRARY
    NAMES vpx
    PATHS ${LIBRARY_PATHS}
)

mark_as_advanced(VPX_INCLUDE_DIR)
mark_as_advanced(VPX_LIBRARY)

find_package_handle_standard_args("VPX"
    DEFAULT_MSG
    VPX_LIBRARY
    VPX_INCLUDE_DIR
)
