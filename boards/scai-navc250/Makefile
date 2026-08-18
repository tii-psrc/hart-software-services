#
# MPFS HSS Embedded Software
#
# Copyright 2019-2022 Microchip FPGA Embedded Systems Solutions.
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
# Defines target-specific build-rules variables, extra sources and include paths
#

$(info scai-navc250 selected)

ifeq ($(BOARD), scai-navc250)
CORE_CFLAGS += -DCONFIG_BOARD_SCAI_NAVC250
endif

TARGET-l2scratch=hss-l2scratch.elf
TARGET-envm-wrapper=hss-envm-wrapper.elf
RISCV_TARGET=$(TARGET-l2scratch) $(TARGET-envm-wrapper)
TARGET:=$(RISCV_TARGET)

LINKER_SCRIPT-l2scratch=boards/${BOARD}/hss-l2scratch.ld

BOARD_DIR=boards/scai-navc250

INCLUDES += \
	-I$(BOARD_DIR)/mpfs_hal_config/ \
	-I$(BINDIR)/$(BOARD_DIR)/fpga_design_config/ \
	-I$(BINDIR)/$(BOARD_DIR)/ \
	-I$(BOARD_DIR)/ \
	-Ibaremetal/polarfire-soc-bare-metal-library/src/platform \

EXTRA_SRCS-y += \
	$(BOARD_DIR)/hss_uart_init.c \
        $(BOARD_DIR)/uart_device_list.c \
        $(BOARD_DIR)/hss_board_init.c \
        $(BOARD_DIR)/hss_uart_ymodem.c \

EXTRA_SRCS-$(CONFIG_USE_LOGO) += \
        init/hss_logo_init.c

EXTRA_SRCS-$(CONFIG_SERVICE_HEALTHMON) += \
        $(BOARD_DIR)/healthmon_monitors.c \


$(BOARD_DIR)/hss_uart_init.o: CFLAGS=$(CFLAGS_GCCEXT)

ifndef CONFIG_SERVICE_BOOT_USE_PAYLOAD_IN_FABRIC
EXTRA_OBJS-$(CONFIG_SERVICE_BOOT_USE_PAYLOAD) += $(BOARD_DIR)/payload.o
$(BOARD_DIR)/payload.o: $(BOARD_DIR)/payload.bin
	$(LD) -r -b binary $< -o $@
else
endif

################################################################################################
#
# Extra hardware dependency rules for QSPI
#

INCLUDES += \
	-Ibaremetal/ \

ifdef CONFIG_SERVICE_QSPI_WINBOND_W25N01GV
INCLUDES += \
	-Ibaremetal/drivers/winbond_w25n01gv \

EXTRA_SRCS-$(CONFIG_SERVICE_QSPI) += \
	baremetal/drivers/winbond_w25n01gv/winbond_w25n01gv.c

$(BINDIR)/baremetal/drivers/winbond_w25n01gv/winbond_w25n01gv.o: CFLAGS=$(CFLAGS_GCCEXT)
endif

ifdef CONFIG_SERVICE_QSPI_MICRON_MQ25T
INCLUDES += \
	-Ibaremetal/drivers/micron_mt25q \

EXTRA_SRCS-$(CONFIG_SERVICE_QSPI) += \
	baremetal/drivers/micron_mt25q/micron_mt25q.c

$(BINDIR)/baremetal/drivers/micron_mt25q/micron_mt25q.o: CFLAGS=$(CFLAGS_GCCEXT)
endif

ifdef CONFIG_SERVICE_SCAI_FPGA
INCLUDES += \
	-Ibaremetal/drivers/scai_fpga \

EXTRA_SRCS-$(CONFIG_SERVICE_SCAI_FPGA) += \
	baremetal/drivers/scai_fpga/scai_fpga.c \
	baremetal/drivers/scai_fpga/scai_fpga_gpio.c \
	baremetal/drivers/scai_fpga/scai_fpga_platform.c \
	baremetal/drivers/scai_fpga/scai_fpga_common.c \
	baremetal/drivers/scai_fpga/winbond_w25n01gv_fpga.c \
	baremetal/drivers/scai_fpga/micron_mt29f_fpga.c \
	baremetal/drivers/scai_fpga/winbond_w25n01gv_direct.c

$(BINDIR)/baremetal/drivers/scai_fpga/scai_fpga.o: CFLAGS=$(CFLAGS_GCCEXT)
$(BINDIR)/baremetal/drivers/scai_fpga/scai_fpga_gpio.o: CFLAGS=$(CFLAGS_GCCEXT)
$(BINDIR)/baremetal/drivers/scai_fpga/scai_fpga_platform.o: CFLAGS=$(CFLAGS_GCCEXT)
$(BINDIR)/baremetal/drivers/scai_fpga/scai_fpga_common.o: CFLAGS=$(CFLAGS_GCCEXT)
$(BINDIR)/baremetal/drivers/scai_fpga/winbond_w25n01gv_fpga.o: CFLAGS=$(CFLAGS_GCCEXT)
$(BINDIR)/baremetal/drivers/scai_fpga/micron_mt29f_fpga.o: CFLAGS=$(CFLAGS_GCCEXT)
$(BINDIR)/baremetal/drivers/scai_fpga/winbond_w25n01gv_direct.o: CFLAGS=$(CFLAGS_GCCEXT)
endif

