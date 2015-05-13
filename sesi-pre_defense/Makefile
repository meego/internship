MKDIR=mkdir -p
RM=rm -f

BIN=./bin
SRC=./src
INC=./include

PROJECT=main
REPORT=peneau-internship-presentation

TEX=pdflatex

all: folder out

folder:
	$(MKDIR) $(BIN)

out:
	BUILDTEX= $(TEX) -output-directory $(BIN) \
					-jobname $(REPORT) $(SRC)/$(PROJECT).tex

clean:
	$(RM) $(BIN)/*

mrproper:
	$(RM) -r $(BIN)

