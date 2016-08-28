#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define QUOTE(s) #s
#define QUOTE_MACRO(s) QUOTE(s)

/* A BDF file is a plain text file.  In other words, it is a sequence of
 * newline- terminated strings. This program treats all strings as ASCII
 * text. There are  two types of strings in BDF  files: bitmap lines and
 * metadata lines.  Bitmap lines are  hexadecimal numbers that  encode a
 * single row  of a glyph. Metadata  lines begin with a  BDF keyword. If
 * the keyword is  unknown, the entire line is considered  a comment and
 * is  ignored  by  this  program.  Metadata lines  have  zero  or  more
 * arguments. The number of arguments is fixed per keyword. For example,
 * the  ENDFONT keyword  has no  arguments,  the CHARS  keyword has  one
 * argument and the FONTBOUNDINGBOX keyword has four arguments. Metadata
 * lines with zero arguments are  composed solely by a keyword. Metadata
 * lines with one or more arguments  are composed by a keyword, followed
 * by one or more spaces or tabs, followed by a space-delimited sequence
 * of arguments. */

#define KEYMAX 64
#define SEPMAX 16
#define VALMAX 128

static char skey[KEYMAX+1];
static char ssep[SEPMAX+1];
static char sval[VALMAX+1];

typedef struct Metadata {
    int w, h, x, y, n;
} Metadata;

typedef struct Range {
    int first, last;
} Range;

typedef struct Header {
    char sig[4];
    uint16_t nglyphs;
    uint8_t width, height;
    uint16_t nranges;
} Header;

static int
get_val(FILE *fp, const char *key)
{
    do {
        if (fscanf(fp, "%" QUOTE_MACRO(KEYMAX) "s", skey) == EOF)
            return 1;
        fscanf(fp, "%" QUOTE_MACRO(SEPMAX) "[ \t\n]", ssep);
        if (strchr(ssep, '\n'))
            strcpy(sval, "");
        else
            fscanf(fp, "%" QUOTE_MACRO(VALMAX) "[^\n]\n", sval);
    } while (strcmp(key, skey));
    return 0;
}

int
bdf_info(FILE *fp, Metadata *md)
{
    if (get_val(fp, "STARTFONT"))
        return 1;
    get_val(fp, "FONTBOUNDINGBOX");
    sscanf(sval, "%d %d %d %d", &md->w, &md->h, &md->x, &md->y);
    get_val(fp, "CHARS");
    sscanf(sval, "%d", &md->n);
    return 0;
}

static int
comp_int(const void *a, const void *b)
{
    const int *arg1 = a;
    const int *arg2 = b;
 
    return *arg1 - *arg2;
}

Range *
bdf_charset(FILE *fp, Metadata *md)
{
    int i, nr, cur, prev;
    int *codes;
    Range *range, *charset;

    if (!md->n)
        bdf_info(fp, md);
    codes = malloc(md->n * sizeof(*codes));
    for (i = 0; i < md->n; i++) {
        get_val(fp, "ENCODING");
        sscanf(sval, "%d", &codes[i]);
    }
    qsort(codes, md->n, sizeof(*codes), comp_int);
    prev = -2; /* make sure prev + 1 is not a valid code */
    nr = 0;
    for (i = 0; i < md->n; i++) {
        cur = codes[i];
        if (cur != prev + 1)
            nr++;
        prev = cur;
    }
    range = charset = malloc((nr + 1) * sizeof(*range));
    range->first = prev = codes[0];
    for (i = 1; i < md->n; i++) {
        cur = codes[i];
        if (cur != prev + 1) {
            range->last = prev;
            range++;
            range->first = cur;
        }
        prev = cur;
    }
    range->last = prev;
    charset[nr].first = -1; /* this marks the end of the array */
    free(codes);
    return charset;
}

int
main(int argc, char *argv[])
{
    FILE *fp;
    Metadata md;
    Range *charset, *range;
    Header hd;
    uint16_t offset, length;
    int n, i, j, k;
    int fd;
    int canvas_stride, glyph_stride;
    int gw, gh, gx, gy;
    uint8_t *canvas;

    if (argc != 3) {
        fprintf(stderr, "usage: %s input.bdf output.mbf\n", argv[0]);
        return 1;
    }
    fp = fopen(argv[1], "r");
    if (!fp) {
        fprintf(stderr, "could not open file %s\n", argv[1]);
        return 1;
    }
    if (bdf_info(fp, &md)) {
        fprintf(stderr, "%s is not a BDF file\n", argv[1]);
        return 1;
    }
    range = charset = bdf_charset(fp, &md);
    fd = creat(argv[2], 0644);
    if (fd == -1) {
        fprintf(stderr, "could not create file %s\n", argv[2]);
        return 1;
    }
    for (n = 0; range->first >= 0; n++, range++);
    hd = (Header) {
      "MBF\x01",
      (uint16_t) md.n,
      (uint8_t) md.w,
      (uint8_t) md.h,
      (uint16_t) n
    };
    write(fd, &hd, sizeof(hd));
    for (range = charset; range->first >= 0; range++) {
        offset = (uint16_t) range->first;
        length = (uint16_t) (range->last - range->first + 1);
        write(fd, &offset, 2);
        write(fd, &length, 2);
    }
    canvas_stride = (md.w >> 3) + !!(md.w & 7);
    canvas = malloc(md.h * canvas_stride);
    if (canvas == NULL) {
        fprintf(stderr, "could not allocate memory\n");
        return 1;
    }
    rewind(fp);
    for (n = 0; n < md.n; n++) {
        uint16_t code;
        uint8_t byte;
        char hex[3] = {0};
        get_val(fp, "ENCODING");
        sscanf(sval, "%" SCNu16, &code);
        get_val(fp, "BBX");
        sscanf(sval, "%d %d %d %d", &gw, &gh, &gx, &gy);
        gx -= md.x;
        gy -= md.y;
        gy = md.h - gy - gh;
        glyph_stride = (gw >> 3) + !!(gw & 7);
        get_val(fp, "BITMAP");
        memset(canvas, 0, md.h * canvas_stride);
        for (i = 0; i < gh; i++) {
            for (j = 0; j < glyph_stride; j++) {
                fread(hex, 1, 2, fp);
                byte = (uint8_t) strtol(hex, NULL, 16);
                k = gx / 8 + j + (gy + i) * canvas_stride;
                canvas[k] |= byte >> (gx % 8);
                if (gx % 8 != 0 && gx / 8 + j + 1 < canvas_stride)
                    canvas[k + 1] |= byte << (7 - gx % 8);
            }
            fseek(fp, 1, SEEK_CUR); /* skip newline */
        }
        write(fd, canvas, md.h * canvas_stride);
    }
    free(canvas);
    fclose(fp);
    close(fd);
    free(charset);
    return 0;
}
