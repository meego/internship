MK=mkdir -p
RM=rm -f
LN=ln -sf

BIN=./bin
SRC=./src

TEX=pdflatex
IDX=makeindex
BIB=bibtex
PROJECT=main

REPORT=main
MAGIC_FILE=/usr/share/texlive/texmf-dist/makeindex/nomencl/nomencl.ist

all: dir out

out:
	BUILDTEX= $(TEX) -output-directory $(BIN) \
		-jobname $(REPORT) $(SRC)/$(PROJECT)

bib: out
	$(LN) $(SRC)/references.bib $(BIN)
	$(BIB) $(BIN)/$(REPORT)

idx: out
	$(IDX) $(BIN)/$(REPORT).nlo -s $(MAGIC_FILE) -o $(BIN)/$(REPORT).nls

dir:
	$(MK) $(BIN)

clean:
	$(RM) $(BIN)/*
