CC=cc
TARGETS=imp4

all: $(TARGETS)

imp4: src/parser.c
	$(CC)  -I include -o bin/imp4 src/parser.c

clean:
	-rm bin/$(TARGETS)
