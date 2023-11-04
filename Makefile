CC = clang
# CFLAGS = -Wall -Wextra -Werror -Wpedantic -Wconversion -std=c18 -ggdb -O0
CFLAGS = -Wall -Wextra -Wpedantic -Wconversion -std=c18 -ggdb -O0
CINCLUDES = -I./src
CLIBS =

SRC = src
OBJ = obj
BINDIR = bin

EXE = $(BINDIR)/math_gym

# List of object files from the src directory
SRCS = $(wildcard $(SRC)/*.c)
OBJS = $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS))

all: $(EXE)

$(EXE): $(OBJS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CINCLUDES) $^ -o $@ $(CLIBS)

$(OBJ)/%.o: $(SRC)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CINCLUDES) -c $< -o $@

clean:
	rm -rf $(BINDIR) $(OBJ)

