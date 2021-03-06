CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

FIND_PACKAGE(MMSEG REQUIRED)
FIND_PACKAGE(MYSQL REQUIRED)
FIND_PACKAGE(LibXml2 REQUIRED)

INCLUDE_DIRECTORIES(
  ${MYSQL_INCLUDE_DIR}
  ${LIBXML2_INCLUDE_DIR}
  ${MMSEG_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}/deps/jsoncpp/include
)

AUX_SOURCE_DIRECTORY(. CUR_SRCS)
AUX_SOURCE_DIRECTORY(../ PUB_SRCS)
AUX_SOURCE_DIRECTORY(../http_frame HTTPSERVER_SRCS)

SET(LIBRARIES
  pthread
  ${LIBXML2_LIBRARIES}
  ${MYSQL_LIBRARY}
  ${MMSEG_LIBRARY}
  ${CMAKE_BINARY_DIR}/lib/libjsoncpp.so
)

ADD_EXECUTABLE(searchd ${CUR_SRCS} ${PUB_SRCS} ${HTTPSERVER_SRCS})
TARGET_LINK_LIBRARIES(searchd ${LIBRARIES})
