targets = mbfinfo mbfchr bdf2mbf mbf2fnt

all: $(targets)

mbfinfo: mbf.h mbf.c mbfinfo.c
	$(CC) $(CFLAGS) -o $@ mbf.c mbfinfo.c

mbfchr: mbf.h mbf.c mbfchr.c
	$(CC) $(CFLAGS) -o $@ mbf.c mbfchr.c

bdf2mbf: bdf2mbf.c
	$(CC) $(CFLAGS) -o $@ bdf2mbf.c

mbf2fnt: mbf.h mbf.c mbf2fnt.c
	$(CC) $(CFLAGS) -o $@ mbf.c mbf2fnt.c

clean:
	$(RM) $(targets)

