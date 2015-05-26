MKDIR=mkdir -p
RM=rm -f

BIN=./bin
SRC=./src

TEX=pandoc
PROJECT=main.md
OUTPUT=dev_env_installation.pdf

all: folder
	$(TEX) $(SRC)/$(PROJECT) -o $(BIN)/$(OUTPUT)


folder:
	$(MKDIR) $(BIN)

clean:
	$(RM) $(BIN)/*

mrproper: clean
	$(RM) -r $(BIN)
