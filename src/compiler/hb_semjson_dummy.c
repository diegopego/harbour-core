#include "hb_semjson.h"

int main(void)
{
    hb_semjson_write_file(".harboursems/project.json",
                          "{ \"name\": \"foo\", \"version\": \"0.0.1\" }\n");
    return 0;
}
