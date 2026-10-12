# (C) AlliedModders / fmu1337
# Build dbi.mysql.ext.so (32-bit) against SMSDK + MariaDB Connector/C

SMSDK       ?= ../sourcemod
MMSOURCE    ?= ../mmsource
MARIADB     ?= ../mariadb-connector-c-3.4.9-x86
SM_LABEL    ?= sm17

PROJECT = dbi.mysql
BINARY  = $(PROJECT).ext.so

OBJECTS = \
	sdk/smsdk_ext.cpp \
	extension.cpp \
	mysql/MyBasicResults.cpp \
	mysql/MyBoundResults.cpp \
	mysql/MyDatabase.cpp \
	mysql/MyDriver.cpp \
	mysql/MyStatement.cpp \
	mysql/MysqlSslConfig.cpp

CPP = g++
BIN_DIR = build/$(SM_LABEL)

C_OPT_FLAGS = -DNDEBUG -O2 -funroll-loops -pipe -fno-strict-aliasing
CFLAGS = -D_LINUX -Dstricmp=strcasecmp -D_stricmp=strcasecmp -D_strnicmp=strncasecmp \
	-Dstrnicmp=strncasecmp -D_snprintf=snprintf -D_vsnprintf=vsnprintf -D_alloca=alloca \
	-Dstrcmpi=strcasecmp -Wall -Wno-unused-parameter -Wno-switch -Wno-unknown-pragmas \
	-mfpmath=sse -msse -DSOURCEMOD_BUILD -DHAVE_STDINT_H -m32 \
	-fvisibility=hidden -fPIC
CPPFLAGS = -Wno-non-virtual-dtor -fno-exceptions -fno-rtti -fvisibility-inlines-hidden -std=c++11

INCLUDE = -I. -Isdk \
	-I$(SMSDK)/public \
	-I$(SMSDK)/public/sourcepawn \
	-I$(SMSDK)/sourcepawn/include \
	-I$(SMSDK)/public/amtl \
	-I$(SMSDK)/public/amtl/amtl \
	-I$(MMSOURCE)/core/sourcehook \
	-I$(MARIADB)/include/mariadb

LINK = -m32 -shared -static-libgcc \
	-Wl,--version-script=exports.map \
	-Wl,-Bsymbolic \
	$(MARIADB)/lib/libmariadbclient.a \
	-Wl,-Bstatic -lssl -lcrypto -Wl,-Bdynamic \
	-lz -lpthread -lrt -ldl -lm -lstdc++

OBJ_FILES := $(OBJECTS:%.cpp=$(BIN_DIR)/%.o)

.PHONY: all clean dirs extension

all: dirs extension

dirs:
	mkdir -p $(BIN_DIR)/sdk $(BIN_DIR)/mysql

$(BIN_DIR)/%.o: %.cpp
	$(CPP) $(INCLUDE) $(CFLAGS) $(CPPFLAGS) $(C_OPT_FLAGS) -o $@ -c $<

extension: $(OBJ_FILES)
	$(CPP) $(OBJ_FILES) $(LINK) -o $(BIN_DIR)/$(BINARY)
	@echo "Built $(BIN_DIR)/$(BINARY)"
	@ldd $(BIN_DIR)/$(BINARY) || true

clean:
	rm -rf build
