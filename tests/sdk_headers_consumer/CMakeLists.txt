cmake_minimum_required(VERSION 3.20)
project(pcinsight_sdk_headers_consumer LANGUAGES CXX)
find_package(pcinsight 0.2.0 EXACT CONFIG REQUIRED)
get_target_property(sdk_includes pcinsight::pcinsight INTERFACE_INCLUDE_DIRECTORIES)
find_file(marker sdk_headers.hpp PATHS ${sdk_includes} PATH_SUFFIXES pcinsight NO_DEFAULT_PATH REQUIRED)
file(READ "${marker}" marker_text)
string(REGEX MATCH "pcinsight_require_headers_([a-f0-9]+)" marker_symbol "${marker_text}")
set(digest "${CMAKE_MATCH_1}")
string(LENGTH "${digest}" digest_length)
if(NOT digest_length EQUAL 64)
  message(FATAL_ERROR "Invalid SDK header marker")
endif()

# The positive and negative probes use the same source/target and flags. The
# alternate marker is deliberately synthetic; it must compile but not link.
set(CMAKE_TRY_COMPILE_CONFIGURATION Release)
set(CMAKE_TRY_COMPILE_TARGET_TYPE EXECUTABLE)
try_compile(matching "${CMAKE_CURRENT_BINARY_DIR}/matching" "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
  LINK_LIBRARIES pcinsight::pcinsight OUTPUT_VARIABLE matching_log)
if(NOT matching)
  message(FATAL_ERROR "Matching SDK marker did not link:\n${matching_log}")
endif()
string(SHA256 alternate "pcinsight deliberate mismatch: ${digest}")
string(REPLACE "${digest}" "${alternate}" incompatible_header "${marker_text}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/alternate/pcinsight/sdk_headers.hpp" "${incompatible_header}")
set(alternate_include "${CMAKE_CURRENT_BINARY_DIR}/alternate")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
try_compile(mismatch_compiles "${CMAKE_CURRENT_BINARY_DIR}/mismatch-compile" "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
  CMAKE_FLAGS "-DINCLUDE_DIRECTORIES:STRING=${alternate_include}" LINK_LIBRARIES pcinsight::pcinsight
  OUTPUT_VARIABLE mismatch_compile_log)
if(NOT mismatch_compiles)
  message(FATAL_ERROR "The alternate fixture must compile:\n${mismatch_compile_log}")
endif()
set(CMAKE_TRY_COMPILE_TARGET_TYPE EXECUTABLE)
try_compile(mismatch_links "${CMAKE_CURRENT_BINARY_DIR}/mismatch-link" "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
  CMAKE_FLAGS "-DINCLUDE_DIRECTORIES:STRING=${alternate_include}" LINK_LIBRARIES pcinsight::pcinsight
  OUTPUT_VARIABLE mismatch_link_log)
if(mismatch_links OR NOT mismatch_link_log MATCHES "pcinsight_require_headers_${alternate}")
  message(FATAL_ERROR "Expected link rejection for the alternate fingerprint:\n${mismatch_link_log}")
endif()
message(STATUS "SDK marker: matched links; alternate compiles but fails to link on its marker symbol")

# Exercise the two-translation-unit case: a correctly marked caller must not
# hide a reachable check compiled against a different header in its peer.
foreach(pair IN ITEMS matching mixed)
  set(peer_header "${marker}")
  if(pair STREQUAL "mixed")
    set(peer_header "${alternate_include}/pcinsight/sdk_headers.hpp")
  endif()
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${pair}_peer.cpp"
    "#include \"${peer_header}\"\nvoid check_peer() { pcinsight::require_matching_sdk_headers(); }\n")
  try_compile(${pair}_pair_links "${CMAKE_CURRENT_BINARY_DIR}/${pair}-pair"
    SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/mixed_main.cpp" "${CMAKE_CURRENT_BINARY_DIR}/${pair}_peer.cpp"
    LINK_LIBRARIES pcinsight::pcinsight OUTPUT_VARIABLE pair_log)
  if(pair STREQUAL "matching" AND NOT matching_pair_links)
    message(FATAL_ERROR "Two matching translation units did not link:\n${pair_log}")
  elseif(pair STREQUAL "mixed" AND (mixed_pair_links OR NOT pair_log MATCHES "pcinsight_require_headers_${alternate}"))
    message(FATAL_ERROR "The matching caller must not mask the peer marker mismatch:\n${pair_log}")
  endif()
endforeach()
message(STATUS "SDK marker: matching translation-unit pair links; mixed pair rejects the peer marker")

add_executable(sdk-headers-consumer main.cpp)
target_link_libraries(sdk-headers-consumer PRIVATE pcinsight::pcinsight)
enable_testing()
add_test(NAME sdk-headers-consumer COMMAND sdk-headers-consumer)
set_tests_properties(sdk-headers-consumer PROPERTIES TIMEOUT 10)
