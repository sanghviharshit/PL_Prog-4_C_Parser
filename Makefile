CC=cc
TARGETS=imp4

all: $(TARGETS)

imp4: parser.c
	$(CC) -o imp4 parser.c

clean:
	-rm *.o $(TARGETS)
