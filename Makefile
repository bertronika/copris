
## Build system for COPRIS
## Common targets (you don't need to specify one for a regular release build):
##   - release     build the release build (executable name 'copris')
##   - debug       build the debugging build (executable name 'copris_dbg')
##   - install     install a copy of the release build with samples and documentation
##   - clean       remove object and dependency files
##   - distclean   remove object, dependency, binary and temporary test files
##   - help        print this text

## Code analysis targets (more are present in 'tests/Makefile'):
##   - check                   build and run unit tests
##   - analyse                 analyse object files with with GCC's static analyser
##   - analyse-cppcheck        analyse codebase with Cppcheck, print results to stdout
##   - analyse-cppcheck-html   analyse codebase with Cppcheck, generate a HTML report

## You may use the USERFLAGS variable to specify custom compiler/linker flags
## e.g.  make USERFLAGS=-std=c99 debug

# GNU Make docs: https://www.gnu.org/software/make/manual/html_node/index.html

include Makefile-common.mk

# Installation directories
PREFIX  ?= /usr/local
BINDIR  ?= $(PREFIX)/bin
DATADIR ?= $(PREFIX)/share
MANDIR  ?= $(PREFIX)/man
DESTDIR ?=

INSTALL ?= install -p

# Object and dependency files for release and debug builds
OBJS_REL := $(OBJECTS:%.o=%_rel.o)
DEPS_REL := $(OBJECTS:%.o=%_rel.d)
OBJS_DBG := $(OBJECTS:%.o=%_dbg.o)
DEPS_DBG := $(OBJECTS:%.o=%_dbg.d)

# Cppcheck settings. Note that 'style' includes 'warning', 'performance' and 'portability'.
CPPCHECK_DIR   = cppcheck_report
CPPCHECK_XML   = $(CPPCHECK_DIR)/report.xml
CPPCHECK_FLAGS = --cppcheck-build-dir=$(CPPCHECK_DIR) --include=<(grep VERSION copris.config) \
                 --enable=style,information,missingInclude --suppress=missingIncludeSystem \
                 --check-level=exhaustive

# Targets that do not produce an eponymous file
.PHONY: check-if-tagged release debug clean distclean help \
        check analyse analyse-cppcheck analyse-cppcheck-html doc \
        install install-copris install-intercopris install-encodings \
        $(CPPCHECK_DIR)/index.html

all:   | check-if-tagged release
release: copris
debug:   copris_dbg

# Automatic variables of GNU Make:
# $@  The file name of the target of the rule.
# $<  The name of the first prerequisite.
# $^  The names of all the prerequisites, with spaces between them.

check-if-tagged:
	@test -n "$$(git tag --points-at HEAD || echo 1)"                               || ( \
	echo "Warning: It seems you are trying to build COPRIS from the master branch,"   && \
	echo "         which may contain untested and non-functional features. Instead, " && \
	echo "         download a numbered release or checkout a tag in the repository."  && \
	echo -e "\nTo proceed anyway, run make with the appropriate recipe.\n"            && \
	false )

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
$(CPPCHECK_DIR)/index.html:
	mkdir -p $(CPPCHECK_DIR)
	cppcheck $(CPPCHECK_FLAGS) --xml src/ 2>$(CPPCHECK_XML)
	cppcheck-htmlreport --file=$(CPPCHECK_XML) --report-dir=$(CPPCHECK_DIR)

# Intercopris binary
intercopris intercopris_dbg: LDFLAGS += -lreadline
intercopris: src/feature_rel.o src/main-helpers_rel.o src/parse_value_rel.o src/writer_rel.o \
             src/intercopris_rel.o
	$(CC) $^ $(LDFLAGS) -o $@

intercopris_dbg: src/feature_dbg.o src/main-helpers_dbg.o src/parse_value_dbg.o src/writer_dbg.o \
                 src/intercopris_dbg.o
	$(CC) $^ $(LDFLAGS) -o $@

# Building intercopris requires linking to readline
src/intercopris_rel.o src/intercopris_dbg.o: LIBRARIES += readline

# Automatic dependency tracking (-MMD -MP). If a header file is
# changed, only the files including it will be recompiled.
-include $(DEPS_REL) $(DEPS_DBG)

install: install-copris install-encodings

install-copris: copris man/copris.1
	mkdir -p $(DESTDIR)$(BINDIR) $(DESTDIR)$(MANDIR)/man1
	$(INSTALL) -m755 copris $(DESTDIR)$(BINDIR)
	$(INSTALL) -m644 man/copris.1 $(DESTDIR)$(MANDIR)/man1

install-intercopris: intercopris man/intercopris.1
	mkdir -p $(DESTDIR)$(BINDIR) $(DESTDIR)$(MANDIR)/man1
	$(INSTALL) -m755 intercopris $(DESTDIR)$(BINDIR)
	$(INSTALL) -m644 man/intercopris.1 $(DESTDIR)$(MANDIR)/man1

install-encodings: $(wildcard encodings/*.ini)
	mkdir -p $(DESTDIR)$(DATADIR)/copris/encodings
	$(INSTALL) -m644 -t $(DESTDIR)$(DATADIR)/copris $^

clean:
	rm -f $(OBJS_REL) $(DEPS_REL) src/intercopris_rel.o src/intercopris_rel.d
	rm -f $(OBJS_DBG) $(DEPS_DBG) src/intercopris_dbg.o src/intercopris_dbg.d

distclean: clean
	rm -f copris copris_dbg intercopris intercopris_dbg
	rm -fr $(CPPCHECK_DIR)
	$(MAKE) -C tests/ clean

help:
	@grep -A 1 '^##' Makefile; \
	 grep -F -m 4 -B 1 -A 2 'FLAGS' Makefile-common.mk
	# Default installation prefix (overridable with PREFIX=<path>): $(PREFIX)

doc:
	$(MAKE) -C release-tools