#ifdef CONFIG_SERVICE_FPGA_QSPI
INCLUDES += \
	-Ibaremetal/drivers/scai_fpgaqspi \

EXTRA_SRCS-$(CONFIG_SERVICE_FPGA_QSPI) += \
	baremetal/drivers/scai_fpgaqspi/bbt.c \
	baremetal/drivers/scai_fpgaqspi/nandcore.c \
	baremetal/drivers/scai_fpgaqspi/mtdcore.c \
	baremetal/drivers/scai_fpgaqspi/scai_fpgaqspi.c \
	baremetal/drivers/scai_fpgaqspi/spicore.c	\
	baremetal/drivers/scai_fpgaqspi/winbond.c \
	baremetal/drivers/scai_fpgaqspi/scai_fpgaqspi_service.c

$(BINDIR)/baremetal/drivers/scai_fpgaqspi/bbt.o: CFLAGS=$(CFLAGS_GCCEXT)
$(BINDIR)/baremetal/drivers/scai_fpgaqspi/nandcore.o: CFLAGS=$(CFLAGS_GCCEXT)
$(BINDIR)/baremetal/drivers/scai_fpgaqspi/mtdcore.o: CFLAGS=$(CFLAGS_GCCEXT)
$(BINDIR)/baremetal/drivers/scai_fpgaqspi/spicore.o: CFLAGS=$(CFLAGS_GCCEXT)
$(BINDIR)/baremetal/drivers/scai_fpgaqspi/scai_fpgaqspi.o: CFLAGS=$(CFLAGS_GCCEXT)
$(BINDIR)/baremetal/drivers/scai_fpgaqspi/winbond.o: CFLAGS=$(CFLAGS_GCCEXT)
$(BINDIR)/baremetal/drivers/scai_fpgaqspi/scai_fpgaqspi_service.o: CFLAGS=$(CFLAGS_GCCEXT)
#endif

EXTRA_SRCS-$(CONFIG_SERVICE_TELEMETRY) += \
        $(BOARD_DIR)/adcs.c

EXTRA_SRCS-$(CONFIG_SERVICE_WDOG_ENABLE_EXTERNAL) +=	\
	baremetal/polarfire-soc-bare-metal-library/src/platform/drivers/mss/mss_gpio/mss_gpio.c \

################################################################################################
#
# Linker Scripts
#

$(BOARD_DIR)/hss-l2scratch.ld: $(BOARD_DIR)/hss-l2scratch.lds $(CONFIG_H)

################################################################################################

#
# Extra dependency rules for auto-generated configuration files (from Libero XML)
#

SOC_CONFIG_FILES =  \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/clocks/hw_clk_ddr_pll.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/clocks/hw_clk_mss_cfm.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/clocks/hw_clk_mss_pll.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/clocks/hw_clk_sgmii_cfm.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/clocks/hw_clk_sgmii_pll.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/clocks/hw_clk_sysreg.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/clocks/hw_mss_clks.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/ddr/hw_ddr_io_bank.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/ddr/hw_ddr_mode.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/ddr/hw_ddr_off_mode.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/ddr/hw_ddr_options.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/ddr/hw_ddr_segs.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/ddr/hw_ddrc.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/general/hw_gen_peripherals.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/fpga_design_config.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/io/hw_hsio_mux.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/io/hw_mssio_mux.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/memory_map/hw_apb_split.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/memory_map/hw_cache.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/memory_map/hw_memory.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/memory_map/hw_mpu_crypto.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/memory_map/hw_mpu_fic0.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/memory_map/hw_mpu_fic1.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/memory_map/hw_mpu_fic2.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/memory_map/hw_mpu_gem0.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/memory_map/hw_mpu_gem1.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/memory_map/hw_mpu_mmc.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/memory_map/hw_mpu_scb.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/memory_map/hw_mpu_trace.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/memory_map/hw_mpu_usb.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/memory_map/hw_pmp_hart0.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/memory_map/hw_pmp_hart1.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/memory_map/hw_pmp_hart2.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/memory_map/hw_pmp_hart3.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/memory_map/hw_pmp_hart4.h \
	$(BINDIR)/$(BOARD_DIR)/fpga_design_config/sgmii/hw_sgmii_tip.h \

ifeq ($(origin CONFIG_SOC_FPGA_DESIGN_XML), undefined)
else
SOC_FPGA_DESIGN_XML_FILE= $(subst $\",,$(CONFIG_SOC_FPGA_DESIGN_XML))
ifeq ("$(wildcard $(SOC_FPGA_DESIGN_XML_FILE))", "")
$(error "XML file $(CONFIG_SOC_FPGA_DESIGN_XML) specified by CONFIG_SOC_FPGA_DESIGN_XML does not exist")
endif
endif

config.h: $(SOC_CONFIG_FILES)
$(SOC_CONFIG_FILES): $(SOC_FPGA_DESIGN_XML_FILE)
	@mkdir -p $(BINDIR)/$(BOARD_DIR)
	@$(ECHO) " MPFSCFGGEN    $<";
	$(PYTHON) tools/polarfire-soc-configuration-generator/mpfs_configuration_generator.py $< $(BINDIR)/$(BOARD_DIR)

$(RISCV_TARGET): $(SOC_CONFIG_FILES)
PACKAGE=FCG1152
DIE=MPFS250T
