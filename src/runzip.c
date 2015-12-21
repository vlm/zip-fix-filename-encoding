#include <unistd.h> /* getopt(3) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <libgen.h> /* basename(3) */
#include <sysexits.h>
#include <errno.h>
#include <iconv.h>

#include <zip.h>

/*
 * Convert a string of the given length from (from_enc)
 * to (to_enc) character encoding.
 */
static char *convert(char *str, size_t len, const char *from_enc, const char *to_enc) {

    size_t outlen = 6 * len;
    char *outstr = malloc(outlen + 1);
    char *op = outstr;

    int ir, ie;
    iconv_t ic = iconv_open(to_enc, from_enc);
    if(ic == (iconv_t)-1) {
        ie = errno;
        free(outstr);
        outstr = NULL;
    } else {
        ir = iconv(ic, &str, &len, &op, &outlen);
        ie = errno;
        iconv_close(ic);
        if(ir == -1) {
            free(outstr);
            outstr = NULL;
        } else {
            *op = 0;
        }
    }

    errno = ie;
    return outstr;
}

/*
 * A structure describing the character frequencies found in a string.
 */
struct char_frequencies {
    const char *encoding;
    size_t characters_seen;
    size_t frequency[256];
};

/*
 * Detect the "cyrillic-factor" - the greater the factor is, the more cyrillic
 * the string appears to be, for a given character encoding.
 * From http://www.sttmedia.com/characterfrequency-cyrillic we know the
 * basic Russian letters frequency; we interpolate some Ukrainian as well.
 */
static double
cyrillic_factor(const struct char_frequencies *freq) {
    static double scale[256] = {
        [32 ... 126] = 0.001,
        [207] = 11.07,  /* О */
        [197] = 8.50,   /* Е */
        [193] = 7.50,   /* А */
        [201] = 7.09,   /* И */
        [206] = 6.70,   /* Н */
        [212] = 5.97,   /* Т */
        [211] = 4.97,   /* С */
        [204] = 4.96,   /* Л */
        [215] = 4.33,   /* В */
        [210] = 4.33,   /* Р */
        [203] = 3.30,   /* К */
        [237] = 3.10,   /* М */
        [196] = 3.09,   /* Д */
        [208] = 2.47,   /* П */
        [217] = 2.36,   /* Ы */
        [213] = 2.22,   /* У */
        [194] = 2.01,   /* Б */
        [209] = 1.96,   /* Я */
        [216] = 1.84,   /* Ь */
        [199] = 1.72,   /* Г */
        [218] = 1.48,   /* З */
        [222] = 1.40,   /* Ч */
        [202] = 1.21,   /* Й */
        [214] = 1.01,   /* Ж */
        [200] = 0.95,   /* Х */
        [219] = 0.72,   /* Ш */
        [192] = 0.47,   /* Ю */
        [195] = 0.39,   /* Ц */
        [220] = 0.35,   /* Э */
        [221] = 0.30,   /* Щ */
        [198] = 0.21,   /* Ф */
        [163] = 0.20,   /* Ё */
        [223] = 0.02,   /* Ъ */
        /* Ukrainian; estimates from http://megamozg.ru/post/2336/ */
        [164] = 0.3,    /* Є */
        [166] = 5.0,    /* І */
        [167] = 0.3,    /* Ї */
        [173] = 0.01,   /* Ґ */
    };

    double factor = 0.0;
    for(int i = 0; i < 256; i++) {
        /*
         * Lowercase KOI8-R/KOI8-U character.
         */
        unsigned char ch = i >= 225 ? (i-32) : i;
        if (ch == 179) ch -= 16;    /* ё */
        if (ch == 180 || ch == 182 || ch == 183 || ch == 189) ch -= 16;    /* є, і, ї, ґ */

        double f = freq->frequency[i];
        if(f == 0.0) f = -10;   /* Penalty for out of range characters. */
        factor += freq->frequency[i] * scale[ch];
    }

    return factor;
}

/*
 * Compare the frequency tables to detect whether one of them is more cyrillic.
 */
static int
compare_cyrillic_factors(const void *ap, const void *bp) {
    const struct char_frequencies *a = ap;
    const struct char_frequencies *b = bp;

    double rfa = cyrillic_factor(a);
    double rfb = cyrillic_factor(b);
    if(rfa < rfb) return 1;
    else if (rfa > rfb) return -1;
    else return 0;
}

/*
 * Given a string, this function detects the best Cyrillic encoding it might
 * appear in. It does it by converting the string from various source encodings
 * into KOI8-U first (-U is somewhat a superset of KOI8-R),
 * building the character frequency tables, and then comparing these tables.
 * The encoding which produces the most cyrillic-like frequency profile wins.
 */
static const char *detect_cyrillic_encoding(char *str, size_t len, int verbose) {
#define NUM_ENCODINGS 6
    char *try_encodings[NUM_ENCODINGS] = {
#if __APPLE__
        "UTF-8-MAC", "UTF-8",
#else
        "UTF-8", "UTF-8-MAC",
#endif
        "Windows-1251", "CP866", "KOI8-R", "KOI8-U" };
    struct char_frequencies freqs[NUM_ENCODINGS];
    int ei;

    for(ei = 0; ei < NUM_ENCODINGS; ei++) {
        memset(&freqs[ei], 0, sizeof(freqs[0]));
        freqs[ei].encoding = try_encodings[ei];
        char *out = convert(str, len, try_encodings[ei], "KOI8-U");
        if(!out) continue;  /* Could not convert */
        const char *p;
        /* Fill out a frequency table */
        for(p = out; *p; p++) {
            freqs[ei].frequency[*(unsigned char *)p]++;
            freqs[ei].characters_seen++;
        }
        free(out);
    }

    qsort(freqs, NUM_ENCODINGS, sizeof(freqs[0]), compare_cyrillic_factors);

    if(verbose) {
        printf("For \"%s\":\n", str);
        for(ei = 0; ei < NUM_ENCODINGS; ei++) {
            printf("\t%s factor %f (%ld)\n", freqs[ei].encoding,
                        cyrillic_factor(&freqs[ei]),
                        (long)freqs[ei].characters_seen);
        }
    }

    return freqs[0].encoding;
}

