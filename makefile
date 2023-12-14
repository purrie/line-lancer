
CC=clang
CCW=x86_64-w64-mingw32-gcc
ARW=x86_64-w64-mingw32-ar

BIN=line-lancer

OBJ_FOLDER=cache
SOURCE_FOLDER=src
SOURCE_TEST_FOLDER=tests
BIN_FOLDER=bin
LIBS_PATH_WIN=lib/windows
LIBS_PATH_LNX=lib/linux

RAYLIB_NAME=libraylib.a
RAYLIB_WIN=$(LIBS_PATH_WIN)/$(RAYLIB_NAME)
RAYLIB_LNX=$(LIBS_PATH_LNX)/$(RAYLIB_NAME)

FLAGS_LNX    ?= -Wall -Wextra -Warray-bounds -Wshadow -Wunused -Wdeprecated -I "vendor/raylib/src"
FLAGS_WIN    ?= -Wall -Wextra -Warray-bounds -Wshadow -Wunused -Wdeprecated -I "vendor/raylib/src"
DEBUG_FLAGS   = -ggdb
RELEASE_FLAGS = -Wl,-s -O3 -DRELEASE

LIBS=-lm
LIBW=-lm -lgdi32 -lwinmm

SOURCES=$(wildcard $(SOURCE_FOLDER)/*.c)
TESTS=$(wildcard $(SOURCE_TEST_FOLDER)/*_test.c)

OBJECTS_LINUX = $(patsubst $(SOURCE_FOLDER)/%.c, $(OBJ_FOLDER)/%_lnx.o, $(SOURCES))
OBJECTS_WIN   = $(patsubst $(SOURCE_FOLDER)/%.c, $(OBJ_FOLDER)/%_win.o, $(SOURCES))
TEST_OBJECTS  = $(patsubst $(SOURCE_TEST_FOLDER)/%_test.c, $(OBJECT_LINUX)/%.o, $(TESTS)) $(patsubst $(SOURCE_TEST_FOLDER)/%.c, $(OBJ_LINUX)/%.o, $(TESTS))

BIN_TESTS=$(patsubst $(SOURCE_TEST_FOLDER)/%.c, $(BIN_FOLDER)/%.ut, $(TESTS))

# LINUX #######################################################################
build: $(BIN_FOLDER)/$(BIN)

run: build
	$(BIN_FOLDER)/$(BIN)

rebuild: clean build

rerun: clean run

$(BIN_FOLDER)/$(BIN): $(BIN_FOLDER) $(OBJECTS_LINUX) $(RAYLIB_LNX)
	$(CC) $(FLAGS_LNX) -L$(LIBS_PATH_LNX)/ -o $@ $(OBJECTS_LINUX) -l:$(RAYLIB_NAME) $(LIBS)

$(OBJ_FOLDER)/%_lnx.o: $(SOURCE_FOLDER)/%.c $(OBJ_FOLDER)
	$(CC) $(FLAGS_LNX) -o $@ -c $<

# WINDOWS #####################################################################
build-win: $(BIN_FOLDER)/$(BIN).exe

$(OBJ_FOLDER)/%_win.o: $(SOURCE_FOLDER)/%.c $(OBJ_FOLDER)
	$(CCW) $(FLAGS_WIN) -o $@ -c $<

$(BIN_FOLDER)/$(BIN).exe: $(BIN_FOLDER) $(OBJECTS_WIN) $(RAYLIB_WIN)
	$(CCW) $(FLAGS_WIN) -L$(LIBS_PATH_WIN) -o $@ $(OBJECTS_WIN) -l:$(RAYLIB_NAME) $(LIBW)

# PACKING #####################################################################
pack-linux:
	make clean
	make build FLAGS_LNX="$(FLAGS_LNX) $(RELEASE_FLAGS)" CC=gcc
	if [ -d "pack/linux" ]; then rm -rf pack/linux; fi
	mkdir -p pack/linux/line-lancer
	mkdir -p pack/linux/line-lancer/assets
	cp -fu $(BIN_FOLDER)/$(BIN) pack/linux/line-lancer/line-lancer
	cp -fur assets/* pack/linux/line-lancer/assets/
	cp -fu deploy/linux/* pack/linux/line-lancer/
	cd pack/linux && if [ -e "line-lancer.tar.gz" ]; then rm line-lancer.tar.gz; fi && tar -caf line-lancer.tar.gz line-lancer

pack-windows:
	make clean
	make build-win FLAGS_WIN="$(FLAGS_WIN) $(RELEASE_FLAGS)"
	if [ -d "pack/windows" ]; then rm -rf pack/windows; fi
	mkdir -p pack/windows/Line-Lancer
	mkdir -p pack/windows/Line-Lancer/assets
	cp -fu $(BIN_FOLDER)/$(BIN).exe pack/windows/Line-Lancer
	cp -fur assets/* pack/windows/Line-Lancer/assets/
	cd pack/windows && if [ -e "Line-Lancer.zip" ]; then rm Line-Lancer.zip; fi && zip -r Line-Lancer Line-Lancer

# STRUCTURE ###################################################################
$(BIN_FOLDER):
	mkdir -p $(BIN_FOLDER)

$(OBJ_FOLDER):
	mkdir -p $(OBJ_FOLDER)

$(LIBS_PATH_WIN):
	mkdir -p $(LIBS_PATH_WIN)

$(LIBS_PATH_LNX):
	mkdir -p $(LIBS_PATH_LNX)

clean:
	rm -rf $(BIN_FOLDER) $(OBJ_FOLDER) $(LIBS_PATH_WIN) $(LIBS_PATH_LNX)

cleanrl:
	make -C vendor/raylib/src clean

clean-packs:
	rm -rf pack

cleanall: clean clean-packs cleanrl

# DEPENDENCIES ################################################################
build-raylib:
	make $(RAYLIB_LNX)
	make $(RAYLIB_WIN)

vendor/raylib/:
	git submodule init
	git submodule update --recursive

$(RAYLIB_LNX): $(LIBS_PATH_LNX) vendor/raylib/
	make -C vendor/raylib/src clean
	make -C vendor/raylib/src PLATFORM=PLATFORM_DESKTOP GRAPHICS=GRAPHICS_API_OPENGL_21 RAYLIB_RELEASE_PATH="../../../$(LIBS_PATH_LNX)"

$(RAYLIB_WIN): $(LIBS_PATH_WIN) vendor/raylib/
	make -C vendor/raylib/src clean
	make -C vendor/raylib/src PLATFORM=PLATFORM_DESKTOP GRAPHICS=GRAPHICS_API_OPENGL_21 CC=$(CCW) AR=$(ARW) RAYLIB_RELEASE_PATH="../../../$(LIBS_PATH_WIN)"

# TESTS #######################################################################
build-debug: clean
	 make build FLAGS_LNX="$(FLAGS) $(DEBUG_FLAGS)"

debug: build-debug
	gf2 $(BIN_FOLDER)/$(BIN) -d $(SOURCE_FOLDER)

$(BIN_FOLDER)/%_test.ut: $(OBJ_FOLDER)/%_src.o $(OBJ_FOLDER)/%_test.o
	$(CC) $(FLAGS) $(LIBS) -o $@ $^

$(OBJ_LINUX)/%_test.o: $(SOURCE_TEST_FOLDER)/%_test.c
	$(CC) $(FLAGS) -o $@ -c $<

test: $(BIN_FOLDER) $(OBJ_FOLDER) $(BIN_TESTS)
	@for test in $(BIN_TESTS); do $$test; done
