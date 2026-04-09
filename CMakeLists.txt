cmake_minimum_required(VERSION 3.16)
project(nexuschat LANGUAGES C)
include(GNUInstallDirs)

find_program(CARGO cargo)

if(APPLE)
       set(DYNAMIC_EXT "dylib")
elseif(UNIX)
       set(DYNAMIC_EXT "so")
else()
       set(DYNAMIC_EXT "dll")
endif()

if(DEFINED ENV{CARGO_BUILD_TARGET})
    set(ARCH_DIR "$ENV{CARGO_BUILD_TARGET}")
else()
    set(ARCH_DIR "./")
endif()

add_custom_command(
	OUTPUT
	"${CMAKE_BINARY_DIR}/target/release/libnexuschat.a"
	"${CMAKE_BINARY_DIR}/target/release/libnexuschat.${DYNAMIC_EXT}"
	"${CMAKE_BINARY_DIR}/target/release/pkgconfig/nexuschat.pc"
        COMMAND
        PREFIX=${CMAKE_INSTALL_PREFIX}
        LIBDIR=${CMAKE_INSTALL_FULL_LIBDIR}
        INCLUDEDIR=${CMAKE_INSTALL_FULL_INCLUDEDIR}
	${CARGO} build --target-dir=${CMAKE_BINARY_DIR}/target --release
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/nexuschat-ffi
)

add_custom_target(
	lib_nexuschat
	ALL
	DEPENDS
	"${CMAKE_BINARY_DIR}/target/release/libnexuschat.a"
	"${CMAKE_BINARY_DIR}/target/release/libnexuschat.${DYNAMIC_EXT}"
	"${CMAKE_BINARY_DIR}/target/release/pkgconfig/nexuschat.pc"
)

install(FILES "nexuschat-ffi/nexuschat.h" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES "${CMAKE_BINARY_DIR}/target/${ARCH_DIR}/release/libnexuschat.a" DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES "${CMAKE_BINARY_DIR}/target/${ARCH_DIR}/release/libnexuschat.${DYNAMIC_EXT}" DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES "${CMAKE_BINARY_DIR}/target/${ARCH_DIR}/release/pkgconfig/nexuschat.pc" DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
