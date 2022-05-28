CPP=g++
LD=g++
SRCDIR=src
BUILDIR=build
EXEC=elfdump

all: $(EXEC)

$(EXEC): $(BUILDIR)/main.o
	$(LD) -o $@ $<

$(BUILDIR)/%.o: $(SRCDIR)/%.cpp build
	$(CPP) -c $< -o $@

build:
	mkdir $(BUILDIR)

.PHONY: clean

clean:
	rm -rf $(BUILDIR)
	rm -f $(EXEC)