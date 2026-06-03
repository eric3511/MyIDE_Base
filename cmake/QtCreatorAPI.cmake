if(QT_CREATOR_API_DEFINED)
  return()
endif()
set(QT_CREATOR_API_DEFINED TRUE)

set(IDE_QT_VERSION_MIN "6.0.0") # 强制最低 Qt 6

# 注意：确保你的 cmake 目录下有这个 Internal 文件
include(${CMAKE_CURRENT_LIST_DIR}/QtCreatorAPIInternal.cmake)

set(IDE_APP_PATH "${_IDE_APP_PATH}")
set(IDE_APP_TARGET "${_IDE_APP_TARGET}")
set(IDE_PLUGIN_PATH "${_IDE_PLUGIN_PATH}")
set(IDE_LIBRARY_BASE_PATH "${_IDE_LIBRARY_BASE_PATH}")
set(IDE_LIBRARY_PATH "${_IDE_LIBRARY_PATH}")
set(IDE_LIBEXEC_PATH "${_IDE_LIBEXEC_PATH}")
set(IDE_DATA_PATH "${_IDE_DATA_PATH}")
set(IDE_DOC_PATH "${_IDE_DOC_PATH}")
set(IDE_BIN_PATH "${_IDE_BIN_PATH}")

file(RELATIVE_PATH RELATIVE_PLUGIN_PATH "/${IDE_BIN_PATH}" "/${IDE_PLUGIN_PATH}")
file(RELATIVE_PATH RELATIVE_LIBEXEC_PATH "/${IDE_BIN_PATH}" "/${IDE_LIBEXEC_PATH}")
file(RELATIVE_PATH RELATIVE_DATA_PATH "/${IDE_BIN_PATH}" "/${IDE_DATA_PATH}")

list(APPEND DEFAULT_DEFINES
  RELATIVE_PLUGIN_PATH="${RELATIVE_PLUGIN_PATH}"
  RELATIVE_LIBEXEC_PATH="${RELATIVE_LIBEXEC_PATH}"
  RELATIVE_DATA_PATH="${RELATIVE_DATA_PATH}"
)

