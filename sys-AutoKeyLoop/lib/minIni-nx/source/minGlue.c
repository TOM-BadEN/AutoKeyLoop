#include "minGlue.h"

#if MININI_USE_NX && MININI_USE_STDIO
#include <string.h>

static bool is_romfs(const char* filename) {
    return !strncmp(filename, "romfs:/", 7);
}

bool ini_openread(const char* filename, struct MiniGlue* glue) {
    if (is_romfs(filename)) {
        glue->is_nx = false;
        return ini_openread_stdio(filename, &glue->stdio_file);
    }
    glue->is_nx = true;
    return ini_openread_nx(filename, &glue->nxfile);
}

bool ini_openwrite(const char* filename, struct MiniGlue* glue) {
    if (is_romfs(filename)) {
        glue->is_nx = false;
        return ini_openwrite_stdio(filename, &glue->stdio_file);
    }
    glue->is_nx = true;
    return ini_openwrite_nx(filename, &glue->nxfile);
}

bool ini_openrewrite(const char* filename, struct MiniGlue* glue) {
    if (is_romfs(filename)) {
        glue->is_nx = false;
        return ini_openrewrite_stdio(filename, &glue->stdio_file);
    }
    glue->is_nx = true;
    return ini_openrewrite_nx(filename, &glue->nxfile);
}

bool ini_close(struct MiniGlue* glue) {
    if (!glue->is_nx) {
        return ini_close_stdio(&glue->stdio_file);
    }
    return ini_close_nx(&glue->nxfile);
}

bool ini_read(char* buffer, u64 size, struct MiniGlue* glue) {
    if (!glue->is_nx) {
        return ini_read_stdio(buffer, size, &glue->stdio_file);
    }
    return ini_read_nx(buffer, size, &glue->nxfile);
}

bool ini_write(const char* buffer, struct MiniGlue* glue) {
    if (!glue->is_nx) {
        return ini_write_stdio(buffer, &glue->stdio_file);
    }
    return ini_write_nx(buffer, &glue->nxfile);
}

bool ini_tell(struct MiniGlue* glue, s64* pos) {
    if (!glue->is_nx) {
        return ini_tell_stdio(&glue->stdio_file, pos);
    }
    return ini_tell_nx(&glue->nxfile, pos);
}

bool ini_seek(struct MiniGlue* glue, s64* pos) {
    if (!glue->is_nx) {
        return ini_seek_stdio(&glue->stdio_file, pos);
    }
    return ini_seek_nx(&glue->nxfile, pos);
}

bool ini_rename(const char* src, const char* dst) {
    if (is_romfs(src) || is_romfs(dst)) {
        return ini_rename_stdio(src, dst);
    }
    return ini_rename_nx(src, dst);
}

bool ini_remove(const char* filename) {
    if (is_romfs(filename)) {
        return ini_remove(filename);
    }
    return ini_remove_nx(filename);
}

#endif // MININI_USE_NX && MININI_USE_STDIO
