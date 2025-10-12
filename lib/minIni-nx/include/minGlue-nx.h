#pragma once

#if defined __cplusplus
extern "C" {
#endif

#include <switch.h>

struct NxFile {
    FsFile file;
    FsFileSystem system;
    s64 offset;
};

bool ini_openread_nx(const char* filename, struct NxFile* nxfile);
bool ini_openwrite_nx(const char* filename, struct NxFile* nxfile);
bool ini_openrewrite_nx(const char* filename, struct NxFile* nxfile);
bool ini_close_nx(struct NxFile* nxfile);
bool ini_read_nx(char* buffer, u64 size, struct NxFile* nxfile);
bool ini_write_nx(const char* buffer, struct NxFile* nxfile);
bool ini_tell_nx(struct NxFile* nxfile, s64* pos);
bool ini_seek_nx(struct NxFile* nxfile, s64* pos);
bool ini_rename_nx(const char* src, const char* dst);
bool ini_remove_nx(const char* filename);

#if defined __cplusplus
} // extern "C" {
#endif
