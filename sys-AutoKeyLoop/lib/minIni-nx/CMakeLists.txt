cmake_minimum_required(VERSION 3.13)

project(minIni LANGUAGES C)

option(MININI_USE_STDIO ON "enable stdio")
option(MININI_USE_NX ON "enable native nx support")
option(MININI_USE_FLOAT ON "enable floating support")

if (NOT DEFINED MININI_LIB_NAME)
    set(MININI_LIB_NAME minIni)
endif()

add_library(${MININI_LIB_NAME}
    source/minGlue-nx.c
    source/minGlue.c
    source/minIni.c
)

target_include_directories(${MININI_LIB_NAME} PUBLIC include)

set_target_properties(${MININI_LIB_NAME} PROPERTIES
    C_STANDARD 99
)

target_compile_definitions(${MININI_LIB_NAME} PRIVATE
    MININI_USE_STDIO=$<BOOL:${MININI_USE_STDIO}>
    MININI_USE_NX=$<BOOL:${MININI_USE_NX}>
    MININI_USE_FLOAT=$<BOOL:${MININI_USE_FLOAT}>
)
