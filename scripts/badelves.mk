# Hostile ELF fixtures for loader validation (Milestone 15.0 / 15.8)
#
#   make -f badelves.mk SRC=build/user/sample OUT=build/fixtures
#   make -f badelves.mk clean OUT=build/fixtures
#
# ELF32 header offsets:
#    0 magic     4 class      5 data      16 type     18 machine
#   24 entry    28 phOffset  42 phSize    44 phCount

SRC ?= /workspace/build/user/sample/sample
OUT ?= /workspace/root/badelf

# patch,<file>,<octal escapes>,<offset>
patch = printf '$(2)' | dd of=$(1) bs=1 seek=$(3) conv=notrunc status=none

FIXTURES := \
	$(OUT)/notelf    $(OUT)/toosmall  $(OUT)/badmagic  $(OUT)/badclass \
	$(OUT)/badendn   $(OUT)/badtype   $(OUT)/badmach   $(OUT)/entryhi  \
	$(OUT)/entrylo  $(OUT)/phsize    $(OUT)/phcount   $(OUT)/badphdr  \
	$(OUT)/badtrunc  $(OUT)/filesz

.PHONY: all clean list
all: $(FIXTURES)
	@echo "--- fixtures in $(OUT) ---"
	@ls -l $(OUT)

$(OUT):
	@mkdir -p $(OUT)

# --- not an ELF at all ----------------------------------------------------
$(OUT)/notelf: | $(OUT)
	@printf 'this is not an elf file\n' > $@
	@dd if=/dev/zero bs=1 count=128 >> $@ status=none

$(OUT)/toosmall: $(SRC) | $(OUT)
	@head -c 20 $< > $@

# --- truncation -----------------------------------------------------------
$(OUT)/badphdr: $(SRC) | $(OUT)
	@head -c 60 $< > $@

$(OUT)/badtrunc: $(SRC) | $(OUT)
	@head -c $$(( $$(wc -c < $<) / 3 )) $< > $@

# --- ELF header field corruption ------------------------------------------
$(OUT)/badmagic: $(SRC) | $(OUT)
	@cp $< $@ && $(call patch,$@,\000\000\000\000,0)

$(OUT)/badclass: $(SRC) | $(OUT)
	@cp $< $@ && $(call patch,$@,\002,4)

$(OUT)/badendn: $(SRC) | $(OUT)
	@cp $< $@ && $(call patch,$@,\002,5)

$(OUT)/badtype: $(SRC) | $(OUT)
	@cp $< $@ && $(call patch,$@,\001\000,16)

$(OUT)/badmach: $(SRC) | $(OUT)
	@cp $< $@ && $(call patch,$@,\050\000,18)

$(OUT)/entryhi: $(SRC) | $(OUT)
	@cp $< $@ && $(call patch,$@,\000\000\000\300,24)

$(OUT)/entrylo: $(SRC) | $(OUT)
	@cp $< $@ && $(call patch,$@,\020\000\000\000,24)

$(OUT)/phsize: $(SRC) | $(OUT)
	@cp $< $@ && $(call patch,$@,\000\000,42)

$(OUT)/phcount: $(SRC) | $(OUT)
	@cp $< $@ && $(call patch,$@,\377\377,44)

# --- program header corruption: physSegSize > memSegSize ------------------
# reads phOffset from the ELF header, then patches p_filesz at phOffset+16
$(OUT)/filesz: $(SRC) | $(OUT)
	@cp $< $@
	@phoff=$$(od -An -tu4 -j28 -N4 $@ | tr -d ' '); \
	 $(call patch,$@,\377\377\000\000,$$(( $$phoff + 16 )))

clean:
	@rm -f $(FIXTURES)
	@echo "removed fixtures from $(OUT)"

list:
	@echo $(FIXTURES) | tr ' ' '\n'