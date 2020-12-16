#
# @author Tobias Weber <tweber@ill.fr>
# @date 6-apr-2018
# @license see 'LICENSE' file
#

project(scanbrowser)
cmake_minimum_required(VERSION 3.0)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/../../../tlibs2/cmake")

message("Build type: ${CMAKE_BUILD_TYPE}")

if(NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	set(CMAKE_VERBOSE_MAKEFILE TRUE)
endif()


set(CMAKE_CXX_STANDARD 20)
add_compile_options(-std=c++20)
add_compile_options(-Wall -Wextra)


# -----------------------------------------------------------------------------
# Boost
add_compile_options(-DBOOST_SYSTEM_NO_DEPRECATED)
find_package(Boost REQUIRED COMPONENTS system filesystem iostreams REQUIRED)
add_compile_options(${Boost_CXX_FLAGS})

# Qt
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets PrintSupport)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# qcp
find_package(QCP REQUIRED)

# Minuit
find_package(Minuit2 REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)
# -----------------------------------------------------------------------------



# -----------------------------------------------------------------------------
# Build parser
find_package(FLEX REQUIRED)
find_package(BISON 3.0 REQUIRED)

# temp dir for parser
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/parser)

# parser
BISON_TARGET(cliparser cli/cliparser.y ${CMAKE_CURRENT_BINARY_DIR}/parser/cliparser_impl.cpp
	COMPILE_FLAGS "-S lalr1.cc --defines=${CMAKE_CURRENT_BINARY_DIR}/parser/cliparser_impl.h")

# lexer
FLEX_TARGET(clilexer cli/clilexer.l ${CMAKE_CURRENT_BINARY_DIR}/parser/clilexer_impl.cpp
	COMPILE_FLAGS "--c++ --header-file=${CMAKE_CURRENT_BINARY_DIR}/parser/clilexer_impl.h")
ADD_FLEX_BISON_DEPENDENCY(clilexer cliparser)

# let moc ignore the generated files
set_property(SOURCE
	parser/cliparser_impl.cpp parser/cliparser_impl.h parser/cliparser_impl.hpp
	parser/clilexer_impl.cpp parser/clilexer_impl.h parser/clilexer_impl.hpp
	PROPERTY SKIP_AUTOGEN ON)
# -----------------------------------------------------------------------------



include_directories(
	"${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/ext" "${PROJECT_SOURCE_DIR}/ext/qcp"
	"${PROJECT_SOURCE_DIR}/../.."
	"${PROJECT_SOURCE_DIR}/../../ext" "${PROJECT_SOURCE_DIR}/../../ext/qcp"
	"${PROJECT_SOURCE_DIR}/cli"
	"${Boost_INCLUDE_DIRS}" "${Boost_INCLUDE_DIRS}/.." "${FLEX_INCLUDE_DIRS}"
	"${PROJECT_BINARY_DIR}/parser"
	"${QCP_INCLUDE_DIRS}"
)


add_executable(takin_scanbrowser
	main.cpp mainwnd.cpp mainwnd.h
	filebrowser.cpp filebrowser.h
	workspace.cpp workspace.h
	data.cpp data.h plot.cpp plot.h
	command.cpp command.h
	globals.cpp globals.h
	hacks.cpp hacks.h

	${BISON_cliparser_OUTPUT_SOURCE} ${BISON_cliparser_OUTPUT_HEADER}
	${FLEX_clilexer_OUTPUTS} ${FLEX_clilexer_OUTPUT_HEADER}
	cli/cliparser.cpp cli/cliparser.h cli/cliparser_types.h
	cli/ast.cpp cli/sym.cpp
	cli/funcs.cpp cli/funcs.h

	../../tlibs2/libs/log.cpp
#	../../ext/qcp/qcustomplot.cpp ../../ext/qcp/qcustomplot.h
)

qt5_use_modules(takin_scanbrowser Core Gui Widgets OpenGL)

target_link_libraries(takin_scanbrowser
	${Boost_LIBRARIES}
	${QCP_LIBRARIES}
	${Minuit2_LIBRARIES} Threads::Threads OpenMP::OpenMP_CXX
	-ldl
)
