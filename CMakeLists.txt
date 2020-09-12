##
## Makefile for uhub
## Copyright (C) 2007-2018, Jan Vidar Krey <janvidar@extatic.org>
## Copyright (c) 2020, Tim Schlueter
## SPDX-License-Identifier: GPL-3.0-or-later
##

# May work on older versions, but 2.8.12 is the oldest it's tested to
# work with (CentOS 7).
cmake_minimum_required(VERSION 2.8.12)

project(uhub C)

set(UHUB_VERSION_MAJOR 0)
set(UHUB_VERSION_MINOR 5)
set(UHUB_VERSION_PATCH 1)

set(CMAKE_PROJECT_HOMEPAGE_URL "https://uhub.org/")

include(GNUInstallDirs)

set(PROJECT_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

set(BIN_DIR    ${CMAKE_INSTALL_FULL_BINDIR}      CACHE PATH "Where to install the uhub executables")
set(PLUGIN_DIR ${CMAKE_INSTALL_FULL_LIBDIR}/uhub CACHE PATH "Where to install the uhub plugins")
set(CONFIG_DIR /etc/uhub                         CACHE PATH "Where to install the uhub configs")
set(LOG_DIR    /var/log                          CACHE PATH "The default log location for uhub")

option(LOWLEVEL_DEBUG  "Enable low level debug messages" OFF)
option(SSL_SUPPORT     "Enable SSL support" ON)
option(HARDENING       "Enable compiler options to harden uhub against memory corruption attacks" OFF)
option(SYSTEMD_SUPPORT "Enable systemd notify and journal logging" OFF)
option(ADC_STRESS      "Enable the stress tester client" OFF)
option(COVERAGE        "Enable code coverage reports" OFF)

# Set position independent to on by default
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


# Control generation of Position Independent Executable (PIE)
if(POLICY CMP0083)
	cmake_policy(SET CMP0083 NEW)

	# Check if pie linker arguments work
	include(CheckPIESupported)

	# Retrieve any error message.
	check_pie_supported(OUTPUT_VARIABLE pie_output LANGUAGES C)
	if(NOT CMAKE_C_LINK_PIE_SUPPORTED)
	  message(WARNING "PIE is not supported at link time: ${pie_output}.\n"
					  "PIE link options will not be passed to linker.")
	endif()
endif()

# Only interpret if() arguments as variables or keywords when unquoted.
if(POLICY CMP0054)
	cmake_policy(SET CMP0054 NEW)
endif()

# Honor visibility properties for all target types.
if(POLICY CMP0063)
	cmake_policy(SET CMP0063 NEW)
endif()

include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME uhub)
set(CPACK_PACKAGE_VENDOR "Jan Vidar Krey")
set(CPACK_PACKAGE_VERSION_MAJOR ${UHUB_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${UHUB_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${UHUB_VERSION_PATCH})

set(CPACK_BINARY_DEB "OFF")
set(CPACK_BINARY_RPM "OFF")
set(CPACK_BINARY_TGZ "ON")
set(CPACK_BINARY_ZIP "OFF")
set(CPACK_DEBIAN_PACKAGE_SECTION "net")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS True)
set(CPACK_GENERATOR "TGZ")
set(CPACK_INSTALL_CMAKE_PROJECTS "${PROJECT_BINARY_DIR};uhub;ALL;/")
set(CPACK_PACKAGE_CONTACT "Jan Vidar Krey <janvidar@extatic.org>")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/doc/description.txt")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High performance ADC p2p hub.")
set(CPACK_PACKAGE_HOMEPAGE_URL ${CMAKE_PROJECT_HOMEPAGE_URL})
set(CPACK_PACKAGE_RELOCATABLE False)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
#set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
#set(CPACK_RESOURCE_FILE_WELCOME "${CMAKE_SOURCE_DIR}/doc/motd.txt")
set(CPACK_RPM_BUILDREQUIRES "cmake, gcc, make, openssl-devel, sqlite-devel") # pkgconfig systemd-devel
#set(CPACK_RPM_CHANGELOG_FILE "${CMAKE_SOURCE_DIR}/ChangeLog")
set(CPACK_RPM_PACKAGE_LICENSE "GPLv3+")
set(CPACK_RPM_PACKAGE_RELEASE 3)
set(CPACK_RPM_PACKAGE_URL ${CMAKE_PROJECT_HOMEPAGE_URL})
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES "~$;[.]swp$;/[.]svn/;/[.]git/;.gitignore;/build/;tags;cscope.*")
set(CPACK_SOURCE_RPM "OFF")
set(CPACK_SOURCE_TGZ "ON")
set(CPACK_SOURCE_ZIP "")
include(CPack)


