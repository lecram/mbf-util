targets = mbfinfo mbfchr bdf2mbf

all: $(targets)

mbfinfo: mbf.h mbf.c mbfinfo.c
	$(CC) $(CFLAGS) -o $@ mbf.c mbfinfo.c

mbfchr: mbf.h mbf.c mbfchr.c
	$(CC) $(CFLAGS) -o $@ mbf.c mbfchr.c

bdf2mbf: bdf2mbf.c
	$(CC) $(CFLAGS) -o $@ bdf2mbf.c

clean:
	$(RM) $(targets)

