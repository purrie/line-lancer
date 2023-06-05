
CC=gcc
BIN=line-lancer
OBJ_FOLDER=cache
SOURCE_FOLDER=src
SOURCE_TEST_FOLDER=tests
BIN_FOLDER=bin

FLAGS=
DEBUG_FLAGS=-g
LIBS=-lraylib -lm

SOURCES=$(wildcard $(SOURCE_FOLDER)/*.c)
TESTS=$(wildcard $(SOURCE_TEST_FOLDER)/*_test.c)

OBJECTS=$(patsubst $(SOURCE_FOLDER)/%.c, $(OBJ_FOLDER)/%_src.o, $(SOURCES))
TEST_OBJECTS=$(patsubst $(SOURCE_TEST_FOLDER)/%_test.c, $(OBJECT_FOLDER)/%.o, $(TESTS)) $(patsubst $(SOURCE_TEST_FOLDER)/%.c, $(OBJ_FOLDER)/%.o, $(TESTS))

BIN_TESTS=$(patsubst $(SOURCE_TEST_FOLDER)/%.c, $(BIN_FOLDER)/%.ut, $(TESTS))

build: $(BIN_FOLDER) $(OBJ_FOLDER) $(BIN_FOLDER)/$(BIN)

build-debug:
	make clean
	FLAGS="$(FLAGS) $(DEBUG_FLAGS)" make build -e

run: build
	$(BIN_FOLDER)/$(BIN)

debug: build-debug
	gdb $(BIN_FOLDER)/$(BIN) -d $(SOURCE_FOLDER)

test: $(BIN_FOLDER) $(OBJ_FOLDER) $(BIN_TESTS)
	@for test in $(BIN_TESTS); do $$test; done

clean:
	rm -rf $(BIN_FOLDER) $(OBJ_FOLDER)

$(BIN_FOLDER)/$(BIN): $(OBJECTS)
	$(CC) $(FLAGS) $(LIBS) -o $@ $(OBJECTS)

$(BIN_FOLDER)/%_test.ut: $(OBJ_FOLDER)/%_src.o $(OBJ_FOLDER)/%_test.o
	$(CC) $(FLAGS) $(LIBS) -o $@ $^

$(BIN_FOLDER):
	mkdir -p $(BIN_FOLDER)

$(OBJ_FOLDER):
	mkdir -p $(OBJ_FOLDER)

$(OBJ_FOLDER)/%_src.o: $(SOURCE_FOLDER)/%.c
	$(CC) $(FLAGS) -o $@ -c $<

$(OBJ_FOLDER)/%_test.o: $(SOURCE_TEST_FOLDER)/%_test.c
	$(CC) $(FLAGS) -o $@ -c $<
