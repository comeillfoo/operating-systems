CC=gcc
LD=gcc

SRCDIR=src
BUILDIR=build
EXEC=slab

all: $(EXEC)

build:
	mkdir $(BUILDIR)

$(EXEC): $(BUILDIR)/main.o
	$(LD) -o $@ $^

$(BUILDIR)/%.o: $(SRCDIR)/%.c build
	$(CPP) -c $< -o $@

.PHONY: clean

clean:
	rm -rf $(BUILDIR)
	rm -f $(EXEC)