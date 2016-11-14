PROJECT(reader_test)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

INCLUDE_DIRECTORIES(
  /usr/include/mysql
  ${MMSEG_PREFIX}/include/mmseg
)

AUX_SOURCE_DIRECTORY(./ CUR_SRCS)
AUX_SOURCE_DIRECTORY(../ PUR_SRCS)

SET(LIBRARIES
  ${MMSEG_PREFIX}/lib/libmmseg.a
  /usr/lib/x86_64-linux-gnu/libmysqlclient.so
)

ADD_EXECUTABLE(reader_test ${CUR_SRCS} ${PUR_SRCS})
TARGET_LINK_LIBRARIES(reader_test ${LIBRARIES})
