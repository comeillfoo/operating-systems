CPP=g++
LD=g++

SRCDIR=src
BUILDIR=build

EXEC=rrobin

all: $(EXEC)

build:
	mkdir $(BUILDIR)

$(EXEC): $(BUILDIR)/main.o
	$(LD) -o $@ $<

$(BUILDIR)/%.o: $(SRCDIR)/%.cpp build
	$(CPP) -c $< -o $@

.PHONY: clean

clean:
	rm -rf $(BUILDIR)
	rm -f $(EXEC)