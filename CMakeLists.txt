##
## Makefile for uhub
## Copyright (C) 2007-2013, Jan Vidar Krey <janvidar@extatic.org>
##

# May work on older versions, but 2.8.12 is the oldest it's tested to
# work with (CentOS 7).
cmake_minimum_required(VERSION 2.8.12)

project(uhub NONE)
enable_language(C)

set(UHUB_VERSION_MAJOR 0)
set(UHUB_VERSION_MINOR 5)
set(UHUB_VERSION_PATCH 1)

include(GNUInstallDirs)

set(PROJECT_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

set(BIN_DIR    ${CMAKE_INSTALL_FULL_BINDIR}      CACHE PATH "Where to install the uhub executables")
set(PLUGIN_DIR ${CMAKE_INSTALL_FULL_LIBDIR}/uhub CACHE PATH "Where to install the uhub plugins")
set(CONFIG_DIR /etc/uhub                         CACHE PATH "Where to install the uhub configs")
set(LOG_DIR    /var/log                          CACHE PATH "The default log location for uhub")

option(LOWLEVEL_DEBUG  "Enable low level debug messages" OFF)
option(SSL_SUPPORT     "Enable SSL support" ON)
option(USE_OPENSSL     "Use OpenSSL's SSL support" ON)
option(SYSTEMD_SUPPORT "Enable systemd notify and journal logging" OFF)
option(ADC_STRESS      "Enable the stress tester client" OFF)

# Set position independent to on by default
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(BuildType)

find_package(Git)
find_package(Sqlite3)

include(TestBigEndian)
include(CheckSymbolExists)
include(CheckIncludeFile)
include(CheckTypeSize)

#Some functions need this to be found
add_definitions(-D_GNU_SOURCE)
list(APPEND CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")

TEST_BIG_ENDIAN(BIGENDIAN)
if(BIGENDIAN)
	add_definitions(-DARCH_BIGENDIAN)
endif()

if(SSL_SUPPORT)
	if(USE_OPENSSL)
		find_package(OpenSSL)
	else()
		find_package(GnuTLS)
	endif()
	if(NOT GNUTLS_FOUND AND NOT OPENSSL_FOUND)
		message(FATAL_ERROR "Neither OpenSSL nor GnuTLS were found!")
	endif()
endif()

if(NOT SQLITE3_FOUND)
	message(FATAL_ERROR "SQLite3 is not found!")
endif()

if(SYSTEMD_SUPPORT)
	INCLUDE(FindPkgConfig)
	pkg_search_module(SD REQUIRED libsystemd)
endif()

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_type_size(ssize_t SSIZE_T)
check_symbol_exists(memmem string.h HAVE_MEMMEM)
check_symbol_exists(strndup string.h HAVE_STRNDUP)
check_symbol_exists(strlcpy string.h HAVE_STRLCPY)
check_symbol_exists(localtime_r time.h HAVE_LOCALTIME_R)
check_symbol_exists(localtime_s time.h HAVE_LOCALTIME_S)

if(HAVE_SYS_TYPES_H)
	list(APPEND CMAKE_EXTRA_INCLUDE_FILES "sys/types.h")
endif()

include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_BINARY_DIR}")
include_directories(${SQLITE3_INCLUDE_DIRS})
link_directories(${SQLITE3_LIBRARY_DIRS})

file(GLOB uhub_SOURCES ${PROJECT_SOURCE_DIR}/core/*.c)
list(REMOVE_ITEM uhub_SOURCES
	${PROJECT_SOURCE_DIR}/core/gen_config.c
	${PROJECT_SOURCE_DIR}/core/main.c
)

file(GLOB adc_SOURCES ${PROJECT_SOURCE_DIR}/adc/*.c)
file(GLOB network_SOURCES ${PROJECT_SOURCE_DIR}/network/*.c)
file(GLOB utils_SOURCES ${PROJECT_SOURCE_DIR}/util/*.c)

set(adcclient_SOURCES
	${PROJECT_SOURCE_DIR}/tools/adcclient.c
	${PROJECT_SOURCE_DIR}/core/ioqueue.c
)

set(BINARIES
	uhub
	uhub-passwd
)

set(PLUGINS
	mod_auth_simple
	mod_auth_sqlite
	mod_chat_history
	mod_chat_history_sqlite
	mod_chat_only
	mod_example
	mod_guest_passwd
	mod_logging # NOTE: depends on adc and network (later)
	mod_no_guest_downloads
	mod_topic
	mod_welcome # NOTE: depends on adc and network (later)
)

set(CONFIG_FILES
	${PROJECT_BINARY_DIR}/uhub.conf
	${PROJECT_BINARY_DIR}/plugins.conf
	${CMAKE_SOURCE_DIR}/doc/rules.txt
	${CMAKE_SOURCE_DIR}/doc/motd.txt
)

set(MAN_PAGES
	${CMAKE_SOURCE_DIR}/doc/uhub.1
	${CMAKE_SOURCE_DIR}/doc/uhub-passwd.1
)

add_library(adc       STATIC ${adc_SOURCES})
add_library(network   STATIC ${network_SOURCES})
add_library(utils     STATIC ${utils_SOURCES})

add_dependencies(adc utils)
add_dependencies(network utils)

# Ensure these are position independent even if it's off globally
set_property(TARGET adc     PROPERTY POSITION_INDEPENDENT_CODE TRUE)
set_property(TARGET network PROPERTY POSITION_INDEPENDENT_CODE TRUE)
set_property(TARGET utils   PROPERTY POSITION_INDEPENDENT_CODE TRUE)

add_executable(uhub ${PROJECT_SOURCE_DIR}/core/main.c ${uhub_SOURCES})
add_executable(autotest-bin ${CMAKE_SOURCE_DIR}/autotest/test.c ${uhub_SOURCES})
add_executable(uhub-passwd ${PROJECT_SOURCE_DIR}/tools/uhub-passwd.c)

foreach(p IN LISTS PLUGINS)
	add_library(${p} MODULE ${PROJECT_SOURCE_DIR}/plugins/${p}.c)
	# Allow plugins to use the utility functions
	target_link_libraries(${p} utils)
	# Also link in sqlite if the module has sqlite in the name
	if(p MATCHES sqlite)
		target_link_libraries(${p} ${SQLITE3_LIBRARIES})
	endif()
endforeach()

set_target_properties(${PLUGINS} PROPERTIES PREFIX "")

SET(COMPILE_DEFINITIONS_DEBUG "${COMPILE_DEFINITIONS_DEBUG} -DDEBUG")

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_C_FLAGS_DEBUG          "${CMAKE_C_FLAGS_DEBUG} -ggdb3")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -ggdb3")
endif()

target_link_libraries(uhub ${CMAKE_DL_LIBS} adc network utils)
target_link_libraries(uhub-passwd ${SQLITE3_LIBRARIES} utils)
target_link_libraries(autotest-bin ${CMAKE_DL_LIBS} adc network utils)
target_link_libraries(utils network)

# logging plugin uses ip_convert_to_string() from network and sid_to_string() from adc
target_link_libraries(mod_logging adc network)
# welcome plugin uses ip_convert_to_string() from network and sid_to_string() from adc
target_link_libraries(mod_welcome adc network)

if(WIN32)
	target_link_libraries(uhub ws2_32)
	target_link_libraries(autotest-bin ws2_32)
	target_link_libraries(network ws2_32)
endif()

if(UNIX)
	add_library(adcclient STATIC ${adcclient_SOURCES})
	add_executable(uhub-admin ${PROJECT_SOURCE_DIR}/tools/admin.c)
	list(APPEND BINARIES uhub-admin)
	target_link_libraries(uhub-admin adcclient adc network utils pthread)
	target_link_libraries(uhub pthread)
	target_link_libraries(autotest-bin pthread)

	if(ADC_STRESS)
		add_executable(adcrush ${PROJECT_SOURCE_DIR}/tools/adcrush.c ${adcclient_SOURCES})
		target_link_libraries(adcrush adcclient adc network utils pthread)
	endif()
endif()

if(NOT UHUB_REVISION AND GIT_FOUND)
	execute_process(COMMAND ${GIT_EXECUTABLE} show -s --pretty=format:%h
					WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
					OUTPUT_VARIABLE UHUB_REVISION_TEMP
					OUTPUT_STRIP_TRAILING_WHITESPACE)
	if(UHUB_REVISION_TEMP)
		set(UHUB_REVISION "git-${UHUB_REVISION_TEMP}")
	endif()
endif()

if(NOT UHUB_REVISION)
	set(UHUB_REVISION "release")
endif()

set(UHUB_GIT_VERSION "${UHUB_VERSION_MAJOR}.${UHUB_VERSION_MINOR}.${UHUB_VERSION_PATCH}-${UHUB_REVISION}")
message(STATUS "Configuring uhub version: ${UHUB_GIT_VERSION}")

if(OPENSSL_FOUND)
	set(SSL_LIBS ${OPENSSL_LIBRARIES})
	add_definitions(-DSSL_SUPPORT=1 -DSSL_USE_OPENSSL=1)
	include_directories(${OPENSSL_INCLUDE_DIR})
endif()

if(GNUTLS_FOUND)
	set(SSL_LIBS ${GNUTLS_LIBRARIES})
	add_definitions(-DSSL_SUPPORT=1 -DSSL_USE_GNUTLS=1 ${GNUTLS_DEFINITIONS})
	include_directories(${GNUTLS_INCLUDE_DIR})
endif()

if(SSL_SUPPORT)
	target_link_libraries(uhub ${SSL_LIBS})
	target_link_libraries(autotest-bin ${SSL_LIBS})
	target_link_libraries(network ${SSL_LIBS})
	if(UNIX)
		target_link_libraries(uhub-admin ${SSL_LIBS})
	endif()
	if(ADC_STRESS)
		target_link_libraries(adcrush ${SSL_LIBS})
	endif()
endif()

if(SYSTEMD_SUPPORT)
	target_link_libraries(uhub ${SD_LIBRARIES})
	target_link_libraries(autotest-bin ${SD_LIBRARIES})
	target_link_libraries(uhub-passwd ${SD_LIBRARIES})
	target_link_libraries(uhub-admin ${SD_LIBRARIES})
	include_directories(${SD_INCLUDE_DIRS})
	add_definitions(-DSYSTEMD)
	if(ADC_STRESS)
		target_link_libraries(adcrush ${SD_LIBRARIES})
	endif()
endif()

configure_file("${PROJECT_SOURCE_DIR}/version.h.in" "${PROJECT_BINARY_DIR}/version.h")
configure_file("${PROJECT_SOURCE_DIR}/system.h.in" "${PROJECT_BINARY_DIR}/system.h")

configure_file("${CMAKE_SOURCE_DIR}/doc/uhub.conf.in" "${PROJECT_BINARY_DIR}/uhub.conf")
configure_file("${CMAKE_SOURCE_DIR}/doc/plugins.conf.in" "${PROJECT_BINARY_DIR}/plugins.conf")

if(LOWLEVEL_DEBUG)
	add_definitions(-DLOWLEVEL_DEBUG)
endif()

message(STATUS "**** Build Configuration ***")
message(STATUS "  CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "  CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  BIN_DIR: ${BIN_DIR}")
message(STATUS "  CONFIG_DIR: ${CONFIG_DIR}")
message(STATUS "  LOG_DIR: ${LOG_DIR}")
message(STATUS "  PLUGIN_DIR: ${PLUGIN_DIR}")
message(STATUS "  SSL_SUPPORT: ${SSL_SUPPORT}")
message(STATUS "  SYSTEMD_SUPPORT: ${SYSTEMD_SUPPORT}")
message(STATUS "**** End Build Configuration ***")

enable_testing()
add_test(NAME autotest COMMAND autotest-bin)

if(UNIX)
	install(TARGETS ${BINARIES} RUNTIME DESTINATION ${BIN_DIR})
	install(TARGETS ${PLUGINS}  LIBRARY DESTINATION ${PLUGIN_DIR})
	install(FILES   ${CONFIG_FILES}     DESTINATION ${CONFIG_DIR})
	install(FILES   ${MAN_PAGES}        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
endif()

