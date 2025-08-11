#include "OS_paths.h"
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <knownfolders.h>
#include <wchar.h>

const GUID FOLDERID_Pictures = {0x33E28130, 0x4E1E, 0x4676,
                                {0x83, 0x5A, 0x98, 0x39, 0x5C, 0x3B, 0xC3, 0xBB}};


const char *get_pictures_path(void) {
    static char path[1024];
    PWSTR widePath = NULL;
    if (SUCCEEDED(SHGetKnownFolderPath(&FOLDERID_Pictures, 0, NULL, &widePath))) {
        wcstombs(path, widePath, sizeof(path));
        CoTaskMemFree(widePath);
        return path;
    }
    return NULL;
}
#else
//NEED TO TEST
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
char *get_pictures_path(void) {
    const char *xdg = getenv("XDG_PICTURES_DIR");
    const char *home = getenv("HOME");
    static char path[1024];
    if (xdg) return (char*)xdg;
    if (home) {
        snprintf(path, sizeof(path), "%s/Pictures", home);
        return path;
    }
    return NULL;
}
#endif