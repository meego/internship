MKDIR=mkdir -p
RM=rm -f

BIN=./bin
SRC=./src

TEX=pdflatex
PROJECT=main
REPORT=problem-and-context

all: folder

	BUILDTEX= $(TEX) -output-directory $(BIN) \
					-jobname $(REPORT) $(SRC)/$(PROJECT).tex

folder:
	$(MKDIR) $(BIN)
clean:
	$(RM) $(BIN)/*

mrproper: clean
	$(RM) -r $(BIN)
