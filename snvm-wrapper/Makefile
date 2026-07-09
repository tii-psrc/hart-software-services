#
# MPFS HSS Embedded Software
#
# Copyright 2019-2021 Microchip Corporation.
#
# SPDX-License-Identifier: MIT
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
#
# eNVM Compression Wrapper

OBJS-snvm-wrapper = \
	$(BINDIR)/snvm-wrapper/mss_entry.o \
	$(BINDIR)/snvm-wrapper/snvm_main.o \
	$(BINDIR)/baremetal/polarfire-soc-bare-metal-library/src/platform/mpfs_hal/common/mss_mtrap.o \
	$(BINDIR)/baremetal/polarfire-soc-bare-metal-library/src/platform/mpfs_hal/common/mss_irq_handler_stubs.o \
	$(BINDIR)/baremetal/polarfire-soc-bare-metal-library/src/platform/mpfs_hal/common/mss_plic.o \
	$(BINDIR)/baremetal/polarfire-soc-bare-metal-library/src/platform/mpfs_hal/common/mss_beu.o \
	$(BINDIR)/baremetal/polarfire-soc-bare-metal-library/src/platform/mpfs_hal/common/mss_clint.o \
	$(BINDIR)/baremetal/polarfire-soc-bare-metal-library/src/platform/mpfs_hal/common/mss_util.o \
#$(BINDIR)/snvm-wrapper/snvm-wrapper_crt.o \

EXTRA_OBJS-snvm-wrapper =

#LINKER_SCRIPT-snvm-wrapper = snvm-wrapper/snvm-wrapper.ld
LINKER_SCRIPT-snvm-wrapper = snvm-wrapper/mpfs-lim.ld


#$(BINDIR)/snvm-wrapper/snvm-wrapper_crt.o:
$(BINDIR)/snvm-wrapper/mss_entry.o:
$(BINDIR)/snvm-wrapper/snvm_main.o: CFLAGS=$(CFLAGS_GCCEXT) -O0 -g3 -fno-omit-frame-pointer -fno-inline -fno-optimize-sibling-calls -fno-ipa-cp -fno-ipa-sra -fno-tree-dce
$(BINDIR)/baremetal/polarfire-soc-bare-metal-library/src/platform/mpfs_hal/common/mss_mtrap.o: CFLAGS=$(CFLAGS_GCCEXT)
$(BINDIR)/baremetal/polarfire-soc-bare-metal-library/src/platform/mpfs_hal/common/mss_irq_handler_stubs.o: CFLAGS=$(CFLAGS_GCCEXT)
$(BINDIR)/baremetal/polarfire-soc-bare-metal-library/src/platform/mpfs_hal/common/mss_plic.o: CFLAGS=$(CFLAGS_GCCEXT)
$(BINDIR)/baremetal/polarfire-soc-bare-metal-library/src/platform/mpfs_hal/common/mss_beu.o: CFLAGS=$(CFLAGS_GCCEXT)
$(BINDIR)/baremetal/polarfire-soc-bare-metal-library/src/platform/mpfs_hal/common/mss_clint.o: CFLAGS=$(CFLAGS_GCCEXT)
$(BINDIR)/baremetal/polarfire-soc-bare-metal-library/src/platform/mpfs_hal/common/mss_util.o: CFLAGS=$(CFLAGS_GCCEXT)
$(BINDIR)/thirdparty/miniz/miniz.o: CFLAGS=$(CFLAGS_GCCEXT) -DMINIZ_NO_STDIO -DMINIZ_NO_TIME
$(BINDIR)/snvm-wrapper/snvm-wrapper_validate_crc.o: CFLAGS=$(CFLAGS_GCCEXT)
$(BINDIR)/$(BOARD_DIR)/uart_device_list.o: CFLAGS=$(CFLAGS_GCCEXT)

