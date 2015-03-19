
MKDIR=mkdir -p
RM=rm -f

BIN=./bin
SRC=./src

TEX=pdflatex
PROJECT=main
REPORT=almos_task_fork

all: folder

	BUILDTEX= $(TEX) -output-directory $(BIN) \
		  -jobname $(REPORT) $(SRC)/$(PROJECT).tex

folder:
	$(MKDIR) $(BIN)

diagram:
	# Generate svg. I need to use ../ because PlantUML does not provide an
	# option to set the cwd
	plantuml -tsvg -o ../include/img src/
	# Generate tikz. 
	plantuml -tlatex src/

clean:
	$(RM) $(SRC)/*.latex
	$(RM) $(BIN)/*

mrproper: clean
	$(RM) -r $(BIN)
