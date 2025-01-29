cmake_minimum_required(VERSION 3.0)
list(APPEND CMAKE_MODULE_PATH "/usr/local/share/cmake/Modules/")
list(APPEND CMAKE_MODULE_PATH "/usr/local/lib/cmake/LibDataChannel/")

project(zavodchat)

file(GLOB_RECURSE src src/*.cpp src/*.h)

add_executable(zavodchat ${src})

add_compile_options(-fsanitize=thread -fsanitize=address)

# libraries
find_library(PQXX_LIB pqxx)
find_library(PQ_LIB pq)
find_library(IXWEBSOCKET_LIBRARY ixwebsocket)

include(FindLibHttpServer)
include(FindGnuTLS)
include(FindOpenSSL)
include(FindZLIB)
include(LibDataChannelConfig)

find_package(PkgConfig REQUIRED)
pkg_search_module(gstreamer REQUIRED IMPORTED_TARGET gstreamer-1.0)
pkg_search_module(gstreamer-rtp REQUIRED IMPORTED_TARGET gstreamer-rtp-1.0)

target_include_directories(zavodchat PUBLIC 
	"${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(zavodchat PUBLIC
	${PQXX_LIB} ${PQ_LIB} 
	${LIBHTTPSERVER_LIBRARIES} GnuTLS::GnuTLS
	${IXWEBSOCKET_LIBRARY} ZLIB::ZLIB OpenSSL::SSL
	LibDataChannel::LibDataChannel
	PkgConfig::gstreamer
	PkgConfig::gstreamer-rtp
)
