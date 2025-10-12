/*  Glue functions for the minIni library, based on the C/C++ stdio library
 *
 *  Or better said: this file contains macros that maps the function interface
 *  used by minIni to the standard C/C++ file I/O functions.
 *
 *  By CompuPhase, 2008-2014
 *  This "glue file" is in the public domain. It is distributed without
 *  warranties or conditions of any kind, either express or implied.
 */

/* map required file I/O types and functions to the standard C library */
#pragma once

#include <stdio.h>

#if defined __cplusplus
extern "C" {
#endif

#define ini_openread_stdio(filename,file)     ((*(file) = fopen((filename),"rb")) != NULL)
#define ini_openwrite_stdio(filename,file)    ((*(file) = fopen((filename),"wb")) != NULL)
#define ini_openrewrite_stdio(filename,file)  ((*(file) = fopen((filename),"r+b")) != NULL)
#define ini_close_stdio(file)                 (fclose(*(file)) == 0)
#define ini_read_stdio(buffer,size,file)      (fgets((buffer),(size),*(file)) != NULL)
#define ini_write_stdio(buffer,file)          (fputs((buffer),*(file)) >= 0)
#define ini_rename_stdio(source,dest)         (rename((source), (dest)) == 0)
#define ini_remove_stdio(filename)            (remove(filename) == 0)
#define ini_tell_stdio(file,pos)              (*(pos) = ftell(*(file)))
#define ini_seek_stdio(file,pos)              (fseek(*(file), *(pos), SEEK_SET) == 0)

#if defined __cplusplus
}
#endif
