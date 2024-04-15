
CC=clang
CCW=x86_64-w64-mingw32-gcc
RSW=x86_64-w64-mingw32-windres
ARW=x86_64-w64-mingw32-ar

BIN=line-lancer
LIB=lancer

OBJ_FOLDER=cache
SOURCE_FOLDER=src
SOURCE_TEST_FOLDER=tests
BIN_FOLDER=bin
LIBS_PATH_WIN=lib/windows
LIBS_PATH_LNX=lib/linux
LIBS_PATH_AND=lib/android

RAYLIB_NAME=libraylib.a
RAYLIB_WIN=$(LIBS_PATH_WIN)/$(RAYLIB_NAME)
RAYLIB_LNX=$(LIBS_PATH_LNX)/$(RAYLIB_NAME)

ANDROID_VERSION = 29
ANDROID_NDK_PATH = $(ANDROID_NDK)
ANDROID_SDK_PATH = $(ANDROID_SDK)
ANDROID_APP_GLUE = $(ANDROID_NDK_PATH)/sources/android/native_app_glue
ANDROID_BUILD_TOOLS = $(ANDROID_SDK_PATH)/build-tools/$(ANDROID_VERSION).0.3
ANDROID_TOOLCHAIN = $(ANDROID_NDK_PATH)/toolchains/llvm/prebuilt/linux-x86_64
ANDROID_SYSROOT = $(ANDROID_TOOLCHAIN)/sysroot/usr
CCAA64 = $(ANDROID_TOOLCHAIN)/bin/aarch64-linux-android$(ANDROID_VERSION)-clang
CCAA32 = $(ANDROID_TOOLCHAIN)/bin/armv7a-linux-androideabi$(ANDROID_VERSION)-clang
CCAX86 = $(ANDROID_TOOLCHAIN)/bin/i686-linux-android$(ANDROID_VERSION)-clang
CCAX64 = $(ANDROID_TOOLCHAIN)/bin/x86_64-linux-android$(ANDROID_VERSION)-clang

FLAGS_LNX    ?= -Wall -Wextra -Warray-bounds -Wshadow -Wunused -Wdeprecated
FLAGS_WIN    ?= -Wall -Wextra -Warray-bounds -Wshadow -Wunused -Wdeprecated
FLAGS_AND    ?= -Wall -Wextra -Warray-bounds -Wshadow -Wunused -Wdeprecated \
				-ffunction-sections -funwind-tables -fstack-protector-strong \
				-fPIC -Wformat -Werror=format-security -no-canonical-prefixes \
				-DANDROID -DPLATFORM_ANDROID -D__ANDROID_API__=$(ANDROID_VERSION) -DRELEASE -DEMBEDED_ASSETS -O3
FLAGS_AND_LD ?= -shared --exclude-libs libatomic.a --build-id \
		 		-z noexecstack -z relro -z now \
				--warn-shared-textrel --fatal-warnings -u ANativeActivity_onCreate \

DEBUG_FLAGS   = -ggdb
RELEASE_FLAGS = -Wl,-s -O3 -DRELEASE

INCLUDES = -I "vendor/raylib/src"
INCLUDES_AND = -Ivendor/raylib/src -I$(ANDROID_SYSROOT)/include

LIBS=-lm
LIBW=-lm -lgdi32 -lwinmm
LIBA=-lraylib -lnative_app_glue -llog -landroid -lEGL -lGLESv2 -lOpenSLES -latomic -lc -lm -ldl

