MKDIR=mkdir -p
RM=rm -f
MV=mv

BIN=./bin
SRC=./src
INC=./include
PDF=$(INC)/pdf

TEX=pdflatex
PROJECT=main
REPORT=reunion_tsar
INK=inkscape

all: folder svg

	BUILDTEX= $(TEX) -output-directory $(BIN) \
					-jobname $(REPORT) $(SRC)/$(PROJECT).tex

svg: folder
	$(INK) -D -z --file=$(INC)/img/ghassan_version.svg \
		  --export-pdf=$(INC)/img/ghassan_version.pdf --export-latex
	$(MV) $(INC)/img/*pdf* $(PDF)/

folder:
	$(MKDIR) $(BIN) $(PDF)

clean:
	$(RM) $(BIN)/*
	$(RM) $(PDF)/*

mrproper: clean
	$(RM) -r $(BIN)
	$(RM) -r $(PDF)
