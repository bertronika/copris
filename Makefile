
# Build system for COPRIS
# Common targets (you don't need to specify one for a regular release build):
#   - release     build the release build (executable name `copris')
#   - debug       build the debugging build (executable name `copris_dbg')
#   - install     install a copy of release build with samples and documentation
#   - clean       remove object and dependency files
#   - distclean   remove object, dependency, binary and test files
#   - help        print this text

# Code analysis targets (more are present in `tests/Makefile'):
#   - check                   build and run unit tests
#   - analyse                 analyse object files with with GCC's static analyser
#   - analyse-cppcheck        analyse codebase with Cppcheck, print results to stdout
#   - analyse-cppcheck-html   analyse codebase with Cppcheck, generate a HTML report

# GNU Make docs: https://www.gnu.org/software/make/manual/html_node/index.html

# Get latest version tag if in a git repository, else from a local file
VERSION := $(shell git describe --tags --dirty 2>/dev/null || cat VERSION)

include Common.mk

# Installation directories
PREFIX  ?= /usr/local
BINDIR  ?= $(PREFIX)/bin
DATADIR ?= $(PREFIX)/share
MANDIR  ?= $(PREFIX)/man
DESTDIR ?=

INSTALL ?= install -p

# Object and dependency files for release and debug builds
OBJS_REL := $(SOURCES:%.c=src/%_rel.o)
DEPS_REL := $(SOURCES:%.c=src/%_rel.d)
OBJS_DBG := $(SOURCES:%.c=src/%_dbg.o)
DEPS_DBG := $(SOURCES:%.c=src/%_dbg.d)

# Cppcheck settings. Note that 'style' includes 'warning', 'performance' and 'portability'.
CPPCHECK_DIR   = cppcheck_report
CPPCHECK_XML   = $(CPPCHECK_DIR)/report.xml
CPPCHECK_FLAGS = --cppcheck-build-dir=$(CPPCHECK_DIR) --enable=style,information,missingInclude

# Targets that do not produce an eponymous file
.PHONY: release debug install clean distclean help \
        check analyse analyse-cppcheck analyse-cppcheck-html

all:     copris
release: copris
debug:   copris_dbg

# Automatic variables of GNU Make:
# $@  The file name of the target of the rule.
# $<  The name of the first prerequisite.
# $^  The names of all the prerequisites, with spaces between them.

# Compile the release binary
copris: $(OBJS_REL)
	$(CC) $^ $(LDFLAGS) -o $@

%_rel.o: %.c
	$(CC) $(CFLAGS) $(RELFLAGS) -MMD -MP -c $< -o $@

# Compile the debug binary
copris_dbg: $(OBJS_DBG)
	$(CC) $^ $(LDFLAGS) -o $@

%_dbg.o: %.c
	$(CC) $(CFLAGS) $(DBGFLAGS) -MMD -MP -c $< -o $@

# Call unit tests' Makefile
check:
	$(MAKE) -C tests/ all

# Remove objects first, then recompile them with static analysis
analyse: DBGFLAGS += -fanalyzer
analyse: | clean $(OBJS_DBG)

# Run Cppcheck code analysis (first recipe prints to stdout, second generates a HTML report)
analyse-cppcheck:
	mkdir -p $(CPPCHECK_DIR)
	cppcheck $(CPPCHECK_FLAGS) src/

analyse-cppcheck-html: $(CPPCHECK_DIR)/index.html
$(CPPCHECK_DIR)/index.html: src/
	mkdir -p $(CPPCHECK_DIR)
	cppcheck $(CPPCHECK_FLAGS) --xml src/ 2>$(CPPCHECK_XML)
	cppcheck-htmlreport --file=$(CPPCHECK_XML) --report-dir=$(CPPCHECK_DIR)

# Automatic dependency tracking (-MMD -MP). If a header file is
# changed, only the files including it will be recompiled.
-include $(DEPS_REL) $(DEPS_DBG)

install: copris man/copris.1
	mkdir -p $(DESTDIR)$(BINDIR) $(DESTDIR)$(DATADIR)/copris $(DESTDIR)$(MANDIR)/man1
	$(INSTALL) -m755 copris $(DESTDIR)$(BINDIR)
	$(INSTALL) -m644 man/copris.1 $(DESTDIR)$(MANDIR)/man1

clean:
	rm -f $(OBJS_REL) $(DEPS_REL)
	rm -f $(OBJS_DBG) $(DEPS_DBG)

distclean: clean
	rm -f copris copris_dbg
	rm -fr $(CPPCHECK_DIR)
	$(MAKE) -C tests/ clean

help:
	head -n 16 $(firstword $(MAKEFILE_LIST)); \
	grep -m 4 -C 1 -E '(CFLAGS|RELFLAGS|DBGFLAGS|LDFLAGS)' Makefile.common
	# Default installation prefix (overridable with PREFIX=<path>): $(PREFIX)
