#
# @author Tobias Weber <tweber@ill.fr>
# @date 6-apr-2018
# @license see 'LICENSE' file
#

project(in20tools)
cmake_minimum_required(VERSION 3.0)


message("Build type: ${CMAKE_BUILD_TYPE}")

if(NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	set(CMAKE_VERBOSE_MAKEFILE TRUE)
endif()


set(CMAKE_CXX_STANDARD 17)
add_definitions(-std=c++17)



# -----------------------------------------------------------------------------
# Boost
find_package(Boost REQUIRED COMPONENTS system filesystem iostreams REQUIRED)
add_definitions(${Boost_CXX_FLAGS})

# Qt
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
# -----------------------------------------------------------------------------



# -----------------------------------------------------------------------------
# Build parser
find_package(Flex REQUIRED)
find_package(Bison 3.0 REQUIRED)

# temp dir for parser
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/parser)

# parser
BISON_TARGET(cliparser tools/cli/cliparser.y ${CMAKE_CURRENT_BINARY_DIR}/parser/cliparser_impl.cpp
	COMPILE_FLAGS "-S lalr1.cc --defines=${CMAKE_CURRENT_BINARY_DIR}/parser/cliparser_impl.h")

# lexer
FLEX_TARGET(clilexer tools/cli/clilexer.l ${CMAKE_CURRENT_BINARY_DIR}/parser/clilexer_impl.cpp
	COMPILE_FLAGS "--c++ --header-file=${CMAKE_CURRENT_BINARY_DIR}/parser/clilexer_impl.h")
ADD_FLEX_BISON_DEPENDENCY(clilexer cliparser)

# let moc ignore the generated files
set_property(SOURCE
	parser/cliparser_impl.cpp parser/cliparser_impl.h parser/cliparser_impl.hpp
	parser/clilexer_impl.cpp parser/clilexer_impl.h parser/clilexer_impl.hpp
	PROPERTY SKIP_AUTOGEN ON)
# -----------------------------------------------------------------------------



include_directories(
	"${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/ext"
	"${Boost_INCLUDE_DIRS}" "${Boost_INCLUDE_DIRS}/.."
	"${PROJECT_SOURCE_DIR}/tools/cli" "${PROJECT_BINARY_DIR}/parser"
)


add_executable(in20
	tools/in20/main.cpp tools/in20/mainwnd.cpp tools/in20/mainwnd.h
	tools/in20/filebrowser.cpp tools/in20/filebrowser.h
	tools/in20/workspace.cpp tools/in20/workspace.h
	tools/in20/data.cpp tools/in20/data.h
	tools/in20/plot.cpp tools/in20/plot.h
	tools/in20/command.cpp tools/in20/command.h
	tools/in20/globals.cpp tools/in20/globals.h

	${BISON_cliparser_OUTPUT_SOURCE} ${BISON_cliparser_OUTPUT_HEADER}
	${FLEX_clilexer_OUTPUTS} ${FLEX_clilexer_OUTPUT_HEADER}
	tools/cli/cliparser.cpp tools/cli/cliparser.h tools/cli/cliparser_types.h
	tools/cli/ast.cpp tools/cli/sym.cpp
	tools/cli/funcs.cpp tools/cli/funcs.h

	ext/tlibs/log/log.cpp ext/tlibs/file/loadinstr.cpp

	ext/qcp/qcustomplot.cpp ext/qcp/qcustomplot.h
)


target_link_libraries(in20
	${Boost_LIBRARIES}
	Qt5::Core Qt5::Gui Qt5::Widgets Qt5::PrintSupport
)
