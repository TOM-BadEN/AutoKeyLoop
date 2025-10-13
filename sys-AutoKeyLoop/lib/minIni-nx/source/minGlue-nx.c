#include "minGlue.h"

#if MININI_USE_NX
#include "minGlue-nx.h"
#include <string.h>

static void fix_nro_path(char* path, size_t len) {
    // hbmenu prefixes paths with sdmc: which fsFsOpenFile won't like
    if (!strncmp(path, "sdmc:/", 6)) {
        memmove(path, path + 5, len-5);
    }
}

static bool ini_open(const char* filename, struct NxFile* nxfile, u32 mode) {
    Result rc;
    char filename_buf[FS_MAX_PATH];

    if (R_FAILED(rc = fsOpenSdCardFileSystem(&nxfile->system))) {
        return false;
    }

    strcpy(filename_buf, filename);
    fix_nro_path(filename_buf, sizeof(filename_buf));

    if (R_FAILED(rc = fsFsOpenFile(&nxfile->system, filename_buf, mode, &nxfile->file))) {
        if (mode & FsOpenMode_Write) {
            if (R_FAILED(rc = fsFsCreateFile(&nxfile->system, filename_buf, 0, 0))) {
                fsFsClose(&nxfile->system);
                return false;
            } else {
                if (R_FAILED(rc = fsFsOpenFile(&nxfile->system, filename_buf, mode, &nxfile->file))) {
                    fsFsClose(&nxfile->system);
                    return false;
                }
            }
        } else {
            fsFsClose(&nxfile->system);
            return false;
        }
    }

    nxfile->offset = 0;
    return true;
}

bool ini_openread_nx(const char* filename, struct NxFile* nxfile) {
    return ini_open(filename, nxfile, FsOpenMode_Read);
}

bool ini_openwrite_nx(const char* filename, struct NxFile* nxfile) {
    return ini_open(filename, nxfile, FsOpenMode_Write|FsOpenMode_Append);
}

bool ini_openrewrite_nx(const char* filename, struct NxFile* nxfile) {
    return ini_open(filename, nxfile, FsOpenMode_Read|FsOpenMode_Write|FsOpenMode_Append);
}

bool ini_close_nx(struct NxFile* nxfile) {
    fsFileClose(&nxfile->file);
    fsFsClose(&nxfile->system);
    return true;
}

bool ini_read_nx(char* buffer, u64 size, struct NxFile* nxfile) {
    u64 bytes_read;
    if (R_FAILED(fsFileRead(&nxfile->file, nxfile->offset, buffer, size, FsReadOption_None, &bytes_read))) {
        return false;
    }

    if (!bytes_read) {
        return false;
    }

    char *eol;
    if ((eol = strchr(buffer, '\n')) == NULL) {
        eol = strchr(buffer, '\r');
    }

    if (eol != NULL) {
        *++eol = '\0';
        bytes_read = eol - buffer;
    }

    nxfile->offset += bytes_read;
    return true;
}

bool ini_write_nx(const char* buffer, struct NxFile* nxfile) {
    const size_t size = strlen(buffer);
    if (R_FAILED(fsFileWrite(&nxfile->file, nxfile->offset, buffer, size, FsWriteOption_None))) {
        return false;
    }
    nxfile->offset += size;
    return true;
}

bool ini_tell_nx(struct NxFile* nxfile, s64* pos) {
    *pos = nxfile->offset;
    return true;
}

bool ini_seek_nx(struct NxFile* nxfile, s64* pos) {
    nxfile->offset = *pos;
    return true;
}

bool ini_rename_nx(const char* src, const char* dst) {
    Result rc;
    FsFileSystem fs;
    char src_buf[FS_MAX_PATH];
    char dst_buf[FS_MAX_PATH];

    if (R_FAILED(rc = fsOpenSdCardFileSystem(&fs))) {
        return false;
    }

    strcpy(src_buf, src);
    strcpy(dst_buf, dst);
    rc = fsFsRenameFile(&fs, src_buf, dst_buf);
    fsFsClose(&fs);
    return R_SUCCEEDED(rc);
}

bool ini_remove_nx(const char* filename) {
    Result rc;
    FsFileSystem fs;
    char filename_buf[FS_MAX_PATH];

    if (R_FAILED(rc = fsOpenSdCardFileSystem(&fs))) {
        return false;
    }

    strcpy(filename_buf, filename);
    rc = fsFsDeleteFile(&fs, filename_buf);
    fsFsClose(&fs);
    return R_SUCCEEDED(rc);
}

#endif // MININI_USE_NX
