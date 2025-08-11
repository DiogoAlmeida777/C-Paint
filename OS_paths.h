#ifndef OS_PATHS_H
#define OS_PATHS_H

#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

const char *get_pictures_path(void);

#endif