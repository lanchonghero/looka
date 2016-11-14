CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

INCLUDE_DIRECTORIES(
  /usr/include/mysql
  /usr/include/libxml2
  ${MMSEG_PREFIX}/include/mmseg
  ${CMAKE_SOURCE_DIR}/deps/jsoncpp/include
)

AUX_SOURCE_DIRECTORY(. CUR_SRCS)
AUX_SOURCE_DIRECTORY(../ PUB_SRCS)
AUX_SOURCE_DIRECTORY(../httpserver HTTPSERVER_SRCS)

SET(LIBRARIES
  libpthread.so
  libxml2.so
  ${MMSEG_PREFIX}/lib/libmmseg.a
  ${CMAKE_BINARY_DIR}/lib/libjsoncpp.so
  /usr/lib/x86_64-linux-gnu/libmysqlclient.so
)

ADD_EXECUTABLE(searchd ${CUR_SRCS} ${PUB_SRCS} ${HTTPSERVER_SRCS})
TARGET_LINK_LIBRARIES(searchd ${LIBRARIES})