set(_THIS_MODULE_BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")

option(BUILD_PLUGINS_BY_DEFAULT "Build plugins by default." ON)
option(BUILD_EXECUTABLES_BY_DEFAULT "Build executables by default." ON)
option(BUILD_LIBRARIES_BY_DEFAULT "Build libraries by default." ON)
option(QTC_STATIC_BUILD "Builds libraries and plugins as static libraries" OFF)

function(qtc_check_default_values_for_list list_type)
  set(PLUGINS_single plugin)
  set(EXECUTABLES_single executable)
  set(LIBRARIES_single library)

  if (NOT DEFINED BUILD_${list_type})
      return()
  endif()
  set(BUILD_${list_type}_BY_DEFAULT OFF CACHE BOOL "" FORCE)
  foreach(element ${BUILD_${list_type}})
    string(TOUPPER "${${list_type}_single}_${element}" upper_element)
    set(BUILD_${upper_element} ON CACHE BOOL "Build ${${list_type}_single} ${element}.")
  endforeach()
endfunction()

qtc_check_default_values_for_list(PLUGINS)
qtc_check_default_values_for_list(EXECUTABLES)
qtc_check_default_values_for_list(LIBRARIES)

function(qtc_plugin_enabled varName name)
  if (TARGET ${name})
    set(${varName} ON PARENT_SCOPE)
  else()
    set(${varName} OFF PARENT_SCOPE)
  endif()
endfunction()

function(qtc_library_enabled varName name)
  if (TARGET ${name})
    set(${varName} ON PARENT_SCOPE)
  else()
    set(${varName} OFF PARENT_SCOPE)
  endif()
endfunction()

function(qtc_output_binary_dir varName)
  if (QTC_MERGE_BINARY_DIR)
    set(${varName} ${CMAKE_BINARY_DIR} PARENT_SCOPE)
  else()
    set(${varName} ${PROJECT_BINARY_DIR} PARENT_SCOPE)
  endif()
endfunction()

# =====================================================================
# 编译核心库 (如 Utils, ExtensionSystem)
# =====================================================================
function(add_qtc_library name)
  cmake_parse_arguments(_arg "STATIC;OBJECT;SHARED;SKIP_TRANSLATION;ALLOW_ASCII_CASTS;FEATURE_INFO;SKIP_PCH"
    "DESTINATION;COMPONENT;SOURCES_PREFIX;BUILD_DEFAULT"
    "CONDITION;DEPENDS;PUBLIC_DEPENDS;DEFINES;PUBLIC_DEFINES;INCLUDES;PUBLIC_INCLUDES;SOURCES;EXPLICIT_MOC;SKIP_AUTOMOC;EXTRA_TRANSLATIONS;PROPERTIES" ${ARGN}
  )

  set(default_defines_copy ${DEFAULT_DEFINES})
  if (_arg_ALLOW_ASCII_CASTS)
    list(REMOVE_ITEM default_defines_copy QT_NO_CAST_TO_ASCII QT_RESTRICTED_CAST_FROM_ASCII)
  endif()

  set(library_type SHARED)
  if (_arg_STATIC OR QTC_STATIC_BUILD)
    set(library_type STATIC)
  endif()

  add_library(${name} ${library_type})
  add_library(QtCreator::${name} ALIAS ${name})

  string(TOUPPER "${name}_LIBRARY" EXPORT_SYMBOL)

  extend_qtc_target(${name}
    SOURCES_PREFIX ${_arg_SOURCES_PREFIX}
    SOURCES ${_arg_SOURCES}
    INCLUDES ${_arg_INCLUDES}
    PUBLIC_INCLUDES ${_arg_PUBLIC_INCLUDES}
    DEFINES ${default_defines_copy} ${_arg_DEFINES}
    PUBLIC_DEFINES ${_arg_PUBLIC_DEFINES}
    DEPENDS ${_arg_DEPENDS}
    PUBLIC_DEPENDS ${_arg_PUBLIC_DEPENDS}
    EXPLICIT_MOC ${_arg_EXPLICIT_MOC}
    SKIP_AUTOMOC ${_arg_SKIP_AUTOMOC}
  )

  extend_qtc_target(${name} DEFINES ${EXPORT_SYMBOL})

  if (NOT _arg_SOURCES_PREFIX)
    get_filename_component(public_build_interface_dir "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
    target_include_directories(${name}
      PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
      PUBLIC "$<BUILD_INTERFACE:${public_build_interface_dir}>"
    )
  endif()

  set(_DESTINATION "${IDE_BIN_PATH}")
  if (_arg_DESTINATION)
    set(_DESTINATION "${_arg_DESTINATION}")
  endif()

  qtc_output_binary_dir(_output_binary_dir)
  set_target_properties(${name} PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    RUNTIME_OUTPUT_DIRECTORY "${_output_binary_dir}/${_DESTINATION}"
    LIBRARY_OUTPUT_DIRECTORY "${_output_binary_dir}/${IDE_LIBRARY_PATH}"
    ARCHIVE_OUTPUT_DIRECTORY "${_output_binary_dir}/${IDE_LIBRARY_PATH}"
    ${_arg_PROPERTIES}
  )

  if (NOT _arg_SKIP_PCH)
    enable_pch(${name})
  endif()

  get_target_property(have_automoc_prop ${name} AUTOMOC)
  if("${have_automoc_prop}")
    qt_extract_metatypes(${name}) # Qt 6 兼容
  endif()
endfunction()

# =====================================================================
# 编译插件 (如 Core, TextEditor)
# =====================================================================
function(add_qtc_plugin target_name)
  cmake_parse_arguments(_arg
    "SKIP_INSTALL;INTERNAL_ONLY;SKIP_TRANSLATION;EXPORT;SKIP_PCH"
    "VERSION;COMPAT_VERSION;PLUGIN_JSON_IN;PLUGIN_PATH;PLUGIN_NAME;OUTPUT_NAME;BUILD_DEFAULT;PLUGIN_CLASS"
    "CONDITION;DEPENDS;PUBLIC_DEPENDS;DEFINES;PUBLIC_DEFINES;INCLUDES;PUBLIC_INCLUDES;SOURCES;EXPLICIT_MOC;SKIP_AUTOMOC;EXTRA_TRANSLATIONS;PLUGIN_DEPENDS;PLUGIN_RECOMMENDS;PLUGIN_TEST_DEPENDS;PROPERTIES"
    ${ARGN}
  )

  set(name ${target_name})
  if (_arg_PLUGIN_NAME)
    set(name ${_arg_PLUGIN_NAME})
  endif()

  if (NOT _arg_VERSION)
    set(_arg_VERSION ${IDE_VERSION})
  endif()
  if (NOT _arg_COMPAT_VERSION)
    set(_arg_COMPAT_VERSION ${_arg_VERSION})
  endif()

  # 生成 plugin.json
  find_dependent_plugins(_DEP_PLUGINS ${_arg_PLUGIN_DEPENDS})
  set(_arg_DEPENDENCY_STRING "\"Dependencies\" : [\n")
  foreach(i IN LISTS _DEP_PLUGINS)
    set(_v ${IDE_VERSION})
    string(REPLACE "QtCreator::" "" i ${i})
    string(APPEND _arg_DEPENDENCY_STRING "        { \"Name\" : \"${i}\", \"Version\" : \"${_v}\" }")
  endforeach()
  string(REPLACE "}        {" "},\n        {" _arg_DEPENDENCY_STRING "${_arg_DEPENDENCY_STRING}")
  string(APPEND _arg_DEPENDENCY_STRING "\n    ]")
  set(IDE_PLUGIN_DEPENDENCY_STRING ${_arg_DEPENDENCY_STRING})

  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${name}.json.in")
    list(APPEND _arg_SOURCES ${name}.json.in)
    file(READ "${name}.json.in" plugin_json_in)
    # 用 @VAR@ 语法走 configure_file,避免 string(CONFIGURE) 误转义双引号
    string(REPLACE "$$QTCREATOR_VERSION" "@IDE_VERSION@" plugin_json_in "${plugin_json_in}")
    string(REPLACE "$$QTCREATOR_COMPAT_VERSION" "@IDE_VERSION_COMPAT@" plugin_json_in "${plugin_json_in}")
    string(REPLACE "$$dependencyList" "@IDE_PLUGIN_DEPENDENCY_STRING@" plugin_json_in "${plugin_json_in}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${name}.json.in.processed" "${plugin_json_in}")
    set(_ide_json_in "${CMAKE_CURRENT_BINARY_DIR}/${name}.json.in.processed")
    set(IDE_VERSION_STR "${IDE_VERSION}")
    set(IDE_VERSION_COMPAT_STR "${IDE_VERSION_COMPAT}")
    configure_file("${_ide_json_in}" "${CMAKE_CURRENT_BINARY_DIR}/${name}.json" @ONLY)
  endif()

  set(library_type SHARED)
  if (QTC_STATIC_BUILD)
    set(library_type STATIC)
  endif()

  add_library(${target_name} ${library_type} ${_arg_SOURCES})
  add_library(QtCreator::${target_name} ALIAS ${target_name})

  string(TOUPPER "${name}_LIBRARY" EXPORT_SYMBOL)

  extend_qtc_target(${target_name}
    INCLUDES ${_arg_INCLUDES}
    PUBLIC_INCLUDES ${_arg_PUBLIC_INCLUDES}
    DEFINES ${DEFAULT_DEFINES} ${_arg_DEFINES}
    PUBLIC_DEFINES ${_arg_PUBLIC_DEFINES}
    DEPENDS ${_arg_DEPENDS} ${_DEP_PLUGINS}
    PUBLIC_DEPENDS ${_arg_PUBLIC_DEPENDS}
    EXPLICIT_MOC ${_arg_EXPLICIT_MOC}
    SKIP_AUTOMOC ${_arg_SKIP_AUTOMOC}
  )

  extend_qtc_target(${target_name} DEFINES ${EXPORT_SYMBOL})

  get_filename_component(public_build_interface_dir "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
  target_include_directories(${target_name}
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    PUBLIC "$<BUILD_INTERFACE:${public_build_interface_dir}>"
  )

  set(plugin_dir "${IDE_PLUGIN_PATH}")
  if (_arg_PLUGIN_PATH)
    set(plugin_dir "${_arg_PLUGIN_PATH}")
  endif()

  if(NOT _arg_PLUGIN_CLASS)
    set(_arg_PLUGIN_CLASS ${target_name}Plugin)
  endif()

  qtc_output_binary_dir(_output_binary_dir)
  set_target_properties(${target_name} PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    LIBRARY_OUTPUT_DIRECTORY "${_output_binary_dir}/${plugin_dir}"
    RUNTIME_OUTPUT_DIRECTORY "${_output_binary_dir}/${plugin_dir}"
    ARCHIVE_OUTPUT_DIRECTORY "${_output_binary_dir}/${IDE_LIBRARY_PATH}"
    OUTPUT_NAME "${name}"
    QTC_PLUGIN_CLASS_NAME ${_arg_PLUGIN_CLASS}
    ${_arg_PROPERTIES}
  )

  if (NOT _arg_SKIP_PCH)
    enable_pch(${target_name})
  endif()
endfunction()

function(extend_qtc_plugin target_name)
  if (TARGET ${target_name})
    extend_qtc_target(${target_name} ${ARGN})
  endif()
endfunction()

function(extend_qtc_library target_name)
  if (TARGET ${target_name})
    extend_qtc_target(${target_name} ${ARGN})
  endif()
endfunction()

# =====================================================================
# 编译可执行文件 (如你的 IDE 主程序)
# =====================================================================
function(add_qtc_executable name)
  cmake_parse_arguments(_arg "SKIP_INSTALL;SKIP_TRANSLATION;ALLOW_ASCII_CASTS;SKIP_PCH;QTC_RUNNABLE"
    "DESTINATION;COMPONENT;BUILD_DEFAULT"
    "CONDITION;DEPENDS;DEFINES;INCLUDES;SOURCES;EXPLICIT_MOC;SKIP_AUTOMOC;EXTRA_TRANSLATIONS;PROPERTIES" ${ARGN})

  set(default_defines_copy ${DEFAULT_DEFINES})
  if (_arg_ALLOW_ASCII_CASTS)
    list(REMOVE_ITEM default_defines_copy QT_NO_CAST_TO_ASCII QT_RESTRICTED_CAST_FROM_ASCII)
  endif()

  set(_DESTINATION "${IDE_BIN_PATH}")
  if (_arg_DESTINATION)
    set(_DESTINATION "${_arg_DESTINATION}")
  endif()

  add_executable("${name}" ${_arg_SOURCES})

  extend_qtc_target("${name}"
    INCLUDES ${_arg_INCLUDES}
    DEFINES ${default_defines_copy} ${_arg_DEFINES}
    DEPENDS ${_arg_DEPENDS}
    EXPLICIT_MOC ${_arg_EXPLICIT_MOC}
    SKIP_AUTOMOC ${_arg_SKIP_AUTOMOC}
  )

  qtc_output_binary_dir(_output_binary_dir)
  set_target_properties("${name}" PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    RUNTIME_OUTPUT_DIRECTORY "${_output_binary_dir}/${_DESTINATION}"
    ${_arg_PROPERTIES}
  )
  if (NOT _arg_SKIP_PCH)
    enable_pch(${name})
  endif()
endfunction()

function(extend_qtc_executable name)
  if (TARGET ${name})
    extend_qtc_target(${name} ${ARGN})
  endif()
endfunction()

# =====================================================================
# 资源处理 (已修复 Qt 6 兼容性)
# =====================================================================
function(qtc_add_resources target resourceName)
  cmake_parse_arguments(rcc "" "PREFIX;LANG;BASE" "FILES;OPTIONS;CONDITION" ${ARGN})
  if(NOT TARGET ${target})
    return()
  endif()

  string(REPLACE "/" "_" resourceName ${resourceName})
  string(REPLACE "." "_" resourceName ${resourceName})

  if (rcc_BASE)
    foreach(file IN LISTS rcc_FILES)
      set(resource_file "${rcc_BASE}/${file}")
      file(TO_CMAKE_PATH ${resource_file} resource_file)
      list(APPEND resource_files ${resource_file})
    endforeach()
  else()
      set(resource_files ${rcc_FILES})
  endif()

  set(generatedResourceFile "${CMAKE_CURRENT_BINARY_DIR}/.rcc/generated_${resourceName}.qrc")
  set(generatedSourceCode "${CMAKE_CURRENT_BINARY_DIR}/.rcc/qrc_${resourceName}.cpp")

  set(qrcContents "<RCC>\n  <qresource")
  if (rcc_PREFIX)
      string(APPEND qrcContents " prefix=\"${rcc_PREFIX}\"")
  endif()
  string(APPEND qrcContents ">\n")

  set(resource_dependencies)
  foreach(file IN LISTS resource_files)
    if (NOT IS_ABSOLUTE ${file})
        set(file "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
    endif()
    string(APPEND qrcContents "    <file alias=\"${file}\">${file}</file>\n")
    list(APPEND resource_dependencies ${file})
    target_sources(${target} PRIVATE "${file}")
    set_property(SOURCE "${file}" PROPERTY HEADER_FILE_ONLY ON)
  endforeach()
  string(APPEND qrcContents "  </qresource>\n</RCC>\n")

  file(WRITE "${generatedResourceFile}.in" "${qrcContents}")
  configure_file("${generatedResourceFile}.in" "${generatedResourceFile}")

  set(rccArgs --name "${resourceName}" --output "${generatedSourceCode}" "${generatedResourceFile}")
  if(rcc_OPTIONS)
      list(APPEND rccArgs ${rcc_OPTIONS})
  endif()

  # 【关键修复】：使用 Qt6::rcc
  add_custom_command(OUTPUT "${generatedSourceCode}"
                     COMMAND Qt6::rcc ${rccArgs}
                     DEPENDS ${resource_dependencies} ${generatedResourceFile} "Qt6::rcc"
                     COMMENT "RCC ${resourceName}"
                     VERBATIM)

  target_sources(${target} PRIVATE "${generatedSourceCode}")
  set_property(SOURCE "${generatedSourceCode}" PROPERTY SKIP_AUTOGEN ON)
endfunction()
