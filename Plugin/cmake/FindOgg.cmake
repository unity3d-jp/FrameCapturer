set(LIBRARY_PATHS
    /usr/lib
    /usr/local/lib
    ${OGG_DIR}/lib
)

find_path(OGG_INCLUDE_DIR
    ogg/ogg.h
    PATHS ${OGG_DIR}/include
)

find_library(OGG_LIBRARY
    NAMES ogg
    PATHS ${LIBRARY_PATHS}
)

mark_as_advanced(OGG_INCLUDE_DIR)
mark_as_advanced(OGG_LIBRARY)

find_package_handle_standard_args("OGG"
    DEFAULT_MSG
    OGG_LIBRARY
    OGG_INCLUDE_DIR
)
