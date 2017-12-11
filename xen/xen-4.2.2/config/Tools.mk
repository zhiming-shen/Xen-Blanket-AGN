# Prefix and install folder
prefix              := /usr
PREFIX              := $(prefix)
exec_prefix         := ${prefix}
libdir              := ${exec_prefix}/lib
LIBDIR              := $(libdir)

# A debug build of tools?
debug               := y

# Tools path
BISON               := 
FLEX                := 
PYTHON              := python
PYTHON_PATH         := /usr/bin/python
PERL                := /usr/bin/perl
CURL_CONFIG         := 
XML2_CONFIG         := 
BASH                := /bin/sh
XGETTTEXT           := /usr/bin/xgettext
AS86                := /usr/bin/as86
LD86                := /usr/bin/ld86
BCC                 := /usr/bin/bcc
IASL                := /usr/bin/iasl

# Extra folder for libs/includes
PREPEND_INCLUDES    := 
PREPEND_LIB         := 
APPEND_INCLUDES     := 
APPEND_LIB          := 

PTHREAD_CFLAGS      := -pthread
PTHREAD_LDFLAGS     := -pthread
PTHREAD_LIBS        := 

PTYFUNCS_LIBS       := -lutil

# Download GIT repositories via HTTP or GIT's own protocol?
# GIT's protocol is faster and more robust, when it works at all (firewalls
# may block it). We make it the default, but if your GIT repository downloads
# fail or hang, please specify GIT_HTTP=y in your environment.
GIT_HTTP            := n

# Optional components
XENSTAT_XENTOP      := y
VTPM_TOOLS          := n
LIBXENAPI_BINDINGS  := n
OCAML_TOOLS         := y
CONFIG_MINITERM     := n
CONFIG_LOMOUNT      := n
CONFIG_OVMF         := n
CONFIG_ROMBIOS      := y
CONFIG_SEABIOS      := y

#System options
CONFIG_SYSTEM_LIBAIO:= y
ZLIB                :=  -DHAVE_BZLIB -lbz2 -DHAVE_LZMA -llzma
CONFIG_LIBICONV     := n
CONFIG_GCRYPT       := n
EXTFS_LIBS          := -lext2fs
CURSES_LIBS         := -lncurses