SOURCES=$(wildcard $(SOURCE_FOLDER)/*.c)
TESTS=$(wildcard $(SOURCE_TEST_FOLDER)/*_test.c)

OBJECTS_LINUX = $(patsubst $(SOURCE_FOLDER)/%.c, $(OBJ_FOLDER)/%_lnx.o, $(SOURCES))
OBJECTS_WIN   = $(patsubst $(SOURCE_FOLDER)/%.c, $(OBJ_FOLDER)/%_win.o, $(SOURCES))
OBJECTS_AND_ARM64 = $(patsubst $(SOURCE_FOLDER)/%.c, $(OBJ_FOLDER)/%_anda64.o, $(SOURCES))
OBJECTS_AND_ARM32 = $(patsubst $(SOURCE_FOLDER)/%.c, $(OBJ_FOLDER)/%_anda32.o, $(SOURCES))
OBJECTS_AND_x86   = $(patsubst $(SOURCE_FOLDER)/%.c, $(OBJ_FOLDER)/%_andx86.o, $(SOURCES))
OBJECTS_AND_x64   = $(patsubst $(SOURCE_FOLDER)/%.c, $(OBJ_FOLDER)/%_andx64.o, $(SOURCES))
TEST_OBJECTS  = $(patsubst $(SOURCE_TEST_FOLDER)/%_test.c, $(OBJECT_LINUX)/%.o, $(TESTS)) $(patsubst $(SOURCE_TEST_FOLDER)/%.c, $(OBJ_LINUX)/%.o, $(TESTS))

BIN_TESTS=$(patsubst $(SOURCE_TEST_FOLDER)/%.c, $(BIN_FOLDER)/%.ut, $(TESTS))

# LINUX #######################################################################
build: $(BIN_FOLDER)/$(BIN)

run: build
	$(BIN_FOLDER)/$(BIN)

rebuild: clean build

rerun: clean run

$(BIN_FOLDER)/$(BIN): $(BIN_FOLDER) $(OBJECTS_LINUX) $(RAYLIB_LNX)
	$(CC) $(FLAGS_LNX) -L$(LIBS_PATH_LNX)/ -o $@ $(OBJECTS_LINUX) -l:$(RAYLIB_NAME) $(LIBS) $(INCLUDES)

$(OBJ_FOLDER)/%_lnx.o: $(SOURCE_FOLDER)/%.c $(OBJ_FOLDER)
	$(CC) $(FLAGS_LNX) -o $@ -c $< $(INCLUDES)

# WINDOWS #####################################################################
build-win: $(BIN_FOLDER)/$(BIN).exe

$(OBJ_FOLDER)/%_win.o: $(SOURCE_FOLDER)/%.c $(OBJ_FOLDER)
	$(CCW) $(FLAGS_WIN) -o $@ -c $< $(INCLUDES)

$(BIN_FOLDER)/$(BIN).exe: $(BIN_FOLDER) $(OBJECTS_WIN) $(RAYLIB_WIN) $(OBJ_FOLDER)/winres.res
	$(CCW) $(FLAGS_WIN) -L$(LIBS_PATH_WIN) -o $@ $(OBJECTS_WIN) $(OBJ_FOLDER)/winres.res -l:$(RAYLIB_NAME) $(LIBW) $(INCLUDES) -Wl,--subsystem,windows

$(OBJ_FOLDER)/winres.res:
	$(RSW) deploy/windows/resources.rs -O coff -o $(OBJ_FOLDER)/winres.res

# ANDROID #####################################################################
build-android: lib/arm64-v8a/lib$(LIB).so lib/armeabi-v7a/lib$(LIB).so lib/x86/lib$(LIB).so lib/x86_64/lib$(LIB).so

install-android: pack/linelancer.apk
	$(ANDROID_SDK)/platform-tools/adb install -r pack/linelancer.apk

$(OBJ_FOLDER)/%_anda64.o: $(SOURCE_FOLDER)/%.c $(OBJ_FOLDER)
	$(CCAA64) -o $@ -c $< $(INCLUDES_AND) -I$(ANDROID_SYSROOT)/include/aarch64-linux-android $(FLAGS_AND) -std=c99 -target aarch64 -mfix-cortex-a53-835769

$(OBJ_FOLDER)/%_anda32.o: $(SOURCE_FOLDER)/%.c $(OBJ_FOLDER)
	$(CCAA32) -o $@ -c $< $(INCLUDES_AND) -I$(ANDROID_SYSROOT)/include/armv7a-linux-androideabi $(FLAGS_AND) -std=c99 -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16

$(OBJ_FOLDER)/%_andx86.o: $(SOURCE_FOLDER)/%.c $(OBJ_FOLDER)
	$(CCAX86) -o $@ -c $< $(INCLUDES_AND) -I$(ANDROID_SYSROOT)/include/i686-linux-android $(FLAGS_AND)

$(OBJ_FOLDER)/%_andx64.o: $(SOURCE_FOLDER)/%.c $(OBJ_FOLDER)
	$(CCAX64) -o $@ -c $< $(INCLUDES_AND) -I$(ANDROID_SYSROOT)/include/x86_64-linux-android $(FLAGS_AND)

$(OBJ_FOLDER)/native_app_glue.arm64.o: $(ANDROID_APP_GLUE)/android_native_app_glue.c
	$(CCAA64) -c $< -o $@ $(INCLUDES_AND) -I$(ANDROID_SYSROOT)/include/aarch64-linux-android $(FLAGS_AND) -std=c99 -target aarch64 -mfix-cortex-a53-835769

$(OBJ_FOLDER)/native_app_glue.arm32.o: $(ANDROID_APP_GLUE)/android_native_app_glue.c
	$(CCAA32) -c $< -o $@ $(INCLUDES_AND) -I$(ANDROID_SYSROOT)/include/armv7a-linux-androideabi $(FLAGS_AND) -std=c99 -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16

$(OBJ_FOLDER)/native_app_glue.x86.o: $(ANDROID_APP_GLUE)/android_native_app_glue.c
	$(CCAX86) -c $< -o $@ $(INCLUDES_AND) -I$(ANDROID_SYSROOT)/include/i686-linux-android $(FLAGS_AND)

$(OBJ_FOLDER)/native_app_glue.x64.o: $(ANDROID_APP_GLUE)/android_native_app_glue.c
	$(CCAX64) -c $< -o $@ $(INCLUDES_AND) -I$(ANDROID_SYSROOT)/include/x86_64-linux-android $(FLAGS_AND)

$(LIBS_PATH_AND)/%/libnative_app_glue.a: $(OBJ_FOLDER)/native_app_glue.%.o
	$(ANDROID_TOOLCHAIN)/bin/llvm-ar rcs $@ $<

lib/arm64-v8a/lib$(LIB).so: $(OBJECTS_AND_ARM64) $(LIBS_PATH_AND) $(LIBS_PATH_AND)/arm64/$(RAYLIB_NAME) $(LIBS_PATH_AND)/arm64/libnative_app_glue.a
	mkdir -p lib/arm64-v8a/
	$(ANDROID_TOOLCHAIN)/bin/ld.lld $(OBJECTS_AND_ARM64) -o $@ $(FLAGS_AND_LD) \
		-L$(ANDROID_SYSROOT)/lib/aarch64-linux-android/$(ANDROID_VERSION) \
		-L$(ANDROID_TOOLCHAIN)/lib/clang/17/lib/linux/aarch64 \
		-L$(LIBS_PATH_AND)/arm64 $(LIBA) \

lib/armeabi-v7a/lib$(LIB).so: $(OBJECTS_AND_ARM32) $(LIBS_PATH_AND) $(LIBS_PATH_AND)/arm32/$(RAYLIB_NAME) $(LIBS_PATH_AND)/arm32/libnative_app_glue.a
	mkdir -p lib/armeabi-v7a/
	$(ANDROID_TOOLCHAIN)/bin/ld.lld $(OBJECTS_AND_ARM32) -o $@ $(FLAGS_AND_LD) \
		-L$(ANDROID_SYSROOT)/lib/arm-linux-androideabi/$(ANDROID_VERSION) \
		-L$(ANDROID_TOOLCHAIN)/lib/clang/17/lib/linux/arm \
		-L$(LIBS_PATH_AND)/arm32 $(LIBA) \

lib/x86/lib$(LIB).so: $(OBJECTS_AND_X86) $(LIBS_PATH_AND) $(LIBS_PATH_AND)/x86/$(RAYLIB_NAME) $(LIBS_PATH_AND)/x86/libnative_app_glue.a
	mkdir -p lib/x86/
	$(ANDROID_TOOLCHAIN)/bin/ld.lld $(OBJECTS_AND_X86) -o $@ $(FLAGS_AND_LD) \
		-L$(ANDROID_SYSROOT)/lib/i686-linux-android/$(ANDROID_VERSION) \
		-L$(ANDROID_TOOLCHAIN)/lib/clang/17/lib/linux/i386 \
		-L$(LIBS_PATH_AND)/x86 $(LIBA) \

lib/x86_64/lib$(LIB).so: $(OBJECTS_AND_X64) $(LIBS_PATH_AND) $(LIBS_PATH_AND)/x64/$(RAYLIB_NAME) $(LIBS_PATH_AND)/x64/libnative_app_glue.a
	mkdir -p lib/x86_64
	$(ANDROID_TOOLCHAIN)/bin/ld.lld $(OBJECTS_AND_X64) -o $@ $(FLAGS_AND_LD) \
		-L$(ANDROID_SYSROOT)/lib/x86_64-linux-android/$(ANDROID_VERSION) \
		-L$(ANDROID_TOOLCHAIN)/lib/clang/17/lib/linux/x86_64 \
		-L$(LIBS_PATH_AND)/x64 $(LIBA) \

$(SOURCE_FOLDER)/packed_assets.h: $(BIN_FOLDER)/asset_packer
	$(BIN_FOLDER)/asset_packer assets/*/* > $@

$(LIBS_PATH_AND)/line-lancer.keystore: $(LIBS_PATH_AND)
	keytool -genkeypair -validity 10000 -dname "CN=linelancer,O=Android,C=ES" -keystore $@ -storepass 'lancer' -keypass 'lancer' -alias projectKey -keyalg RSA

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

pack-android: pack/linelancer.apk

pack/linelancer.apk: $(LIBS_PATH_AND)/line-lancer.keystore $(SOURCE_FOLDER)/packed_assets.h lib/arm64-v8a/lib$(LIB).so lib/armeabi-v7a/lib$(LIB).so lib/x86/lib$(LIB).so lib/x86_64/lib$(LIB).so
	if [ -d "pack/android" ]; then rm -rf pack/android; fi
	mkdir -p pack/android/src/com/linelancer/game/
	mkdir -p pack/android/obj
	mkdir -p pack/android/dex
	mkdir -p pack/android/assets
	mkdir -p pack/android/res
	cp deploy/android/play_store_512.png pack/android/
	cp -r deploy/android/res/* pack/android/res/
	cp deploy/android/NativeLoader.java pack/android/src/com/linelancer/game/NativeLoader.java
	cp deploy/android/AndroidManifest.xml pack/android/AndroidManifest.xml

	$(ANDROID_BUILD_TOOLS)/aapt package -f -m -S pack/android/res -J pack/android/src \
		-M pack/android/AndroidManifest.xml -I $(ANDROID_SDK_PATH)/platforms/android-$(ANDROID_VERSION)/android.jar

	javac -source 1.8 -target 1.8 -d pack/android/obj \
		-bootclasspath jre/lib/rt.jar \
		-classpath $(ANDROID_SDK_PATH)/platforms/android-$(ANDROID_VERSION)/android.jar:pack/android/obj \
		-sourcepath src pack/android/src/com/linelancer/game/*.java

	$(ANDROID_BUILD_TOOLS)/dx --dex --output=pack/android/dex/classes.dex pack/android/obj

	$(ANDROID_BUILD_TOOLS)/aapt package -f \
		-M pack/android/AndroidManifest.xml -S pack/android/res  \
		-I $(ANDROID_SDK_PATH)/platforms/android-$(ANDROID_VERSION)/android.jar -F pack/linelancer.apk pack/android/dex

	$(ANDROID_BUILD_TOOLS)/aapt add pack/linelancer.apk lib/arm64-v8a/lib$(LIB).so
	$(ANDROID_BUILD_TOOLS)/aapt add pack/linelancer.apk lib/armeabi-v7a/lib$(LIB).so
	$(ANDROID_BUILD_TOOLS)/aapt add pack/linelancer.apk lib/x86/lib$(LIB).so
	$(ANDROID_BUILD_TOOLS)/aapt add pack/linelancer.apk lib/x86_64/lib$(LIB).so

	jarsigner -keystore $(LIBS_PATH_AND)/line-lancer.keystore -storepass lancer -keypass lancer \
		-signedjar pack/linelancer.apk pack/linelancer.apk projectKey

	$(ANDROID_BUILD_TOOLS)/zipalign -f 4 pack/linelancer.apk pack/line.apk
	mv -f pack/line.apk pack/linelancer.apk

# STRUCTURE ###################################################################
$(BIN_FOLDER):
	mkdir -p $(BIN_FOLDER)

$(OBJ_FOLDER):
	mkdir -p $(OBJ_FOLDER)

$(LIBS_PATH_WIN):
	mkdir -p $(LIBS_PATH_WIN)

$(LIBS_PATH_LNX):
	mkdir -p $(LIBS_PATH_LNX)

$(LIBS_PATH_AND):
	mkdir -p $(LIBS_PATH_AND)/arm64
	mkdir -p $(LIBS_PATH_AND)/arm32
	mkdir -p $(LIBS_PATH_AND)/x86
	mkdir -p $(LIBS_PATH_AND)/x64

clean:
	rm -rf $(BIN_FOLDER) $(OBJ_FOLDER) $(LIBS_PATH_WIN) $(LIBS_PATH_LNX) $(LIBS_PATH_AND)

cleanrl:
	make -C vendor/raylib/src clean

clean-packs:
	rm -rf pack

cleanall: clean clean-packs cleanrl

# TOOLS #######################################################################
build-tools: $(BIN_FOLDER)/asset_packer

$(BIN_FOLDER)/asset_packer: tools/asset_packer.c $(BIN_FOLDER) $(RAYLIB_LNX)
	$(CC) $(FLAGS_LNX) -L$(LIBS_PATH_LNX)/ -o $@ $< -l:$(RAYLIB_NAME) $(LIBS) $(INCLUDES)

# DEPENDENCIES ################################################################
build-raylib:
	make $(RAYLIB_LNX)
	make $(RAYLIB_WIN)

build-raylib-lnx:
	make $(RAYLIB_LNX)

build-raylib-win:
	make $(RAYLIB_WIN)

build-raylib-android: $(LIBS_PATH_AND)/arm64/$(RAYLIB_NAME) $(LIBS_PATH_AND)/arm32/$(RAYLIB_NAME) $(LIBS_PATH_AND)/x86/$(RAYLIB_NAME) $(LIBS_PATH_AND)/x64/$(RAYLIB_NAME)

vendor/raylib/:
	git submodule init
	git submodule update --recursive

$(RAYLIB_LNX): $(LIBS_PATH_LNX) vendor/raylib/
	make -C vendor/raylib/src clean
	make -C vendor/raylib/src PLATFORM=PLATFORM_DESKTOP GRAPHICS=GRAPHICS_API_OPENGL_21 RAYLIB_RELEASE_PATH="../../../$(LIBS_PATH_LNX)"

$(RAYLIB_WIN): $(LIBS_PATH_WIN) vendor/raylib/
	make -C vendor/raylib/src clean
	make -C vendor/raylib/src PLATFORM=PLATFORM_DESKTOP GRAPHICS=GRAPHICS_API_OPENGL_21 CC=$(CCW) AR=$(ARW) RAYLIB_RELEASE_PATH="../../../$(LIBS_PATH_WIN)"

$(LIBS_PATH_AND)/arm64/$(RAYLIB_NAME): $(LIBS_PATH_AND) vendor/raylib
	make -C vendor/raylib/src clean
	make -C vendor/raylib/src PLATFORM=PLATFORM_ANDROID ANDROID_NDK=$(ANDROID_NDK_PATH) ANDROID_ARCH=arm64 ANDROID_API_VERSION=$(ANDROID_VERSION) RAYLIB_RELEASE_PATH=../../../$(LIBS_PATH_AND)/arm64

$(LIBS_PATH_AND)/arm32/$(RAYLIB_NAME): $(LIBS_PATH_AND) vendor/raylib
	make -C vendor/raylib/src clean
	make -C vendor/raylib/src PLATFORM=PLATFORM_ANDROID ANDROID_NDK=$(ANDROID_NDK_PATH) ANDROID_ARCH=arm ANDROID_API_VERSION=$(ANDROID_VERSION) RAYLIB_RELEASE_PATH=../../../$(LIBS_PATH_AND)/arm32

$(LIBS_PATH_AND)/x86/$(RAYLIB_NAME): $(LIBS_PATH_AND) vendor/raylib
	make -C vendor/raylib/src clean
	make -C vendor/raylib/src PLATFORM=PLATFORM_ANDROID ANDROID_NDK=$(ANDROID_NDK_PATH) ANDROID_ARCH=x86 ANDROID_API_VERSION=$(ANDROID_VERSION) RAYLIB_RELEASE_PATH=../../../$(LIBS_PATH_AND)/x86

$(LIBS_PATH_AND)/x64/$(RAYLIB_NAME): $(LIBS_PATH_AND) vendor/raylib
	make -C vendor/raylib/src clean
	make -C vendor/raylib/src PLATFORM=PLATFORM_ANDROID ANDROID_NDK=$(ANDROID_NDK_PATH) ANDROID_ARCH=x86_64 ANDROID_API_VERSION=$(ANDROID_VERSION) RAYLIB_RELEASE_PATH=../../../$(LIBS_PATH_AND)/x64

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