include(BuildType)

find_package(Git)
find_package(Sqlite3)

include(CheckCSourceCompiles)
include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckTypeSize)
include(TestBigEndian)

#Some functions need this to be found
add_definitions(-D_GNU_SOURCE)
list(APPEND CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")

test_big_endian(BIGENDIAN)
if(BIGENDIAN)
	add_definitions(-DARCH_BIGENDIAN)
	list(APPEND CMAKE_REQUIRED_DEFINITIONS "-DARCH_BIGENDIAN")
endif()

if(SSL_SUPPORT)
	find_package(OpenSSL)
	if(NOT OPENSSL_FOUND)
		message(FATAL_ERROR "OpenSSL is not found!")
	endif()
endif()

if(NOT SQLITE3_FOUND)
	message(FATAL_ERROR "SQLite3 is not found!")
endif()

if(SYSTEMD_SUPPORT)
	include(FindPkgConfig)
	pkg_search_module(SD REQUIRED libsystemd)
endif()

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	list(APPEND CMAKE_REQUIRED_DEFINITIONS "-D_CRT_SECURE_NO_WARNINGS")
endif()

if(HARDENING)
	if(MSVC)
		# Enable ASLR
		set(LINKER_FLAGS_TO_USE /DYNAMICBASE /NXCOMPAT)
	else()
		add_definitions(-D_FORTIFY_SOURCE=2)
		list(APPEND CMAKE_REQUIRED_DEFINITIONS "-D_FORTIFY_SOURCE=2")

		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wformat -Werror=format-security -Wall -fstack-protector-strong -fstack-check -O2")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,relro -Wl,-z,now ")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,relro -Wl,-z,now ")
	endif()
endif()

if(COVERAGE)
	if(MSVC)
		message(FATAL_ERROR "COVERAGE is not supported with MSVC")
	else()
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb3 --coverage")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O0 -ggdb3 --coverage")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -O0 -ggdb3 --coverage")
	endif()
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

