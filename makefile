
CC=clang
BIN=line-lancer
OBJ_FOLDER=cache
SOURCE_FOLDER=src
SOURCE_TEST_FOLDER=tests
BIN_FOLDER=bin

FLAGS=
LIBS=-lraylib

SOURCES=$(wildcard $(SOURCE_FOLDER)/*.c)
TESTS=$(wildcard $(SOURCE_TEST_FOLDER)/*_test.c)

OBJECTS=$(patsubst $(SOURCE_FOLDER)/%.c, $(OBJ_FOLDER)/%_src.o, $(SOURCES))
TEST_OBJECTS=$(patsubst $(SOURCE_TEST_FOLDER)/%_test.c, $(OBJECT_FOLDER)/%.o, $(TESTS)) $(patsubst $(SOURCE_TEST_FOLDER)/%.c, $(OBJ_FOLDER)/%.o, $(TESTS))

BIN_TESTS=$(patsubst $(SOURCE_TEST_FOLDER)/%.c, $(BIN_FOLDER)/%.ut, $(TESTS))

run: $(BIN_FOLDER) $(OBJ_FOLDER) $(BIN_FOLDER)/$(BIN)
	$(BIN_FOLDER)/$(BIN)

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
	$(CC) -o $@ -c $<

$(OBJ_FOLDER)/%_test.o: $(SOURCE_TEST_FOLDER)/%_test.c
	$(CC) -o $@ -c $<
