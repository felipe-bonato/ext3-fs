C_CPP = g++
C_FLAGS = -Wall -Wextra -Wno-unused-parameter -Wpedantic
C_FLAGS_DEBUG = -g3 -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls
C_LANG_VERSION = c++17
C_LIBS = -lcrypto -lgtest -lpthread

PATH_SRC_FILES = main.cpp fs.cpp sha256.cpp
PATH_OUT_BIN = out
PATH_OUT_BIN_EXTENTION = 

SYS_EXEC_CMD = ./

# Target specific outputs
PATH_OUT_BIN_TARGET_DEV     = $(PATH_OUT_BIN)_dev$(PATH_OUT_BIN_EXTENTION)
PATH_OUT_BIN_TARGET_DEBUG   = $(PATH_OUT_BIN)_debug$(PATH_OUT_BIN_EXTENTION)
PATH_OUT_BIN_TARGET_RELEASE = $(PATH_OUT_BIN)_release$(PATH_OUT_BIN_EXTENTION)

# Target specific flags
C_FLAGS_TARGET_DEV     = -std=$(C_LANG_VERSION) $(C_FLAGS) $(C_LIBS) -O1
C_FLAGS_TARGET_DEBUG   = -std=$(C_LANG_VERSION) $(C_FLAGS) $(C_LIBS) -O0 $(C_FLAGS_DEBUG)
C_FLAGS_TARGET_RELEASE = -std=$(C_LANG_VERSION) $(C_FLAGS) $(C_LIBS) -O3 -Werror

build_dev: $(PATH_SRC_FILES)
	$(C_CPP) $(PATH_SRC_FILES) $(C_FLAGS_TARGET_DEV) -o $(PATH_OUT_BIN_TARGET_DEV)

build_debug: $(PATH_SRC_FILES)
	$(C_CPP) $(PATH_SRC_FILES) $(C_FLAGS_TARGET_DEBUG) -o $(PATH_OUT_BIN_TARGET_DEBUG)  

build_release: $(PATH_SRC_FILES)
	$(C_CPP) $(PATH_SRC_FILES) $(C_FLAGS_TARGET_RELEASE) -o $(PATH_OUT_BIN_TARGET_RELEASE)

run_dev: build_dev $(PATH_OUT_BIN_TARGET_DEV)
	$(SYS_EXEC_CMD)$(PATH_OUT_BIN_TARGET_DEV)

run_debug: build_debug $(PATH_OUT_BIN_TARGET_DEV)
	$(SYS_EXEC_CMD)$(PATH_OUT_BIN_TARGET_DEV)

run_release: build_release $(PATH_OUT_BIN_TARGET_DEV)
	$(SYS_EXEC_CMD)$(PATH_OUT_BIN_TARGET_DEV)

.PHONY: clean
clean:
	rm $(PATH_OUT_BIN_TARGET_DEV) $(PATH_OUT_BIN_TARGET_DEBUG) $(PATH_OUT_BIN_TARGET_RELEASE)
