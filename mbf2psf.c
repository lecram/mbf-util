#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mbf.h"

struct psf2_header {
    unsigned char magic[4];
    unsigned int version;
    unsigned int headersize;    /* offset of bitmaps in file */
    unsigned int flags;
    unsigned int length;        /* number of glyphs */
    unsigned int charsize;      /* number of bytes for each character */
    unsigned int height, width; /* max dimensions of glyphs */
    /* charsize = height * ((width + 7) / 8) */
};

#define write_byte(fd, b)   write((fd), (uint8_t []) {(b)}, 1)

int
main(int argc, char *argv[])
{
    Font *font;
    int i;
    uint16_t a, b, code;
    int fd;
    int index;
    uint8_t *bitmap;
    struct psf2_header header;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s input.mbf output.psf\n", argv[0]);
        return 1;
    }
    font = load_font(argv[1]);
    if (font == NULL) {
        fprintf(stderr, "Failed to load font '%s'.\n", argv[1]);
        return 1;
    }
    fd = creat(argv[2], 0644);
    if (fd == -1) {
        fprintf(stderr, "Failed to create file '%s'\n", argv[2]);
        return 1;
    }
    memcpy(header.magic, (uint8_t []) {0x72, 0xb5, 0x4a, 0x86}, 4);
    header.version = 0;
    header.headersize = sizeof(header);
    header.flags = 0x01; /* has unicode table */
    header.length = font->header.ng;
    header.charsize = font->header.h * ((font->header.w + 7) / 8);
    header.height = font->header.h;
    header.width = font->header.w;
    write(fd, &header, sizeof(header));
    for (i = 0; i < font->header.nr; i++) {
        a = font->ranges[i].offset;
        b = a + font->ranges[i].length;
        for (code = a; code < b; code++) {
            index = search_glyph(font, code);
            bitmap = &font->data[font->stride * font->header.h * index];
            write(fd, bitmap, header.charsize);
        }
    }
    for (i = 0; i < font->header.nr; i++) {
        a = font->ranges[i].offset;
        b = a + font->ranges[i].length;
        for (code = a; code < b; code++) {
            /* here we write UCS-2 `code` as UTF-8 to `fd` */
            if (code < 0x80)
                write_byte(fd, (uint8_t) code);
            else if (code < 0x800) {
                write_byte(fd, (uint8_t) (0xC0 | (code >> 6)));
                write_byte(fd, (uint8_t) (0x80 | (code & 0x3F)));
            } else {
                write_byte(fd, (uint8_t) (0xE0 | (code >> 12)));
                write_byte(fd, (uint8_t) (0x80 | ((code >> 6) & 0x3F)));
                write_byte(fd, (uint8_t) (0x80 | (code & 0x3F)));
            }
            /* write table entry separator */
            write_byte(fd, 0xFF);
        }
    }
    close(fd);
    free(font);
    return 0;
}
