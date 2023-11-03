CC = clang
CFLAGS = -Wall -Wextra -Werror -Wpedantic -Wconversion -std=c18 -ggdb -O0
CINCLUDES = -I./src
CLIBS =

SRC = src
OBJ = obj
EXAMPLES = examples
BINDIR = bin

# List of example source files
EXAMPLE_SRCS = $(wildcard $(EXAMPLES)/*.c)
EXAMPLE_BINS = $(patsubst $(EXAMPLES)/%.c, $(BINDIR)/%, $(EXAMPLE_SRCS))

# List of object files from the src directory
MAIN_SRCS = $(wildcard $(SRC)/*.c)
MAIN_OBJS = $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(MAIN_SRCS))

all: $(EXAMPLE_BINS)

$(BINDIR)/%: $(EXAMPLES)/%.c $(MAIN_OBJS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CINCLUDES) $^ -o $@ $(CLIBS)

$(OBJ)/%.o: $(SRC)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CINCLUDES) -c $< -o $@

$(OBJ)/%.o: $(EXAMPLES)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CINCLUDES) -c $< -o $@

clean:
	rm -rf $(BINDIR) $(OBJ)

