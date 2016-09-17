#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <assert.h>

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
comp_int(const void *a, const void *b)
{
    const int *arg1 = a;
    const int *arg2 = b;

    return *arg1 - *arg2;
}

int
main(int argc, char *argv[])
{
    DIR *dp;
    struct dirent *ep;
    int i, j, k, n, w, h, nr, cur, prev;
    int *codes;
    Range *range, *charset;
    int fd;
    Header hd;
    int stride;
    uint8_t *canvas;
    FILE *fp;
    char fname[] = "U+0000\0";

    if (argc != 3) {
        fprintf(stderr, "Usage: %s fntdir font.mbf\n", argv[0]);
        return 1;
    }
    if (!(dp = opendir(argv[1]))) {
        fprintf(stderr, "Failed to open directory '%s'.\n", argv[1]);
        return 1;
    }
    sscanf(basename(argv[1]), "%dx%d", &w, &h);
    n = 0;
    while ((ep = readdir(dp))) {
        if (strncmp(ep->d_name, "U+", 2))
            continue;
        n++;
    }
    rewinddir(dp);
    codes = malloc(n * sizeof(*codes));
    i = 0;
    while ((ep = readdir(dp))) {
        if (strncmp(ep->d_name, "U+", 2))
            continue;
        codes[i++] = strtol(&ep->d_name[2], NULL, 16);
    }
    closedir(dp);
    qsort(codes, n, sizeof(*codes), comp_int);
    prev = -2; /* make sure prev + 1 is not a valid code */
    nr = 0;
    for (i = 0; i < n; i++) {
        cur = codes[i];
        if (cur != prev + 1)
            nr++;
        prev = cur;
    }
    range = charset = malloc((nr + 1) * sizeof(*range));
    range->first = prev = codes[0];
    for (i = 1; i < n; i++) {
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
    fd = creat(argv[2], 0644);
    if (fd == -1) {
        fprintf(stderr, "Failed to create file '%s'\n", argv[2]);
        return 1;
    }
    hd = (Header) {
        "MBF\x01",
        (uint16_t) n,
        (uint8_t) w,
        (uint8_t) h,
        (uint16_t) nr
    };
    write(fd, &hd, sizeof(hd));
    for (range = charset; range->first >= 0; range++) {
        uint16_t offset = (uint16_t) range->first;
        uint16_t length = (uint16_t) (range->last - range->first + 1);
        write(fd, &offset, 2);
        write(fd, &length, 2);
    }
    free(charset);
    stride = (w >> 3) + !!(w & 7);
    canvas = malloc(h * stride);
    if (canvas == NULL) {
        fprintf(stderr, "Failed to allocate canvas.\n");
        return 1;
    }
    chdir(argv[1]);
    for (i = 0; i < n; i++) {
        snprintf(fname, sizeof(fname) - 1, "U+%04X", codes[i]);
        fp = fopen(fname, "r");
        assert(fp != NULL);
        memset(canvas, 0, h * stride);
        for (j = 0; j < h; j++) {
            for (k = 0; k < w; k++) {
                if (fgetc(fp) != '.')
                    canvas[j * stride + (k >> 3)] |= 1 << (7 - k % 8);
            }
            assert(fgetc(fp) == '\n');
        }
        write(fd, canvas, h * stride);
        fclose(fp);
    }
    free(codes);
    free(canvas);
    close(fd);
    return 0;
}
