#include "hb_semjson.h"
#include <stdio.h>
#include <sys/stat.h>

// ensure directory exists (only one-level, e.g. ".harboursems/")
static void hb_semjson_mkdirs(const char *path)
{
    (void)path; // ignore for now, keep stub
    mkdir(".harboursems", 0777);
}

int hb_semjson_write_file(const char *path, const char *content)
{
    hb_semjson_mkdirs(path);
    FILE *f = fopen(path, "w");
    if (!f)
        return 0;
    fputs(content, f);
    fclose(f);
    return 1;
}
