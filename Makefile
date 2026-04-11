#---------------------------------------------------------------------------------
# WiiReflex Makefile (FIXED)
#---------------------------------------------------------------------------------

# Use devkitPro standard env
DEVKITPRO ?= /d/devkitPro
DEVKITPPC := $(DEVKITPRO)/devkitPPC

ifeq ($(strip $(DEVKITPPC)),)
$(error "DEVKITPPC not found. Install devkitPro correctly.")
endif

PPC_BIN    := $(DEVKITPPC)/bin
TOOLS_BIN  := $(DEVKITPRO)/tools/bin

CC      := $(PPC_BIN)/powerpc-eabi-gcc.exe
LD      := $(PPC_BIN)/powerpc-eabi-gcc.exe
RAW2C   := D:\devkitPro\tools\bin\raw2c.exe
ELF2DOL := $(TOOLS_BIN)/elf2dol.exe

TARGET  := WiiReflex
BUILD   := build
SOURCES := source
DATA    := data

PORTLIBS_WII := $(DEVKITPRO)/portlibs/wii
PORTLIBS_PPC := $(DEVKITPRO)/portlibs/ppc
LIBOGC       := $(DEVKITPRO)/libogc

MACHDEP := -DGEKKO -mrvl -mcpu=750 -meabi -mhard-float

#---------------------------------------------------------------------------------
# LIBS (FIXED ORDER - REQUIRED FOR GRRLIB + PNGU + FREETYPE)
#---------------------------------------------------------------------------------
LIBS := -lgrrlib -lfreetype -lpngu -lpng -ljpeg \
        -lfat -lwiiuse -lbte -logc -lm -lz -lbz2 \
		-lbrotlidec -lbrotlicommon

#---------------------------------------------------------------------------------
# BUILD SYSTEM
#---------------------------------------------------------------------------------

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT   := $(CURDIR)/$(TARGET)
export VPATH    := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                   $(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR  := $(CURDIR)/$(BUILD)

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
PNGFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.png)))

export OFILES_BIN     := $(PNGFILES:.png=.o)
export OFILES_SOURCES := $(CFILES:.c=.o)

export OFILES := $(OFILES_BIN) $(OFILES_SOURCES)

export CFLAGS := -g -O2 -Wall $(MACHDEP) \
    -I$(PORTLIBS_WII)/include \
    -I$(PORTLIBS_PPC)/include \
    -I$(LIBOGC)/include \
    -I$(CURDIR)/$(BUILD)

export LDFLAGS := $(MACHDEP) \
    -Wl,-Map,$(notdir $@).map \
    -L$(PORTLIBS_WII)/lib \
    -L$(PORTLIBS_PPC)/lib \
    -L$(LIBOGC)/lib/wii

.PHONY: $(BUILD) clean

#---------------------------------------------------------------------------------
# BUILD TARGET
#---------------------------------------------------------------------------------

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile $(OUTPUT).dol

clean:
	@echo cleaning...
	@rm -rf $(BUILD) $(OUTPUT).elf $(OUTPUT).dol
	@rm -f $(DATA)/*.c $(DATA)/*.h

else

#---------------------------------------------------------------------------------
# COMPILE RULES
#---------------------------------------------------------------------------------

%.o: %.c
	@echo compiling ... $(notdir $<)
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: %.png
	@echo processing ... $(notdir $<)
	@$(RAW2C) $<
	@$(CC) $(CFLAGS) -c $(basename $(notdir $<)).c -o $@
	@rm -f $(basename $(notdir $<)).c

#---------------------------------------------------------------------------------
# LINK
#---------------------------------------------------------------------------------

$(OUTPUT).elf: $(OFILES)
	@echo linking ... $(notdir $@)
	@$(LD) $(LDFLAGS) $(OFILES) $(LIBS) -o $@

$(OUTPUT).dol: $(OUTPUT).elf
	@echo output ... $(notdir $@)
	@$(ELF2DOL) $< $@

endif