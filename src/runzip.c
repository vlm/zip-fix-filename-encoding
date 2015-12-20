#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sysexits.h>
#include <errno.h>
#include <iconv.h>

#include <zip.h>
#include "UTF8String.h"

int
main (int ac, char **av) {
    char ebuf[256];
    struct zip *z;
    int fidx, fmax;
    int e;

    if(ac != 2) {
        fprintf(stderr,
          "Windows-1251 -> UTF-8 filename recoder inside ZIP archives\n"
          "Copyright (c) 2007, 2009, 2015 Lev Walkin <vlm@lionet.info>\n"
          "libzip by Dieter Baron <dillo@giga.or.at> and Thomas Klausner <tk@giga.or.at>\n"
          "Usage: %s <file.zip>\n", basename(av[0]));
        exit(EX_USAGE);
    }

    z = zip_open(av[1], 0, &e);
    if(!z) {
        zip_error_to_str(ebuf, sizeof ebuf, e, errno);
        fprintf(stderr, "%s: %s\n", av[1], ebuf);
        exit(EX_NOINPUT);
    }

    fmax = zip_get_num_files(z);
    printf("%s contains %d file%s\n", av[1], fmax, fmax == 1 ? "" : "s");
    for(fidx = 0; fidx < fmax; fidx++) {
        char *fName;
        char *nName;
        char *ip;
        char *op;
        size_t fSize, nSize;
        iconv_t ic;
        int ir, ie;

        fName = strdup(zip_get_name(z, fidx, 0));
        if(!fName) continue;

        fSize = strlen(fName);
        if(UTF8String_length(fName, fSize) < 0) {
            printf("No recode necessary for \"%s\"\n", fName);
            free(fName);
            continue;
        }

        nSize = 4 * (fSize + 1);
        nName = malloc(nSize);
        ip = fName; op = nName;

        ic = iconv_open("UTF-8", "windows-1251");
        ir = iconv(ic, &ip, &fSize, &op, &nSize);
        ie = errno;
        iconv_close(ic);
        if(ir == -1) {
            printf("Failed to recode \"%s\": \"%s\"\n",
                fName, strerror(ie));
            free(fName);
            continue;
        }
        *op = 0;

        if(zip_rename(z, fidx, nName)) {
            printf("Failed to rename \"%s\" -> \"%s\": %s\n",
                fName, nName, zip_strerror(z));
        } else {
            printf("Renamed \"%s\" -> \"%s\"\n", fName, nName);
        }
        free(fName);
    }

    zip_close(z);

    return 0;
}
