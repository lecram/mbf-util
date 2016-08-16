#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "mbf.h"

int
main(int argc, char *argv[])
{
    Font *font;
    int index;
    uint16_t code;
    int i, j;
    int pixel;
    uint8_t *row;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s font.mbf code\n", argv[0]);
        return 1;
    }
    font = load_font(argv[1]);
    if (font == NULL) {
        fprintf(stderr, "Failed to load font '%s'.\n", argv[1]);
        return 1;
    }
    code = (uint16_t) strtoul(argv[2], NULL, 0);
    index = search_glyph(font, code);
    if (index == -1) {
        fprintf(stderr, "Glyph not found.\n");
        return 1;
    }
    row = &font->data[font->stride * font->header.h * index];
    for (i = 0; i < font->header.h; i++) {
        for (j = 0; j < font->header.w; j++) {
            pixel = row[j >> 3] & (1 << (7 - (j & 7)));
            putchar(pixel ? '#' : ' ');
        }
        putchar('\n');
        row += font->stride;
    }
    free(font);
    return 0;
}
