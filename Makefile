#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
#
# NO_SMDH: if set to anything, no SMDH file is generated.
# APP_TITLE is the name of the app stored in the SMDH file (Optional)
# APP_DESCRIPTION is the description of the app stored in the SMDH file (Optional)
# APP_AUTHOR is the author of the app stored in the SMDH file (Optional)
# ICON is the filename of the icon (.png), relative to the project folder.
#   If not set, it attempts to use one of the following (in this order):
#     - <Project name>.png
#     - icon.png
#     - <libctru folder>/default_icon.png
#---------------------------------------------------------------------------------
TARGET		:=	$(notdir $(CURDIR))
BUILD		:=	build
SOURCES		:=	source source/EMULib source/fMSX source/Z80 source/3DS source/3DS/snddrv
DATA		:=	data
INCLUDES	:=	source source/EMULib source/fMSX source/Z80 source/3DS source/3DS/snddrv
ROMFS		:=	romfs

APP_TITLE       := fMSX emu
APP_DESCRIPTION := MSX Emulator
APP_AUTHOR      := Marat Fayzullin  h.tomioka

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv6k -ei -mtune=mpcore -mfloat-abi=hard

CFLAGS	:=	-g -Wall -O2 -mword-relocations \
			-fomit-frame-pointer -ffunction-sections \
			$(ARCH)

CFLAGS	+=	$(INCLUDE) -DARM11 -D_3DS -DFMSX -DLSB_FIRST -DALTSOUND -DNO_AUDIO_PLAYBACK -DUSE_CBIOS -DDEBUG_ENABLE \
	-D__3DS__

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11 -Wno-narrowing

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS = -L/opt/devkitpro/portlibs/3ds/lib\
	-L/opt/devkitpro/portlibs/3ds\
	-L/opt/devkitpro/libctru/lib\
	-lSDL\
	-lSDL_image\
	-ljpeg\
	-lpng\
	-lSDL_mixer\
	-lz\
	-lminizip\
	-lcitro2d\
	-lcitro3d\
	-lctru\
	-lm

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(CTRULIB) $(PORTLIBS)

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

OFILES	:=	$(addsuffix .o,$(BINFILES)) \
			$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

OFILES	:=	$(filter-out machine.o ,$(OFILES))

export OFILES	:=	$(OFILES)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

ifeq ($(strip $(ICON)),)
	icons := $(wildcard *.png)
	ifneq (,$(findstring $(TARGET).png,$(icons)))
		export APP_ICON := $(TOPDIR)/$(TARGET).png
	else
		ifneq (,$(findstring icon.png,$(icons)))
			export APP_ICON := $(TOPDIR)/icon.png
		endif
	endif
else
	export APP_ICON := $(TOPDIR)/$(ICON)
endif

ifeq ($(strip $(NO_SMDH)),)
	export _3DSXFLAGS += --smdh=$(CURDIR)/$(TARGET).smdh
endif

ifneq ($(ROMFS),)
	export _3DSXFLAGS += --romfs=$(CURDIR)/$(ROMFS)
endif

.PHONY: $(BUILD) clean all

#---------------------------------------------------------------------------------
all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).3dsx $(OUTPUT).smdh $(TARGET).elf


#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
ifeq ($(strip $(NO_SMDH)),)
$(OUTPUT).3dsx	:	$(OUTPUT).elf $(OUTPUT).smdh
else
$(OUTPUT).3dsx	:	$(OUTPUT).elf
endif

$(OUTPUT).elf	:	$(OFILES)

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

# WARNING: This is not the right way to do this! TODO: Do it right!
#---------------------------------------------------------------------------------
%.vsh.o	:	%.vsh
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@python $(AEMSTRO)/aemstro_as.py $< ../$(notdir $<).shbin
	@bin2s ../$(notdir $<).shbin | $(PREFIX)as -o $@
	@echo "extern const u8" `(echo $(notdir $<).shbin | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"_end[];" > `(echo $(notdir $<).shbin | tr . _)`.h
	@echo "extern const u8" `(echo $(notdir $<).shbin | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"[];" >> `(echo $(notdir $<).shbin | tr . _)`.h
	@echo "extern const u32" `(echo $(notdir $<).shbin | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`_size";" >> `(echo $(notdir $<).shbin | tr . _)`.h
	@rm ../$(notdir $<).shbin

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------