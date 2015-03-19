MKDIR=mkdir -p
RM=rm -f

BIN=./bin
SRC=./src

TEX=pdflatex
OUTPUT=tikz_v1.pdf tikz_v2.pdf


all: folder $(OUTPUT)

%.pdf: $(SRC)/%.tex
	BUILDTEX= $(TEX) -output-directory $(BIN) \
					-jobname $@ $<

folder:
	$(MKDIR) $(BIN)
clean:
	$(RM) $(BIN)/*

mrproper: clean
	$(RM) -r $(BIN)
