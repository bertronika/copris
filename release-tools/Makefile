# Internal release tools for documentation generation and packaging.
#
# Order of documentation processing is as follows:
#  DESTINATION    INTERMEDIATE STEP           SOURCE
#  README.md                                  release-tools/README.md.m4
#  man/copris.1   release-tools/copris.1.md   release-tools/copris.1.md.m4

README   = ../README.md
MANPAGE  = ../man/copris.1
DOCS     = $(README) $(MANPAGE)
INTERMED = README.md copris.1 copris.1.md

.PHONY: gen-docs clean
.INTERMEDIATE: $(INTERMED)

gen-docs: $(DOCS)

$(README): README.md usage_examples.md.m4
$(MANPAGE): copris.1 README.md usage_examples.md.m4

$(DOCS):
	cp $< $@

copris.1: copris.1.md
	pandoc --standalone --to man $< --output $@

%.md: %.md.m4
	m4 -PEE $< > $@

clean:
	rm -f $(INTERMED)
