#
# MegaMol™ spout_sender Plugin
# Copyright 2017, by MegaMol Team
# Alle Rechte vorbehalten. All rights reserved.
#
string(TOUPPER spout_sender EXPORT_NAME)
option(BUILD_${EXPORT_NAME}_PLUGIN "Option to build spout_sender" ON)

if(BUILD_${EXPORT_NAME}_PLUGIN)
  project(spout_sender)

  # Check option dependencies: First list item is always the current build option,
  # afterwards list the dependencies.
  set(DEP_LIST "${DEP_LIST};BUILD_${EXPORT_NAME}_PLUGIN BUILD_CORE BUILD_VISLIB" CACHE INTERNAL "")

  file(GLOB_RECURSE public_header_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "include/*.h")
  file(GLOB_RECURSE source_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "src/*.cpp")
  file(GLOB_RECURSE header_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "src/*.h")
  #file(GLOB_RECURSE shader_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "Shaders/*")
  
  file(GLOB_RECURSE spout_source_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "SpoutSDK/*.cpp")
  file(GLOB_RECURSE spout_header_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "SpoutSDK/*.h")

  add_subdirectory(oscpack)

  # Target definition
  add_library(${PROJECT_NAME} SHARED ${public_header_files} ${header_files} ${source_files} ${spout_source_files} ${spout_header_files})# ${oscpack_source_files} ${oscpack_header_files})
  set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".mmplg")
  target_compile_definitions(${PROJECT_NAME} PRIVATE ${EXPORT_NAME}_EXPORTS)
  target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>  "include" "src" "SpoutSDK")
  target_link_libraries(${PROJECT_NAME} PRIVATE core oscpack)

  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)


  # Installation rules for generated files
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION "include")
  #install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/ DESTINATION "share/shaders")
  if(WIN32)
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "bin")
    install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION "lib")
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION bin OPTIONAL)
  else()
    install(TARGETS ${PROJECT_NAME} DESTINATION "lib" EXPORT ${PROJECT_NAME}-target)
  endif()

  # Grouping in Visual Studio
  set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER plugins)
  source_group("Public Header Files" FILES ${public_header_files})
  source_group("Header Files" FILES ${header_files})
  source_group("Source Files" FILES ${source_files})
  source_group("SpoutSDK Files" FILES ${spout_source_files} ${spout_header_files})
  #source_group("Shaders" FILES ${shader_files})
endif()
