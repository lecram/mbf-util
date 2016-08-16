#include <stdio.h>
#include <stdlib.h>

#include "mbf.h"

int
main(int argc, char *argv[])
{
    Font *font;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s font.mbf\n", argv[0]);
        return 1;
    }
    font = load_font(argv[1]);
    if (font == NULL) {
        fprintf(stderr, "Failed to load font '%s'.\n", argv[1]);
        return 1;
    }
    printf("chars %u\n", font->header.ng);
    printf("width %u\n", font->header.w);
    printf("height %u\n", font->header.h);
    free(font);
    return 0;
}
