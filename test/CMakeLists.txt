enable_testing()

find_package(OpenCV REQUIRED)
find_package(Matplot++ REQUIRED)

add_executable(fileio-test
  ${CMAKE_SOURCE_DIR}/src/fileio.cpp
  fileio_test.cpp
)

target_include_directories(fileio-test PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(fileio-test PUBLIC
  GTest::gtest_main
  ${OpenCV_LIBS}
  Matplot++::matplot
)

include(GoogleTest)
gtest_discover_tests(fileio-test)