check_c_source_compiles("
__declspec(dllexport) int a = 0;
int main(void) { return a; }
" HAVE_DECLSPEC)

check_c_source_compiles("
__attribute__((unused)) static void foo(void) {};
int main(void) { return 0; }
" HAVE_ATTRIBUTE)

if(HAVE_ATTRIBUTE)
	set(NO_RETURN "__attribute__((noreturn))")
elseif(HAVE_DECLSPEC)
	set(NO_RETURN "__declspec(noreturn)")
endif()

if(HAVE_DECLSPEC)
	set(PLUGIN_API "__declspec(dllexport)")
elseif(HAVE_ATTRIBUTE)
	set(PLUGIN_API "__attribute__((visibility(\"default\")))")
endif()

if(HAVE_DECLSPEC OR HAVE_ATTRIBUTE)
	set(CMAKE_C_VISIBILITY_PRESET hidden)
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
	mod_joins
	mod_logging
	mod_restrict
	mod_topic
	mod_users
	mod_welcome
)

set(CONFIG_FILES
	${CMAKE_SOURCE_DIR}/doc/rules.txt
	${CMAKE_SOURCE_DIR}/doc/motd.txt
	# These are generated from their .in counterparts
	${PROJECT_BINARY_DIR}/uhub.conf
	${PROJECT_BINARY_DIR}/plugins.conf
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
add_executable(uhub-passwd ${PROJECT_SOURCE_DIR}/tools/uhub-passwd.c)
add_executable(autotest-bin ${CMAKE_SOURCE_DIR}/tests/auto/test.c ${uhub_SOURCES})
add_executable(passwd-test ${CMAKE_SOURCE_DIR}/tests/passwd/test.c)

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

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_C_FLAGS_DEBUG          "${CMAKE_C_FLAGS_DEBUG} -ggdb3")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -ggdb3")
endif()

target_link_libraries(utils network)
target_link_libraries(uhub ${CMAKE_DL_LIBS} adc network utils)
target_link_libraries(uhub-passwd ${SQLITE3_LIBRARIES} utils)
target_link_libraries(autotest-bin ${CMAKE_DL_LIBS} adc network utils)
target_link_libraries(passwd-test ${SQLITE3_LIBRARIES} utils)

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
	target_link_libraries(utils ${SD_LIBRARIES})
	target_link_libraries(uhub ${SD_LIBRARIES})

	include_directories(${SD_INCLUDE_DIRS})
	add_definitions(-DSYSTEMD)
endif()

configure_file("${PROJECT_SOURCE_DIR}/version.h.in" "${PROJECT_BINARY_DIR}/version.h")
configure_file("${PROJECT_SOURCE_DIR}/system.h.in" "${PROJECT_BINARY_DIR}/system.h")

configure_file("${CMAKE_SOURCE_DIR}/doc/uhub.conf.in" "${PROJECT_BINARY_DIR}/uhub.conf")
configure_file("${CMAKE_SOURCE_DIR}/doc/plugins.conf.in" "${PROJECT_BINARY_DIR}/plugins.conf")

if(CMAKE_BUILD_TYPE STREQUAL Debug OR CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
	add_definitions(-DDEBUG)

	if(LOWLEVEL_DEBUG)
		add_definitions(-DLOWLEVEL_DEBUG)
	endif()
elseif(LOWLEVEL_DEBUG)
	message(WARNING "Can't use LOWLEVEL_DEBUG with a Release build, try -DCMAKE_BUILD_TYPE=RelWithDebInfo")
endif()

math(EXPR POINTER_SIZE "${CMAKE_SIZEOF_VOID_P} * 8")

message(STATUS "**** Build Configuration ***")
message(STATUS "  CMAKE_BUILD_TYPE:     ${CMAKE_BUILD_TYPE}")
message(STATUS "  CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  POINTER_SIZE:         ${POINTER_SIZE}-bits")
message(STATUS "  BIN_DIR:              ${BIN_DIR}")
message(STATUS "  CONFIG_DIR:           ${CONFIG_DIR}")
message(STATUS "  COVERAGE:             ${COVERAGE}")
message(STATUS "  HARDENING:            ${HARDENING}")
message(STATUS "  LOG_DIR:              ${LOG_DIR}")
message(STATUS "  LOWLEVEL_DEBUG:       ${LOWLEVEL_DEBUG}")
message(STATUS "  PLUGIN_DIR:           ${PLUGIN_DIR}")
message(STATUS "  SSL_SUPPORT:          ${SSL_SUPPORT}")
message(STATUS "  SYSTEMD_SUPPORT:      ${SYSTEMD_SUPPORT}")
message(STATUS "**** End Build Configuration ***")

enable_testing()
# By default, only print failing tests and a test summary
add_test(NAME autotest COMMAND autotest-bin -f -s)
if(${CMAKE_VERSION} VERSION_LESS "3.0.0")
	# CMake < v3.0 doesn't support generators in the working directory, so
	# fall back to just using the current directory
	add_test(NAME passwd-test COMMAND passwd-test -f -s)
else()
	# The passwd-test uses the uhub-passwd in the current working directory
	add_test(NAME passwd-test COMMAND passwd-test -f -s
			WORKING_DIRECTORY $<TARGET_FILE_DIR:uhub-passwd>)
endif()

if(UNIX)
	install(TARGETS ${BINARIES} RUNTIME DESTINATION ${BIN_DIR})
	install(TARGETS ${PLUGINS}  LIBRARY DESTINATION ${PLUGIN_DIR})
	install(FILES   ${CONFIG_FILES}     DESTINATION ${CONFIG_DIR})
	install(FILES   ${MAN_PAGES}        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
endif()

