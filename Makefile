# sm-dbi-mysql v2: build dbi.mysql.ext.so (32-bit Linux) against one SourceMod SDK.
#
# Use scripts/prepare.sh first: it assembles work/ from vendor/ (pinned upstream
# extensions/mysql) and overlay/ (this project's changes). Do not build from vendor/.
#
#   make -C work SMSDK=... MMSOURCE=... MARIADB=... OPENSSL=... SM_LABEL=sm17

SMSDK     ?= ../sourcemod
MMSOURCE  ?= ../mmsource
MARIADB   ?= ../mariadb-connector-c-3.4.9-x86
OPENSSL   ?= ../openssl-3.0.22-install
SM_LABEL  ?= sm17

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
	-fvisibility=hidden -fPIC \
	-DSM_DBI_LABEL=\"$(SM_LABEL)\"
CPPFLAGS = -Wno-non-virtual-dtor -fno-exceptions -fno-rtti -fvisibility-inlines-hidden -std=c++17

INCLUDE = -I. -Isdk \
	-I$(SMSDK)/public \
	-I$(SMSDK)/public/sourcepawn \
	-I$(SMSDK)/sourcepawn/include \
	-I$(SMSDK)/public/amtl \
	-I$(SMSDK)/public/amtl/amtl \
	-I$(MMSOURCE)/core/sourcehook \
	-I$(MARIADB)/include/mariadb \
	-I$(OPENSSL)/include

# Static everything except glibc: MariaDB C/C 3.4.9 (LGPL-2.1) and OpenSSL 3.0 (Apache-2.0).
# Only GetSMExtAPI is exported (exports.map).
LINK = -m32 -shared -static-libgcc \
	-Wl,--version-script=exports.map \
	-Wl,-Bsymbolic \
	$(MARIADB)/lib/libmariadbclient.a \
	$(OPENSSL)/lib/libssl.a $(OPENSSL)/lib/libcrypto.a \
	-lz -lpthread -lrt -ldl -lm

# SourceMod 1.5 - 1.7 SDKs predate ke::RefPtr and ke::SafeSprintf (see overlay/amtl_compat.h).
ifneq (,$(filter sm15 sm16 sm17,$(SM_LABEL)))
CPPFLAGS += -include $(CURDIR)/amtl_compat.h
endif

ifeq (sm15,$(SM_LABEL))
# SourceMod 1.5 ships no amtl: stand-in for am-refcounting-threadsafe.h.
INCLUDE += -Icompat-sm15
endif

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

clean:
	rm -rf build
