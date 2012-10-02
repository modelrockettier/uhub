 ##
## Makefile for uhub
## Copyright (C) 2007-2012, Jan Vidar Krey <janvidar@extatic.org>
 #

cmake_minimum_required (VERSION 2.6)

project (uhub)

set (UHUB_VERSION_MAJOR 0)
set (UHUB_VERSION_MINOR 4)
set (UHUB_VERSION_PATCH 1)

set (PROJECT_SOURCE_DIR "src")

option(RELEASE "Release build, debug build if disabled" ON)
option(LINK_SUPPORT "Allow hub linking" OFF)
option(SSL_SUPPORT "Enable SSL support" ON)
option(SQLITE_SUPPORT "Enable SQLite support" ON)

find_package(Git)

if (SSL_SUPPORT)
	find_package(OpenSSL REQUIRED)
endif()

if (MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

include_directories("${PROJECT_SOURCE_DIR}")

set (network_SOURCES
	${PROJECT_SOURCE_DIR}/network/backend.c
	${PROJECT_SOURCE_DIR}/network/connection.c
	${PROJECT_SOURCE_DIR}/network/epoll.c
	${PROJECT_SOURCE_DIR}/network/kqueue.c
	${PROJECT_SOURCE_DIR}/network/select.c
	${PROJECT_SOURCE_DIR}/network/timeout.c
	${PROJECT_SOURCE_DIR}/network/timer.c
	${PROJECT_SOURCE_DIR}/network/network.c
	${PROJECT_SOURCE_DIR}/util/ipcalc.c
)

set (uhub_SOURCES
	${PROJECT_SOURCE_DIR}/core/auth.c
	${PROJECT_SOURCE_DIR}/core/command_parser.c
	${PROJECT_SOURCE_DIR}/core/commands.c
	${PROJECT_SOURCE_DIR}/core/config.c
	${PROJECT_SOURCE_DIR}/core/eventqueue.c
	${PROJECT_SOURCE_DIR}/core/hub.c
	${PROJECT_SOURCE_DIR}/core/hubevent.c
	${PROJECT_SOURCE_DIR}/core/inf.c
	${PROJECT_SOURCE_DIR}/core/ioqueue.c
	${PROJECT_SOURCE_DIR}/core/netevent.c
	${PROJECT_SOURCE_DIR}/core/probe.c
	${PROJECT_SOURCE_DIR}/core/route.c
	${PROJECT_SOURCE_DIR}/core/user.c
	${PROJECT_SOURCE_DIR}/core/usermanager.c
	${PROJECT_SOURCE_DIR}/core/plugincallback.c
	${PROJECT_SOURCE_DIR}/core/plugininvoke.c
	${PROJECT_SOURCE_DIR}/core/pluginloader.c
)

set (adc_SOURCES
	${PROJECT_SOURCE_DIR}/adc/message.c
	${PROJECT_SOURCE_DIR}/adc/sid.c
)

set (utils_SOURCES
	${PROJECT_SOURCE_DIR}/util/cbuffer.c
	${PROJECT_SOURCE_DIR}/util/config_token.c
	${PROJECT_SOURCE_DIR}/util/credentials.c
	${PROJECT_SOURCE_DIR}/util/floodctl.c
	${PROJECT_SOURCE_DIR}/util/getopt.c
	${PROJECT_SOURCE_DIR}/util/list.c
	${PROJECT_SOURCE_DIR}/util/log.c
	${PROJECT_SOURCE_DIR}/util/memory.c
	${PROJECT_SOURCE_DIR}/util/misc.c
	${PROJECT_SOURCE_DIR}/util/rbtree.c
	${PROJECT_SOURCE_DIR}/util/tiger.c
)

set (adcclient_SOURCES
	${PROJECT_SOURCE_DIR}/tools/adcclient.c
)

add_library(adc       STATIC ${adc_SOURCES})
add_library(network   STATIC ${network_SOURCES})
add_library(utils     STATIC ${utils_SOURCES})


add_dependencies(adc utils)
add_dependencies(network utils)

add_executable(uhub ${PROJECT_SOURCE_DIR}/core/main.c ${uhub_SOURCES} )

add_library(mod_example MODULE ${PROJECT_SOURCE_DIR}/plugins/mod_example.c ${utils_SOURCES})
add_library(mod_welcome MODULE ${PROJECT_SOURCE_DIR}/plugins/mod_welcome.c ${utils_SOURCES} ${network_SOURCES})
add_library(mod_logging MODULE ${PROJECT_SOURCE_DIR}/plugins/mod_logging.c ${utils_SOURCES} ${PROJECT_SOURCE_DIR}/adc/sid.c ${network_SOURCES})
add_library(mod_auth_simple MODULE ${PROJECT_SOURCE_DIR}/plugins/mod_auth_simple.c ${utils_SOURCES})
add_library(mod_chat_history MODULE ${PROJECT_SOURCE_DIR}/plugins/mod_chat_history.c ${utils_SOURCES})
add_library(mod_chat_only MODULE ${PROJECT_SOURCE_DIR}/plugins/mod_chat_only.c ${utils_SOURCES})
add_library(mod_topic MODULE ${PROJECT_SOURCE_DIR}/plugins/mod_topic.c ${utils_SOURCES})

if (SQLITE_SUPPORT)
	add_library(mod_auth_sqlite MODULE ${PROJECT_SOURCE_DIR}/plugins/mod_auth_sqlite.c ${utils_SOURCES})
	add_executable(uhub-passwd ${PROJECT_SOURCE_DIR}/tools/uhub-passwd.c ${PROJECT_SOURCE_DIR}/util/misc.c ${PROJECT_SOURCE_DIR}/util/memory.c ${PROJECT_SOURCE_DIR}/util/log.c ${PROJECT_SOURCE_DIR}/util/list.c)

	target_link_libraries(mod_auth_sqlite sqlite3)
	target_link_libraries(uhub-passwd sqlite3)
	set_target_properties(mod_auth_sqlite PROPERTIES PREFIX "")
endif()

if(WIN32)
	target_link_libraries(uhub ws2_32)
	target_link_libraries(mod_logging ws2_32)
	target_link_libraries(mod_welcome ws2_32)
endif()

set_target_properties(
	mod_example
	mod_welcome
	mod_logging
	mod_auth_simple
	mod_chat_history
	mod_chat_only
	mod_topic
	PROPERTIES PREFIX "")

target_link_libraries(uhub ${CMAKE_DL_LIBS} adc network utils)

if(UNIX)
	add_library(adcclient STATIC ${adcclient_SOURCES})
	add_executable(uhub-admin ${PROJECT_SOURCE_DIR}/tools/admin.c ${adcclient_SOURCES})
	target_link_libraries(uhub-admin adcclient adc network utils)
endif()

if(GIT_FOUND)
	execute_process(COMMAND ${GIT_EXECUTABLE} show -s --pretty=format:%h OUTPUT_VARIABLE UHUB_REVISION)
	set (UHUB_GIT_VERSION "${UHUB_VERSION_MAJOR}.${UHUB_VERSION_MINOR}.${UHUB_VERSION_PATCH}-git-${UHUB_REVISION}")
else()
endif()

if(OPENSSL_FOUND)
	add_definitions(-DSSL_SUPPORT=1)
	include_directories(${OPENSSL_INCLUDE_DIR})
	target_link_libraries(uhub ${OPENSSL_LIBRARIES})
	if(UNIX)
		target_link_libraries(uhub-admin ${OPENSSL_LIBRARIES})
	endif()
	target_link_libraries(mod_welcome ${OPENSSL_LIBRARIES})
	target_link_libraries(mod_logging ${OPENSSL_LIBRARIES})
endif()

configure_file ("${PROJECT_SOURCE_DIR}/version.h.in" "${PROJECT_SOURCE_DIR}/version.h")

mark_as_advanced(FORCE CMAKE_BUILD_TYPE)
if (RELEASE)
	set(CMAKE_BUILD_TYPE Release)
else()
	set(CMAKE_BUILD_TYPE Debug)
endif()






 