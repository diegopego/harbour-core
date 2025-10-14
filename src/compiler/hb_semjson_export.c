#include "hbapi.h"
#include "hbcomp.h"
#include "hb_semjson_export.h"
#include "hb_semjson_fs.c" /* mkdirp */
#include <stdio.h>
#include <string.h>

/* cria caminho de saída: outdir/<modulo>.ast.json */
static void semjson_outpath(PHB_COMP pComp, char *buf, size_t bufsz)
{
    const char *src = pComp->szFile ? pComp->szFile : pComp->currModule;
    if (!src)
        src = "module";

    const char *base = strrchr(src, '/');
#if defined(_WIN32)
    const char *base2 = strrchr(src, '\\');
    if (base2 && (!base || base2 > base))
        base = base2;
#endif
    if (base)
        src = base + 1;

    snprintf(buf, bufsz, "%s/%s.ast.json",
             pComp->szASTDir[0] ? pComp->szASTDir : ".harboursems",
             src);
}

/* ponto de entrada chamado no fim da compilação */
void hb_compEmitASTIfEnabled(HB_COMP_DECL)
{
    char outpath[512];
    semjson_outpath(pComp, outpath, sizeof(outpath));
    FILE *out = fopen(outpath, "w");
    if (out)
    {
        fprintf(out,
                "  \"name\": \"%s\",\n"
                "  \"line\": -1\n", /* linha indisponível neste estágio */
                pComp->currModule ? pComp->currModule : "<anon>");
        fclose(out);
    }
}
