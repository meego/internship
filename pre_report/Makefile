MK=mkdir -p
RM=rm -f
LN=ln -sf

BIN=./bin
SRC=./src

TEX=pdflatex
BIB=bibtex
PROJECT=main

REPORT=main

all: dir out

out:
	BUILDTEX= $(TEX) -output-directory $(BIN) \
		-jobname $(REPORT) $(SRC)/$(PROJECT)

bib: out
	$(LN) $(SRC)/references.bib $(BIN)
	$(BIB) $(BIN)/$(REPORT)

dir:
	$(MK) $(BIN)
clean:
	$(RM) $(BIN)/*