static int fix_cyrillic_filenames(const char *zipfile, int dry_run, const char *suggested_source_encoding, const char *target_encoding, int verbose) {
    char ebuf[256];
    struct zip *z;
    int fidx, fmax;
    int e;

    z = zip_open(zipfile, 0, &e);
    if(!z) {
        zip_error_to_str(ebuf, sizeof ebuf, e, errno);
        fprintf(stderr, "%s: %s\n", zipfile, ebuf);
        return -1;
    }

    fmax = zip_get_num_files(z);
    printf("%s contains %d file%s\n", zipfile, fmax, fmax == 1 ? "" : "s");
    for(fidx = 0; fidx < fmax; fidx++) {
        char *fName;
        size_t fSize;

        fName = strdup(zip_get_name(z, fidx, 0));
        if(!fName) continue;
        fSize = strlen(fName);

        const char *source_encoding = suggested_source_encoding;
        if(source_encoding == NULL)
            source_encoding = detect_cyrillic_encoding(fName, fSize, verbose);

        if(strcasecmp(source_encoding, target_encoding) == 0) {
            printf("  %s: OK\n", fName);
            free(fName);
            continue;
        } else if(verbose) {
            printf("  Converting \"%s\" (%s -> %s)\n",
                fName, source_encoding, target_encoding);
        }

        char *nName = convert(fName, fSize, source_encoding, target_encoding);
        if(!nName) {
            printf("  Failed to recode \"%s\" (%s -> %s): \"%s\"\n",
                fName, source_encoding, target_encoding, strerror(errno));
            free(fName);
            continue;
        }

        /*
         * Check if the name is alredy encoded in a valid target encoding.
         * If the source and destination match, ignore.
         */
        if(fSize == strlen(nName) && strcmp(nName, fName) == 0) {
            printf("  %s: OK\n", fName);
            free(fName);
            continue;
        }

        if(!dry_run && zip_rename(z, fidx, nName)) {
            printf("  %s: Failed to rename inside archive: %s\n",
                nName, zip_strerror(z));
        } else {
            printf("  %s: FIXED (%s -> %s)\n", nName,
                    source_encoding, target_encoding);
        }
        free(fName);
    }

    if(zip_close(z) == 0) {
        return 0;
    } else {
        fprintf(stderr, "%s: %s\n", zipfile, zip_strerror(z));
        zip_unchange(z, 0);
        zip_close(z);
        return -1;
    }
}

static void usage(char *argv0, const char *default_target_encoding) {
    fprintf(stderr,
      "Russian filename encoding fix inside ZIP archives\n"
      "Copyright (c) 2007, 2009, 2015 Lev Walkin <vlm@lionet.info>\n"
      "libzip by Dieter Baron <dillo@giga.or.at> and Thomas Klausner <tk@giga.or.at>\n\n"
      "Usage: %s [OPTIONS] <filename.zip>...\n"
      "Where OPTIONS are:\n"
      "  -h                 Display this help screen\n"
      "  -n                 Dry run. Do not modify the <file.zip>\n"
      "  -v                 Verbose output\n"
      "  -s <encoding>      Set source encoding. Auto-detect, if not set\n"
      "  -t <encoding>      Set target encoding. Default is %s\n"
      , basename(argv0), default_target_encoding);
    exit(EX_USAGE);
}

int
main (int ac, char **av) {
    int ch;
    int dry_run = 0;    /* -n, do not modify archive */
    int verbose = 0;
    const char *source_encoding = NULL;
#if __APPLE__
    const char *default_target_encoding = "UTF-8-MAC";
#else
    const char *default_target_encoding = "UTF-8";
#endif
    const char *target_encoding = default_target_encoding;

    while((ch = getopt(ac, av, "hvns:t:")) != -1) {
        switch(ch) {
        case 'n':
            dry_run = 1;
            break;
        case 'v':
            verbose = 1;
            break;
        case 's': {
            source_encoding = optarg;
            iconv_t ic = iconv_open(target_encoding, source_encoding);
            if(ic == (iconv_t)-1) {
                fprintf(stderr, "-s %s: Invalid encoding\n", source_encoding);
                exit(EX_USAGE);
            } else {
                iconv_close(ic);
            }
            break;
            }
        case 't': {
            target_encoding = optarg;
            iconv_t ic = iconv_open(target_encoding,
                                    source_encoding?:"windows-1251");
            if(ic == (iconv_t)-1) {
                fprintf(stderr, "-t %s: Invalid encoding\n", target_encoding);
                exit(EX_USAGE);
            } else {
                iconv_close(ic);
            }
            break;
            }
        case 'h':
            /* FALL THROUGH */
        default:
            usage(av[0], default_target_encoding);
        }
    }
    if(optind >= ac)
        usage(av[0], default_target_encoding);

    for(; optind < ac; optind++) {
        char *zipfile = av[optind];
        if(fix_cyrillic_filenames(zipfile, dry_run,
                    source_encoding, target_encoding, verbose) == -1) {
            exit(EX_NOINPUT);
        }
    }

    return 0;
}
