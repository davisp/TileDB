if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CARGO_CMD cargo build -p tiledb-rust-api)
    set(TARGET_DIR "debug")
else ()
    set(CARGO_CMD cargo build --release -p tiledb-rust-api)
    set(TARGET_DIR "release")
endif ()

set(TILEDB_RUST_API_H "${CMAKE_CURRENT_BINARY_DIR}/tiledb_rust_api.h")
set(TILEDB_RUST_API_CXX "${CMAKE_CURRENT_BINARY_DIR}/tiledb_rust_api.cpp")
set(TILEDB_RUST_API_LIB "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_DIR}/libtiledb_rust_api.a")

add_custom_command(
    OUTPUT ${TILEDB_RUST_API_H} ${TILEDB_RUST_API_CXX}
    COMMAND MACOSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET} CARGO_TARGET_DIR=${CMAKE_CURRENT_BINARY_DIR} RUSTFLAGS="${RUST_FLAGS}" ${CARGO_CMD}
    COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/cxxbridge/tiledb-rust-api/src/lib.rs.h  ${TILEDB_RUST_API_H}
    COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/cxxbridge/tiledb-rust-api/src/lib.rs.cc ${TILEDB_RUST_API_CXX}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(tiledb_rust_generated
    DEPENDS
        ${TILEDB_RUST_API_H}
        ${TILEDB_RUST_API_CXX}
)

add_library(tiledb_rust_bridge STATIC ${TILEDB_RUST_API_CXX})
add_dependencies(tiledb_rust_bridge tiledb_rust_generated)
target_include_directories(tiledb_rust_bridge PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/..")
target_link_libraries(tiledb_rust_bridge pthread dl ${TILEDB_RUST_API_LIB})

add_test(
    NAME tiledb_rust
    COMMAND CARGO_TARGET_DIR=${CMAKE_CURRENT_BINARY_DIR} cargo test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
