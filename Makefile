TARGETNAME = sh4zamsprites
BUILDDIR=build
OBJS := $(shell find . -name '*.c' -not -name "part_*.c" -not -path "./.git/*" |sed -e 's,\.\(.*\).c,$(BUILDDIR)\1.o,g')

KOS_CSTD := -std=gnu23
CC=kos-cc

DTTEXTURES:=$(shell find assets/textures -name '*.png'| sed -e 's,assets/textures/\(.*\)/\([a-z_A-Z0-9]*\).png,$(BUILDDIR)/pvrtex/\1/\2.dt,g')

.PRECIOUS: $(DTTEXTURES)

LDLIBS 	:= -lm -lm -lkosutils -lsh4zam


DEFINES=
ifdef RELEASEBUILD
	DEFINES += -s -DRELEASEBUILD
else
	DEFINES += -g
endif

ifdef OPTLEVEL
	DEFINES += -O${OPTLEVEL}
else
	DEFINES += -O3
endif

ifdef DCTRACE
DEFINES += -finstrument-functions -DDCTRACE
OBJS += ./profilers/dcprofiler.o
endif 

ifdef SHOWFRAMETIMES
	DEFINES += -DSHOWFRAMETIMES=${SHOWFRAMETIMES}
endif

ifdef BASEPATH
	DEFINES += -DBASEPATH="${BASEPATH}/${TARGETNAME}/"
endif
	
ifdef DEBUG
	DEFINES += -DDEBUG
endif

ifdef DCPROF
	OBJS += ./profilers/dcprof/profiler.o
	DEFINES += -DDCPROF
endif



INCLUDES= -I$(KOS_BASE)/include \
		-I$(KOS_BASE)/kernel/arch/dreamcast/include \
		-I$(KOS_BASE)/addons/include \
		-I$(KOS_BASE)/../kos-ports/include \
		-I$(KOS_BASE)/utils \
		-I$(shell pwd)/include \


CFLAGS+=\
		$(KOS_CSTD) \
		$(INCLUDES) \
		-D__DREAMCAST__  \
		-D_arch_dreamcast -D_arch_sub_pristine \
		-fstack-protector-all \
		-flto=auto \
		-DFRAME_POINTERS \
		-Wall -Wextra  \
		-fno-strict-aliasing \
		-fomit-frame-pointer \
		-fbuiltin -ffast-math -ffp-contract=fast \
		-ml \
		-matomic-model=soft-imask \
		-ffunction-sections -fdata-sections -ftls-model=local-exec \
		-m4-single-only \
		-fms-extensions \
		$(LDLIBS) \
		${DEFINES} \


all: ${TARGETNAME}.elf

${TARGETNAME}.elf: $(OBJS)
	$(CC) $(CFLAGS)  $(OBJS) -o $@ 

include $(KOS_BASE)/Makefile.rules

$(BUILDDIR)/%.o: %.c Makefile $(DTTEXTURES)
	@mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@


TEXDIR_PAL4=$(BUILDDIR)/pvrtex/pal4
$(TEXDIR_PAL4):
	mkdir -p $@
$(TEXDIR_PAL4)/%.dt: assets/textures/pal4/%.png $(TEXDIR_PAL4)
	pvrtex -f PAL4BPP -c --max-color 16 -i $< -o $@

TEXDIR_PAL8=$(BUILDDIR)/pvrtex/pal8
$(TEXDIR_PAL8):
	mkdir -p $@
$(TEXDIR_PAL8)/%.dt: assets/textures/pal8/%.png $(TEXDIR_PAL8)
	pvrtex -f PAL8BPP -c --max-color 256 -i $< -o $@

TEXDIR_RGB565_VQ_TW=$(BUILDDIR)/pvrtex/rgb565_vq_tw
$(TEXDIR_RGB565_VQ_TW):
	mkdir -p $@
$(TEXDIR_RGB565_VQ_TW)/%.dt: assets/textures/rgb565_vq_tw/%.png $(TEXDIR_RGB565_VQ_TW)
	pvrtex -f RGB565 -c -i $< -o $@

TEXDIR_ARGB1555_VQ_TW=$(BUILDDIR)/pvrtex/argb1555_vq_tw
$(TEXDIR_ARGB1555_VQ_TW):
	mkdir -p $@
$(TEXDIR_ARGB1555_VQ_TW)/%.dt: assets/textures/argb1555_vq_tw/%.png $(TEXDIR_ARGB1555_VQ_TW)
	pvrtex -f ARGB1555 -c -i $< -o $@

cdi: ${TARGETNAME}.elf
	mkdcdisc -n ${TARGETNAME} -e $<  -N -o ${TARGETNAME}.cdi -v 3 -m

run: ${TARGETNAME}.elf
	$(KOS_LOADER) ${TARGETNAME}.elf

dist:
	$(KOS_STRIP) ${TARGETNAME}.elf

clean:
	-rm -rf ${TARGETNAME}.elf ${TARGETNAME}.cdi $(OBJS)
