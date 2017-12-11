# Base directory for the compiler
XTENSA_TOOLS_ROOT	?= /home/akash/esp-open-sdk/xtensa-lx106-elf/bin
SDK_BASE			?= /home/akash/esp-open-sdk/sdk
SDK_TOOLS			?= $(XTENSA_TOOLS_ROOT)
BIN_BASE			= Espressif_BINs
USER_BIN_BASE		= USER_BINs
FW_FILE_ADDR		= 0x01000

ESPPORT				= /dev/ttyUSB0
ESPPORT				= /dev/rfcomm0
ESPBAUD				= 921600
ifeq ("$(ESPPORT)", "/dev/rfcomm0")
ESPBAUD	=	115200
else
ESPBAUD	=	921600
endif

# select which tools to use as compiler, librarian and linker
CC					:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
AR					:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-ar
LD					:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
OBJCOPY				:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-objcopy
OBJDUMP				:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-objdump
ESPTOOL				?= esptool.py

# compiler flags used during compilation of source files
TARGET				= myApp
CFLAGS				= -Os -g -O3 -Wpointer-arith -Wundef -Werror -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals -mno-serialize-volatile -D__ets__ -DICACHE_FLASH
MODULES				= user driver
EXTRA_INCDIR		= include
BUILD_BASE			= build
FW_BASE				= firmware
SDK_LIBDIR			= lib
SDK_LDDIR			= ld
SDK_INCDIR			= include include/json driver_lib/include/driver
FW_FILE				:= $(addprefix $(FW_BASE)/,user1.bin)

#
# Nothing to configure south of here.
#
# linker flags used to generate the main object file
LDFLAGS				= -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static
# libraries used in this project, mainly provided by the SDK
LIBS				= c gcc hal pp phy net80211 lwip wpa wpa2 main driver upgrade
# linker script used for the above linkier step
LD_SCRIPT			= eagle.app.v6.new.2048.ld
flashimageoptions = --flash_freq 80m --flash_mode qio --flash_size 32m

SDK_LIBDIR			:= $(addprefix $(SDK_BASE)/,$(SDK_LIBDIR))
SDK_INCDIR			:= $(addprefix -I$(SDK_BASE)/,$(SDK_INCDIR))
LD_SCRIPT			:= $(addprefix -T$(SDK_BASE)/$(SDK_LDDIR)/, $(LD_SCRIPT))
LIBS				:= $(addprefix -l, $(LIBS))
APP_AR				:= $(addprefix $(BUILD_BASE)/, $(TARGET)_app.a)
TARGET_OUT			:= $(addprefix $(BUILD_BASE)/, $(TARGET).out)
BUILD_DIRS			 = $(addprefix $(BUILD_BASE)/, $(MODULES)) $(FW_BASE)
SRC					 = $(foreach moduleDir, $(MODULES), $(wildcard $(moduleDir)/*.c))
EXTRA_INCDIR		:= $(addprefix -I,$(EXTRA_INCDIR))
INCDIR				:= $(addprefix -I,$(MODULES))

# Replace all x.c with x.o
OBJS				 = $(patsubst %.c, $(BUILD_BASE)/%.o, $(SRC))

all: checkdirs $(TARGET_OUT)
	echo "Image file built!"

# Build the application archive.
# This is dependent on the compiled objects.
$(APP_AR): $(OBJS)
	$(AR) -cru $(APP_AR) $(OBJS)

# Build the objects from the C source files
$(BUILD_BASE)/%.o : %.c
	$(CC) $(INCDIR) $(EXTRA_INCDIR) $(SDK_INCDIR) $(CFLAGS) -c $< -o $@

# Check that the required directories are present
checkdirs: $(BUILD_DIRS)

# Create the directory structure which holds the builds (compiles)
$(BUILD_DIRS):
	mkdir --parents --verbose $@

$(TARGET_OUT):	$(APP_AR)
	$(LD) -L$(SDK_LIBDIR) $(LD_SCRIPT) $(LDFLAGS) -Wl,--start-group $(LIBS) $(APP_AR) -Wl,--end-group -o $@
	$(OBJDUMP) --headers --section=.data \
		--section=.rodata \
		--section=.bss \
		--section=.text \
		--section=.irom0.text $@
	$(OBJCOPY) --only-section .text --output-target binary $@ eagle.app.v6.text.bin
	$(OBJCOPY) --only-section .data --output-target binary $@ eagle.app.v6.data.bin
	$(OBJCOPY) --only-section .rodata --output-target binary $@ eagle.app.v6.rodata.bin
	$(OBJCOPY) --only-section .irom0.text --output-target binary $@ eagle.app.v6.irom0text.bin
	#$(SDK_TOOLS)/gen_appbin.py $@ 2 0 0xf 4
	#mv eagle.app.flash.bin $(FW_BASE)/eagle.flash.bin
	#mv eagle.app.v6.irom0text.bin $(FW_BASE)/eagle.irom0text.bin
	$(ESPTOOL) elf2image --version=2 -o $(FW_BASE)/user1.bin $(TARGET_OUT)
	rm eagle.app.v6.*
#
# Flash the ESP8266
#
flash: all
	$(ESPTOOL) --port $(ESPPORT) --baud $(ESPBAUD) write_flash $(flashimageoptions) 0x00000 $(BIN_BASE)/boot_v1.6.bin $(FW_FILE_ADDR) $(FW_FILE) 0x3FC000 $(BIN_BASE)/esp_init_data_default.bin 0x3FE000 $(BIN_BASE)/blank.bin
	#$(ESPTOOL) --port $(ESPPORT) --baud $(ESPBAUD) write_flash $(flashimageoptions) 0x200000 $(USER_BIN_BASE)/Pacbinscreen.bin

# Clean any previous builds
clean:
# Remove forceably and recursively
	rm --recursive --force --verbose $(BUILD_BASE) $(FW_BASE)
flashId:
	$(ESPTOOL) --port $(ESPPORT) --baud $(ESPBAUD) flash_id
readMac:
	$(ESPTOOL) --port $(ESPPORT) --baud $(ESPBAUD) read_mac
imageInfo: all
	$(ESPTOOL) image_info $(FW_BASE)/user1.bin
eraseFlash:
	$(ESPTOOL) --port $(ESPPORT) --baud $(ESPBAUD) erase_flash
Run:
	$(ESPTOOL) --port $(ESPPORT) --baud $(ESPBAUD) run
Table: all
	xtensa-lx106-elf-readelf -s /home/akash/workspace/'Moving Pixels'/build/myApp.out > /home/akash/workspace/'Moving Pixels'/build/table.csv
	libreoffice --calc /home/akash/workspace/'Moving Pixels'/build/table.csv
