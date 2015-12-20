#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/base_rules
CTRULIB := $(DEVKITPRO)/libctru

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
#---------------------------------------------------------------------------------
TARGET		:=	citrus
BUILD		:=	build
SOURCES		:=	source source/citrus source/citrus/ui source/libkhax
DATA		:=	data
INCLUDES	:=	include

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS	:=	-g -Wall -Wno-strict-aliasing -O3 -mword-relocations \
			-fomit-frame-pointer -ffast-math \
			$(ARCH)

CFLAGS	+=	$(INCLUDE) -DARM11 -D_3DS -DLIBKHAX_AS_LIB

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11

CFLAGS +=       -std=gnu99

ASFLAGS	:=	-g $(ARCH)

LIBS	:= -lctru -lm

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(CTRULIB)

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/lib/lib$(TARGET).a

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
PICAFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.pica)))
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

export OFILES	:=	$(addsuffix .o,$(BINFILES)) $(PICAFILES:.pica=.shbin.o) \
			$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

.PHONY: $(BUILD) clean all

#---------------------------------------------------------------------------------
all: $(BUILD)

dist-bin: all
	@tar -cjf $(TARGET).tar.bz2 include lib tools

dist-src:
	@tar -cjf $(TARGET)-src.tar.bz2 include source tools Makefile

dist: dist-src dist-bin

install: dist-bin
	mkdir -p $(DEVKITPRO)/$(TARGET)
	bzip2 -cd $(TARGET).tar.bz2 | tar -x -C $(DEVKITPRO)/$(TARGET)

dox:
	@doxygen Doxyfile
	@doxygen Doxyfile.internal

lib:
	@[ -d $@ ] || mkdir -p $@

$(BUILD): lib
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) lib docs internal_docs $(TARGET).tar.bz2

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

BUILD: submodules $(OUTPUT)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT)	:	$(OFILES)

submodules:
	@git submodule init
	@git submodule update --recursive

#---------------------------------------------------------------------------------
%.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

%.shbin.o : %.pica
	@echo $(notdir $<)
	$(eval CURBIN := $(patsubst %.pica,%.shbin,$(notdir $<)))
	$(eval CURH := $(patsubst %.pica,%.psh.h,$(notdir $<)))
	@picasso -h $(CURH) -o $(CURBIN) $<
	@bin2s $(CURBIN) | $(AS) -o $@
	@echo "extern const u8" `(echo $(CURBIN) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"_end[];" > `(echo $(CURBIN) | tr . _)`.h
	@echo "extern const u8" `(echo $(CURBIN) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"[];" >> `(echo $(CURBIN) | tr . _)`.h
	@echo "extern const u32" `(echo $(CURBIN) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`_size";" >> `(echo $(CURBIN) | tr . _)`.h


-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