define common-boot-mode-programmer-snvm
	-$(RM) -r $(BINDIR)/bootmode2
	[ "$(SC_INSTALL_DIR)" ] || ( echo "SC_INSTALL_DIR environment variable is unset"; exit 1 )
	[ -d $(SC_INSTALL_DIR) ] || ( echo "SoftConsole subdirectory >>$(SC_INSTALL_DIR)<< does not exist."; exit 2 )
	$(ECHO) "$(MPFS_BOOTMODE_PROGRAMMER) --workdir `pwd`/$(BINDIR) --die $(DIE) --package $(PACKAGE) --bootmode $(BOOTMODE_SNVM) $(BOOTMODE_ARGS) $(1) $(2)"
	$(MPFS_BOOTMODE_PROGRAMMER) --workdir `pwd`/$(BINDIR) --die $(DIE) --package $(PACKAGE) --bootmode $(BOOTMODE_SNVM) $(BOOTMODE_ARGS) $(1) $(2)
endef


# MPFS_BOOTMODE_PROGRAMMER_VERSION := $(findstring v3.6,$(shell $(MPFS_BOOTMODE_PROGRAMMER) --help))
MPFS_BOOTMODE_PROGRAMMER_VERSION := $(shell $(MPFS_BOOTMODE_PROGRAMMER) --help 2>&1)
MPFS_BOOTMODE_PROGRAMMER_VERSION := $(word 5, $(MPFS_BOOTMODE_PROGRAMMER_VERSION))

EXPECTED_MPFS_BOOTMODE_PROGRAMMER_VERSION := v3.6
ifneq ($(MPFS_BOOTMODE_PROGRAMMER_VERSION),$(EXPECTED_MPFS_BOOTMODE_PROGRAMMER_VERSION))
  $(info INFO: Expected mpfsBootmodeProgrammer.jar version $(EXPECTED_MPFS_BOOTMODE_PROGRAMMER_VERSION) but found $(MPFS_BOOTMODE_PROGRAMMER_VERSION))
  $(info INFO: This version of the HSS relies on SoftConsole v2021.3 or later)
endif

$(TARGET-snvm-wrapper): LIBS:=
$(TARGET-snvm-wrapper): $(OBJS-snvm-wrapper)
	$(call main-build-target,snvm-wrapper)
	@$(ECHO) " BIN       `basename $@ .elf`.bin"
	@$(ECHO) " $(OBJCOPY) -O binary $(BINDIR)/$@ $(BINDIR)/`basename $@ .elf`.bin"
	$(OBJCOPY) -O binary $(BINDIR)/$@ $(BINDIR)/`basename $@ .elf`.bin
	@$(ECHO) " HEX       `basename $@ .elf`.hex";
	$(call common-boot-mode-programmer-snvm,--dryrun,`basename $@`)
	@$(ECHO) " @$(CP) $(BINDIR)/bootmode$(BOOTMODE_SNVM)/hss-snvm-wrapper-bm$(BOOTMODE_SNVM)-p0.hex $(BINDIR)/`basename $@ .elf`.${BOARD}.hex"
	@$(CP) $(BINDIR)/bootmode$(BOOTMODE_SNVM)/hss-snvm-wrapper-bm$(BOOTMODE_SNVM)-p0-ap.hex $(BINDIR)/`basename $@ .elf`.${BOARD}.hex
	$(SIZE) $(BINDIR)/$(TARGET-snvm-wrapper) 2>/dev/null

snvm-wrapper_clean:
	-$(RM) $(OBJS-snvm-wrapper) $(BINDIR)/$(TARGET-snvm-wrapper) $(BINDIR)/`basename $(TARGET-snvm-wrapper) .elf`.sym $(BINDIR)/`basename $(TARGET-snvm-wrapper) .elf`.bin

HEX_FILE-snvm-wrapper=$(BINDIR)/$(TARGET-snvm-wrapper:.elf=.hex)
BIN_FILE-snvm-wrapper=$(BINDIR)/$(TARGET-snvm-wrapper:.elf=.bin)

BIN_FILE-snvm-wrapper: $(TARGET-snvm-wrapper)

BOOTMODE_SNVM=2
PACKAGE?=FCVG484
DIE?=MPFS250T

#program: $(BIN_FILE-snvm-wrapper)
#	$(call common-boot-mode-programmer,,$(TARGET-snvm-wrapper))

#verify: $(BIN_FILE-snvm-wrapper)
#	$(call common-boot-mode-programmer,--verify,$(TARGET-snvm-wrapper))
