
# How to use this makefile:
#
#  > make prepare  # copy all source files to a working directory (src)
#  > make          # build the default target (demo.bin)
#  > make debug    # run the GDB debugger
#  > make debugpy  # run the GDB debugger with python
#  > make clean    # remove all temporary files in the working directories (src, obj, dep)

# default target and name of the image and executable files to generate
TARGET     = demo

# path to the root folder of the STM32Cube platform
STM_DIR    = ../../STM32CubeF4/

# Board and MCU names as used in the linker script path and file name, e.g. "$(STM_DIR)/Demonstrations/SW4STM32/STM32F4-DISCO/STM32F407VGTx_FLASH.ld"
BOARD_UC   = STM32F4-DISCO
MCU_UC     = STM32F407VG

# board name as used in the STM32cube Drivers/BSP folder, e.g. "$(STM_DIR)/Drivers/BSP/STM32F4-Discovery"
BSP_BOARD  = STM32F4-Discovery

# MCU name as used in the .s source file name, e.g. "startup_stm32f407xx.s"
MCU_LC     = stm32f407xx

# pre-processor symbol to be defined for the compilation (will be used in a -Dxxx flag in gcc)
MCU_MC     = STM32F407xx


###############################################################################
# Directories

BSP_DIR    = $(STM_DIR)/Drivers/BSP
HAL_DIR    = $(STM_DIR)/Drivers/STM32F4xx_HAL_Driver
CMSIS_DIR  = $(STM_DIR)/Drivers/CMSIS
DEV_DIR    = $(CMSIS_DIR)/Device/ST/STM32F4xx
FATFS_DIR  = $(STM_DIR)/Middlewares/Third_Party/FatFS
USBH_DIR   = $(STM_DIR)/Middlewares/ST/STM32_USB_Host_Library

###############################################################################
# Source files

