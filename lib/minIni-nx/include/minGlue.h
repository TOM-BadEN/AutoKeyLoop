#pragma once

#if defined __cplusplus
extern "C" {
#endif

#if !defined(MININI_USE_NX) && !defined(MININI_USE_STDIO)
    #define MININI_USE_NX 0
    #define MININI_USE_STDIO 1
#endif

#if !defined(MININI_USE_NX)
    #define MININI_USE_NX 0
#endif

#if !defined(MININI_USE_STDIO)
    #define MININI_USE_STDIO 0
#endif

#if !defined(MININI_USE_FLOAT)
    #define MININI_USE_FLOAT 1
#endif

#if !defined(INI_BUFFERSIZE)
    #define INI_BUFFERSIZE 0x301
#endif

#if MININI_USE_NX && MININI_USE_STDIO
#include <switch.h>
#include <stdio.h>
#include <stdbool.h>
#include "minGlue-nx.h"
#include "minGlue-stdio.h"

struct MiniGlue {
    struct NxFile nxfile;
    FILE* stdio_file;
    bool is_nx;
};

#define INI_FILETYPE struct MiniGlue
#define INI_FILEPOS s64
#define INI_OPENREWRITE
#define INI_REMOVE

bool ini_openread(const char* filename, struct MiniGlue* glue);
bool ini_openwrite(const char* filename, struct MiniGlue* glue);
bool ini_openrewrite(const char* filename, struct MiniGlue* glue);
bool ini_close(struct MiniGlue* glue);
bool ini_read(char* buffer, u64 size, struct MiniGlue* glue);
bool ini_write(const char* buffer, struct MiniGlue* glue);
bool ini_tell(struct MiniGlue* glue, s64* pos);
bool ini_seek(struct MiniGlue* glue, s64* pos);
bool ini_rename(const char* src, const char* dst);
bool ini_remove(const char* filename);

#elif MININI_USE_NX
#include <switch.h>
#include "minGlue-nx.h"

#define INI_FILETYPE struct NxFile
#define INI_FILEPOS s64
#define INI_OPENREWRITE
#define INI_REMOVE

#define ini_openread ini_openread_nx
#define ini_openwrite ini_openwrite_nx
#define ini_openrewrite ini_openrewrite_nx
#define ini_close ini_close_nx
#define ini_read ini_read_nx
#define ini_write ini_write_nx
#define ini_rename ini_rename_nx
#define ini_remove ini_remove_nx
#define ini_tell ini_tell_nx
#define ini_seek ini_seek_nx

#else
#include "minGlue-stdio.h"
#include <stdio.h>

#define INI_FILETYPE FILE*
#define INI_FILEPOS long
#define INI_OPENREWRITE
#define INI_REMOVE

#define ini_openread ini_openread_stdio
#define ini_openwrite ini_openwrite_stdio
#define ini_openrewrite ini_openrewrite_stdio
#define ini_close ini_close_stdio
#define ini_read ini_read_stdio
#define ini_write ini_write_stdio
#define ini_rename ini_rename_stdio
#define ini_remove ini_remove_stdio
#define ini_tell ini_tell_stdio
#define ini_seek ini_seek_stdio
#endif

#if MININI_USE_FLOAT
#include <stdio.h>
#include <stdlib.h>

/* for floating-point support, define additional types and functions */
#define INI_REAL                        float
#define ini_ftoa(string,value)          sprintf((string),"%f",(value))
#define ini_atof(string)                (INI_REAL)strtod((string),NULL)
#endif

#if defined __cplusplus
}
#endif
