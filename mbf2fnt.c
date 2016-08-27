#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include "mbf.h"

#define MAX_NAME    0x10

int
main(int argc, char *argv[])
{
    Font *font;
    int i, j, k;
    uint16_t a, b, code;
    char name[MAX_NAME+1];
    FILE *fp;
    int index, pixel;
    uint8_t *row;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s font.mbf\n", argv[0]);
        return 1;
    }
    font = load_font(argv[1]);
    if (font == NULL) {
        fprintf(stderr, "Failed to load font '%s'.\n", argv[1]);
        return 1;
    }
    name[MAX_NAME] = '\0';
    snprintf(name, MAX_NAME, "%ux%u", font->header.w, font->header.h);
    if (mkdir(name, 0777) == -1) {
        fprintf(stderr, "Failed to create dir '%s'.\n", name);
        return 1;
    }
    chdir(name);
    for (i = 0; i < font->header.nr; i++) {
        a = font->ranges[i].offset;
        b = a + font->ranges[i].length;
        for (code = a; code < b; code++) {
            snprintf(name, MAX_NAME, "U+%04X", code);
            fp = fopen(name, "w");
            index = search_glyph(font, code);
            row = &font->data[font->stride * font->header.h * index];
            for (j = 0; j < font->header.h; j++) {
                for (k = 0; k < font->header.w; k++) {
                    pixel = row[k >> 3] & (1 << (7 - (k & 7)));
                    fputc(pixel ? 'X' : '.', fp);
                }
                fputc('\n', fp);
                row += font->stride;
            }
            fclose(fp);
        }
    }
    free(font);
    return 0;
}
