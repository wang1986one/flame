project(flame)

function(add_static_lib name source_list folder)
	add_library(${name} STATIC ${source_list})
	set_target_properties(${name} PROPERTIES FOLDER ${folder})
endfunction()

function(add_lib name source_list folder)
	add_library(${name} SHARED ${source_list})
	set_target_properties(${name} PROPERTIES FOLDER ${folder})
endfunction()

function(add_exe name source_list folder)
	add_executable(${name} ${source_list})
	set_target_properties(${name} PROPERTIES FOLDER ${folder})
	set_target_properties(${name} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
endfunction()

option(FLAME_ENABLE_PHYSICS "FLAME_ENABLE_PHYSICS" OFF)

set(VS_PATH "" CACHE PATH "VisualStudio Path")
set(RAPIDXML_PATH "" CACHE PATH "RapidXML Path")
set(STB_PATH "" CACHE PATH "STB Path")
set(FREETYPE_PATH "" CACHE PATH "FreeType Path")
set(SPRIV_CROSS_PATH "" CACHE PATH "SPIRV-Cross Path")
if (FLAME_ENABLE_MODEL)
set(ASSIMP_PATH "" CACHE PATH "Assimp Path")
endif()
set(OPENAL_PATH "" CACHE PATH "OpenAL Path")

add_subdirectory(source)
add_subdirectory(tests)
add_subdirectory(games)
add_subdirectory(tools)