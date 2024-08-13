# Internal release tools for documentation generation and packaging.

README = ../README.md
MANPAGE    = ../man/copris.1
MANPAGE_IC = ../man/intercopris.1

.PHONY: all readme manpage manpage_ic

all: readme manpage manpage_ic
readme: $(README)
manpage: $(MANPAGE) $(MANPAGE).txt
manpage_ic: $(MANPAGE_IC) $(MANPAGE_IC).txt

$(README): README.md.m4 common-description.md.m4 common-usage.md.m4
	m4 -PEE $< | fmt -s -w 100 > $@

$(MANPAGE): copris.1.md.m4 common-description.md.m4 common-usage.md.m4
	m4 -PEE $< | pandoc --standalone --to man > $@

$(MANPAGE_IC): intercopris.1.md.m4
	m4 -PEE $< | pandoc --standalone --to man > $@

%.1.txt: %.1
	MANWIDTH=100 man -l $< | col -bx > $@
