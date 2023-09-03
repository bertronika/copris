# Internal release tools for documentation generation and packaging.

README  = ../README.md
MANPAGE = ../man/copris.1

.PHONY: all readme manpage

all: readme manpage
readme: $(README)
manpage: $(MANPAGE) $(MANPAGE).txt

$(README): README.md.m4 common-description.md.m4 common-usage.md.m4
	m4 -PEE README.md.m4 | fmt -s -w 100 > $@

$(MANPAGE): copris.1.md.m4 common-description.md.m4 common-usage.md.m4
	m4 -PEE copris.1.md.m4 | pandoc --standalone --to man > $@

$(MANPAGE).txt: $(MANPAGE)
	MANWIDTH=100 man -l $< | col -bx > $@
