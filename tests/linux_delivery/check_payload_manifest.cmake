# Exercise manifest generation, positive comparison and a deliberate mismatch
# without modifying the SDK under test. Outputs stay in the supplied work dir.
cmake_minimum_required(VERSION 3.20)
if(NOT IS_DIRECTORY "${AUDIT_OUTPUT}")
  message(FATAL_ERROR "Set AUDIT_OUTPUT to an existing dedicated work directory")
endif()
set(script "${CMAKE_CURRENT_LIST_DIR}/sdk_payload_manifest.cmake")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DSDK_PREFIX=${SDK_PREFIX}" -P "${script}"
  RESULT_VARIABLE rc ERROR_VARIABLE manifest TIMEOUT 30)
if(NOT "${rc}" STREQUAL "0")
  message(FATAL_ERROR "Manifest generation failed: ${manifest}")
endif()
file(WRITE "${AUDIT_OUTPUT}/sdk-payload.txt" "${manifest}")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DSDK_PREFIX=${SDK_PREFIX}"
  "-DEXPECTED_MANIFEST=${AUDIT_OUTPUT}/sdk-payload.txt" -P "${script}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 30)
if(NOT "${rc}" STREQUAL "0" OR NOT output MATCHES "matches the recorded manifest")
  message(FATAL_ERROR "Positive verification failed: ${output}${error}")
endif()

string(REGEX REPLACE "\n[a-f0-9]+  (lib[^\n]*/libpcinsight\\.a)\n"
  "\n0000000000000000000000000000000000000000000000000000000000000000  \\1\n" altered "${manifest}")
if(altered STREQUAL manifest)
  message(FATAL_ERROR "Expected a library row to alter")
endif()
file(WRITE "${AUDIT_OUTPUT}/sdk-payload-altered.txt" "${altered}")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DSDK_PREFIX=${SDK_PREFIX}"
  "-DEXPECTED_MANIFEST=${AUDIT_OUTPUT}/sdk-payload-altered.txt" -P "${script}"
  RESULT_VARIABLE rc ERROR_VARIABLE error TIMEOUT 30)
if(NOT "${rc}" STREQUAL "1" OR NOT error MATCHES "Payload mismatch: lib[^\n]*/libpcinsight\\.a")
  message(FATAL_ERROR "Expected an archive mismatch: ${rc}: ${error}")
endif()
message(STATUS "SDK manifest verified; altered archive digest rejected")