SRCS += $(wildcard Dev/*.c) 
SRCS += $(wildcard $(FATFS_DIR)/src/*.c)
SRCS += $(filter-out %/usbh_conf_template.c,$(wildcard $(USBH_DIR)/Core/Src/*.c))
SRCS += $(wildcard $(USBH_DIR)/Class/MSC/Src/*.c)
SRCS += $(BSP_DIR)/$(BSP_BOARD)/stm32f4_discovery.c
SRCS += $(BSP_DIR)/Components/lis3dsh/lis3dsh.c
SRCS += $(BSP_DIR)/Components/lis302dl/lis302dl.c
SRCS += $(BSP_DIR)/$(BSP_BOARD)/stm32f4_discovery_accelerometer.c
SRCS += $(HAL_DIR)/Src/stm32f4xx_hal.c
SRCS += $(HAL_DIR)/Src/stm32f4xx_hal_rcc.c
SRCS += $(HAL_DIR)/Src/stm32f4xx_hal_gpio.c
SRCS += $(HAL_DIR)/Src/stm32f4xx_hal_hcd.c
SRCS += $(HAL_DIR)/Src/stm32f4xx_hal_cortex.c
SRCS += $(HAL_DIR)/Src/stm32f4xx_hal_flash.c
SRCS += $(HAL_DIR)/Src/stm32f4xx_hal_sram.c
SRCS += $(HAL_DIR)/Src/stm32f4xx_hal_spi.c
SRCS += $(HAL_DIR)/Src/stm32f4xx_hal_tim.c
SRCS += $(HAL_DIR)/Src/stm32f4xx_ll_usb.c

# remove paths from the file names
SRCS_FN = $(notdir $(SRCS))

LDFILE     = $(MCU_UC)Tx_FLASH.ld
#LDFILE     = $(MCU_UC)Tx_RAM.ld


###############################################################################
# Tools

PREFIX     = arm-none-eabi
CC         = $(PREFIX)-gcc
AR         = $(PREFIX)-ar
OBJCOPY    = $(PREFIX)-objcopy
OBJDUMP    = $(PREFIX)-objdump
SIZE       = $(PREFIX)-size
GDB        = $(PREFIX)-gdb

###############################################################################
# Options

# Defines (-D flags)
DEFS       = -D$(MCU_MC) -DUSE_HAL_DRIVER
DEFS       += -DUSE_DBPRINTF


# Include search paths (-I flags)
INCS       = -IInc
INCS      += -I$(CMSIS_DIR)/Include
INCS      += -I$(DEV_DIR)/Include
INCS      += -I$(HAL_DIR)/Inc
INCS      += -I$(BSP_DIR)/$(BSP_BOARD)/
INCS      += -I$(BSP_DIR)/Components/lis3dsh
INCS      += -I$(BSP_DIR)/Components/lis302dl
INCS      += -I$(FATFS_DIR)/src/
INCS      += -I$(USBH_DIR)/Core/Inc
INCS      += -I$(USBH_DIR)/Class/MSC/Inc

# Library search paths (-L flags)
LIBS       = -L$(CMSIS_DIR)/Lib

# Compiler flags
CFLAGS     = -Wall -std=c99 -O0
CFLAGS    += -g
CFLAGS    += -mlittle-endian -mcpu=cortex-m4 -march=armv7e-m -mthumb
CFLAGS    += -mfpu=fpv4-sp-d16 -mfloat-abi=softfp
CFLAGS    += -ffunction-sections -fdata-sections
CFLAGS    += $(INCS) $(DEFS)

# Linker flags
LDFLAGS    = -Wl,--gc-sections -Wl,-Map=$(TARGET).map $(LIBS) -Tsrc/linkerScript.ld

# Enable Semihosting
LDFLAGS   += --specs=rdimon.specs -lc -lrdimon
#LDFLAGS   += --specs=nosys.specs --specs=nano.specs --specs=rdimon.specs -lc -lrdimon

# Source search paths
VPATH      = ./src
VPATH     += $(BSP_DIR)
VPATH     += $(HAL_DIR)/Src
VPATH     += $(DEV_DIR)/Source/

# Debugger flags
GDBFLAGS   =

# generate OBJS and DEPS target lists by prepending obj/ and dep prefixes
OBJS       = $(addprefix obj/,$(SRCS_FN:.c=.o))
DEPS       = $(addprefix dep/,$(SRCS_FN:.c=.d))


###################################################

.PHONY: all dirs debug debugpy prepare clean 

comp: clean prepare $(TARGET).elf $(TARGET).hex debugpy

all: $(TARGET).bin

-include $(DEPS)

dirs: dep obj

dep obj src:
	@echo "[MKDIR]   $@"
	mkdir -p $@

obj/%.o : %.c | dirs
	@echo "generating \"$@\" from \"$<\""
	$(CC) $(CFLAGS) -c -o $@ $< -MMD -MF dep/$(*F).d

$(TARGET).elf: $(OBJS)
	@echo "[LD]      $(TARGET).elf"
	$(CC) $(CFLAGS) $(LDFLAGS) src/startup_$(MCU_LC).s $^ -o $@
	@echo "[OBJDUMP] $(TARGET).lst"
	$(OBJDUMP) -St $(TARGET).elf >$(TARGET).lst
	@echo "[SIZE]    $(TARGET).elf"
	$(SIZE) $(TARGET).elf

$(TARGET).bin: $(TARGET).elf
	@echo "[OBJCOPY] $(TARGET).bin"
	$(OBJCOPY) -O binary $< $@

$(TARGET).hex: $(TARGET).elf
	@echo "[OBJCOPY] $(TARGET).hex"
	$(OBJCOPY) -O ihex $< $@
	
debug:
	@if ! nc -z localhost 3333; then \
		@echo "\n\t[Error] OpenOCD is not running!\n"; exit 1; \
	else \
	$(GDB)  -ex "target extended localhost:3333" \
			-ex "monitor arm semihosting enable" \
			-ex "monitor reset halt" \
			-ex "load" \
			-ex "monitor reset init" \
			$(GDBFLAGS) $(TARGET).elf; \
	fi

debugpy:
	@if ! nc localhost 3333 ; then \
		echo "Server OpenOCD non trovato"; \
	else \
		$(GDB)-py -ex "target remote localhost:3333" \
				  -ex "monitor reset" \
				  -ex "file $(TARGET).elf" \
				  -ex "load $(TARGET).elf" \
				  -ex "b main" \
				  -ex "source debugconf/gdb-svd.py" \
				  -ex "svd debugconf/STM32F407.svd" ;\
	fi
	
prepare: src
	cp $(SRCS) src/
	cp $(DEV_DIR)/Source/Templates/gcc/startup_$(MCU_LC).s src/
	cp $(LDFILE) src/linkerScript.ld
	cp dev/* src/

clean:
	@echo "[RM]      $(TARGET).bin"; rm -f $(TARGET).bin
	@echo "[RM]      $(TARGET).elf"; rm -f $(TARGET).elf
	@echo "[RM]      $(TARGET).map"; rm -f $(TARGET).map
	@echo "[RM]      $(TARGET).lst"; rm -f $(TARGET).lst
	@echo "[RM]      src files"; rm -f src/*
	@echo "[RM]      ld script"; rm -f src/linkerScript.ld
	@echo "[RMDIR]   dep"          ; rm -fr dep
	@echo "[RMDIR]   obj"          ; rm -fr obj
	@echo "[RMDIR]   obj"          ; rm -fr src
