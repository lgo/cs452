#
# Makefile for the kernel, and library
#

# Disable builtin rules, including %.o: %.c
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

ifndef PROJECT
PROJECT=TC
endif

ifndef TRACK
TRACK=A
endif

ifndef PACKETS
PACKETS=true
endif

GCC_ROOT := /u/wbcowan/gnuarm-4.0.2
GCC_TYPE := arm-elf
GCC_VERSION := 4.0.2

STANDARD_INCLUDES=-include stdbool.h -include stddef.h -include stdint.h
CFLAGS_BACKTRACE := -mpoke-function-name -fverbose-asm -fno-omit-frame-pointer -mapcs-frame -mabi=aapcs -mno-thumb-interwork -marm
CFLAGS_COMPILE_WARNINGS := -Winline -Werror -Wno-unused-variable -Wno-format-security
CFLAGS_OPTIMIZATIONS := -O0 -finline-functions -finline-functions-called-once -fno-optimize-sibling-calls

ifndef LOCAL
# Set of compiler settings for compiling ARM on the student environment
ARCH   = arm
CC     = $(GCC_ROOT)/bin/$(GCC_TYPE)-gcc
AS     = $(GCC_ROOT)/bin/$(GCC_TYPE)-as
AR     = $(GCC_ROOT)/bin/$(GCC_TYPE)-ar
LD     = $(GCC_ROOT)/bin/$(GCC_TYPE)-ld
CFLAGS = -fPIC -Wall -mcpu=arm920t -msoft-float --std=gnu99 -DUSE_$(PROJECT) -DUSE_TRACK$(TRACK) -DUSE_PACKETS=$(PACKETS) $(CFLAGS_OPTIMIZATIONS) $(STANDARD_INCLUDES) $(CFLAGS_BACKTRACE) $(CFLAGS_COMPILE_WARNINGS)
# -Wall: report all warnings
# -fPIC: emit position-independent code
# -mcpu=arm920t: generate code for the 920t architecture
# -msoft-float: use software for floating point
# --std=gnu99: use C99, woo! (default is gnu89)
# -O2: lots of optimizing
ASFLAGS  = -mcpu=arm920t -mapcs-32
# -mcpu=arm920t: use assembly code for the 920t architecture
# -mapcs-32: always create a complete stack frame
LDFLAGS = -init main -Map main.map -N  -T orex.ld -L$(GCC_ROOT)/lib/gcc/$(GCC_TYPE)/$(GCC_VERSION) -L.
# TODO: Document what these mean... heh
else
# Set of compiler settings for compiling on a local machine (likely x86, but nbd)
ARCH   = x86
CC     = gcc
CFLAGS = -Wall -msoft-float --std=gnu99 -Wno-comment -DDEBUG_MODE -g -Wno-varargs -Wno-typedef-redefinition -DUSE_$(PROJECT)  -DUSE_TRACK$(TRACK) -DUSE_PACKETS=$(PACKETS) -finline-functions -Wno-undefined-inline -Wno-int-to-void-pointer-cast $(CFLAGS_COMPILE_WARNINGS) -Wno-int-to-pointer-cast
# -Wall: report all warnings
# -msoft-float: use software for floating point
# --std=gnu99: use C99, same as possible on the school ARM GCC
# -Wno-comment: disable one line comment warnings, they weren't a thing in GNU89
# -DDEBUG_MODE: define DEBUG_MODE for debug purposes
# -g: add symbol information for using GDB
# -Wno-varargs: disable a varargs warning of casting an int to char, no big deal
# -O2: lots of optimizing
endif
ARFLAGS = rcs

# Libraries for linker
# WARNING: Fucking scary as hell. if you put -lgcc before anything, nothing works
# so be careful with the order when you add things
#
# When you add a file it will be in the form of -l<filename>
# NOTE: If you add an ARM specific file, you also need to add -larm<filename>
LIBRARIES= -lcbuffer -ljstring -lmap -larmio -lbwio -larmbwio -lutil -lheap -lalloc -lstdlib -lgcc

# List of includes for headers that will be linked up in the end
INCLUDES = -I./include
USERLAND_INCLUDES = -I./userland


# Various separate components src files, objs, and bins
LIB_SRCS := $(wildcard lib/*.c) $(wildcard lib/$(ARCH)/*.c)
LIB_BINS := $(LIB_SRCS:.c=.a)
LIB_BINS := $(patsubst lib/$(ARCH)/%.a,lib$(ARCH)%.a,$(LIB_BINS))
LIB_OBJS := $(patsubst lib%.a,%.o,$(LIB_BINS))
LIB_BINS := $(patsubst lib/%.a,lib%.a,$(LIB_BINS)) libstdlib.a

STDLIB_SRCS := $(wildcard lib/stdlib/*.c)
STDLIB_OBJS := $(subst /,_,$(STDLIB_SRCS:.c=.o))

KERNEL_SRCS := $(wildcard src/*.c) $(wildcard src/$(ARCH)/*.c)
KERNEL_OBJS := $(patsubst src/$(ARCH)/%.c,$(ARCH)%.o,$(KERNEL_SRCS))
KERNEL_OBJS := $(patsubst src/%.c,%.o,$(KERNEL_OBJS))

TEST_SRCS := $(wildcard test/*.c)
TEST_OBJS := $(patsubst test/%.c,%.o,$(TEST_SRCS))
TEST_BINS := $(TEST_OBJS:.o=.a)

USERLAND_SRCS := $(wildcard userland/**/*.c) $(wildcard userland/*.c)
USERLAND_OBJS := $(subst /,_,$(USERLAND_SRCS:.c=.o))

OBJS := $(USERLAND_OBJS) $(LIB_OBJS) $(KERNEL_OBJS) $(STDLIB_OBJS) $(TEST_OBJS)

ifdef LOCAL
# If were compiling locally, we care about main.a and test binaries
all: main.a $(TEST_BINS)
else
# Compiling on the student environment, we want to compile the ELF and install it
all: main.elf install
endif

# Depend on main.elf and use the script to copy the file over
install: main.elf
	cp $< $(TRACK).elf && ./upload.sh $(TRACK).elf && ./upload.sh $<

# ARM binary
main.elf: $(LIB_BINS) $(KERNEL_OBJS) $(USERLAND_OBJS) orex.ld
	$(LD) $(LDFLAGS) $(KERNEL_OBJS) $(USERLAND_OBJS) -o $@ $(LIBRARIES)

%.elf: standalone_%.o $(LIB_BINS)
	$(LD) $(LDFLAGS) $< -o $@ $(LIBRARIES) && ./upload.sh $<


small_main.elf: $(KERNEL_SRCS) $(LIB_SRCS) $(USERLAND_SRCS)
	$(CC) $(CFLAGS) -nostdlib -nostartfiles -ffreestanding -Wl,-Map,main.map -Wl,-N -T orex.ld -Wl,-L$(GCC_ROOT)/lib/gcc/$(GCC_TYPE)/$(GCC_VERSION) $(INCLUDES) $(USERLAND_INCLUDES) $^ -o $@ -Wl,-lgcc


SRCS_FOR_TESTS=lib/*.c lib/x86/*.c lib/stdlib/*.c
LOCAL_SRCS=$(filter-out src/main.c, $(wildcard src/*.c)) src/x86/*.c lib/*.c lib/x86/*.c lib/stdlib/*.c  userland/*.c userland/**/*.c
LOCAL_LIBS=-lncurses -lpthread

# Local simulation binary
# NOTE: it just explicitly lists a bunch of folders, this is because the
# process of .c => .s => .o drops debugging symbols :(
main.a:
	$(CC) $(CFLAGS) $(INCLUDES) $(USERLAND_INCLUDES) $(LOCAL_SRCS) src/main.c $(LOCAL_LIBS) -o $@

# ASM files from various locations
%.s: src/%.c
	$(CC) $(INCLUDES) $(USERLAND_INCLUDES) $(CFLAGS) -MMD -S -c $< -o $@

standalone_%.s: standalone/%.c
	$(CC) $(INCLUDES) $(CFLAGS) -S -c $< -o $@


%.s: lib/%.c
	$(CC) $(INCLUDES) $(CFLAGS) -MMD -S -c $< -o $@

lib_stdlib_%.s: lib/stdlib/%.c
	$(CC) $(INCLUDES) $(CFLAGS) -MMD -S -c $< -o $@

userland_%.s: userland/%.c
	$(CC) $(INCLUDES) $(USERLAND_INCLUDES) $(CFLAGS) -MMD -S -c $< -o $@
userland_trains_%.s: userland/trains/%.c
	$(CC) $(INCLUDES) $(USERLAND_INCLUDES) $(CFLAGS) -MMD -S -c $< -o $@
userland_track_%.s: userland/track/%.c
	$(CC) $(INCLUDES) $(USERLAND_INCLUDES) $(CFLAGS) -MMD -S -c $< -o $@
userland_interactive_%.s: userland/interactive/%.c
	$(CC) $(INCLUDES) $(USERLAND_INCLUDES) $(CFLAGS) -MMD -S -c $< -o $@
userland_servers_%.s: userland/servers/%.c
	$(CC) $(INCLUDES) $(USERLAND_INCLUDES) $(CFLAGS) -MMD -S -c $< -o $@
userland_entry_%.s: userland/entry/%.c
	$(CC) $(INCLUDES) $(USERLAND_INCLUDES) $(CFLAGS) -MMD -S -c $< -o $@
userland_detective_%.s: userland/detective/%.c
	$(CC) $(INCLUDES) $(USERLAND_INCLUDES) $(CFLAGS) -MMD -S -c $< -o $@

# architecture specific ASM gets prefixed
# this is done to have both a src/bwio.c and src/arm/bwio.c
$(ARCH)%.s: src/$(ARCH)/%.c
	$(CC) $(INCLUDES) $(USERLAND_INCLUDES) $(CFLAGS) -MMD -S -c $< -o $@

$(ARCH)%.s: lib/$(ARCH)/%.c
	$(CC) $(INCLUDES) $(CFLAGS) -MMD -S -c $< -o $@

# all object files from ASM files
%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

# create binaries for each test file, depending on all lib code
%.a: test/%.c $(LIB_SRCS)
	$(CC) $(INCLUDES) $(USERLAND_INCLUDES) $(CFLAGS) $< $(SRCS_FOR_TESTS) $(LOCAL_LIBS) -Wno-error=unused-function -lcheck -o $@

# create library binaries from object files, for ARM bundling
lib%.a: %.o
	$(AR) $(ARFLAGS) $@ $<

# create library binaries from object files, for ARM bundling
libstdlib.a: $(STDLIB_OBJS)
	$(AR) $(ARFLAGS) $@ $<

# clean all files in the top-level, the only place we have temp files
clean:
	rm -rf *.o *.s *.elf *.a *.a.dSYM/ *.map *.d


DEP = $(OBJS:%.o=%.d)

-include $(DEP)


# always run clean (it doesn't produce files)
# also always run main.a because it implicitly depends on all C files
.PHONY: clean main.a

ifndef LOCAL
# if we're compiling ARM, keep the ASM and map files, they're useful
.PRECIOUS: %.s arm%.s %.map lib_stdlib_%.s userland_%.s userland_trains_%.s userland_track_%.s userland_interactive_%.s userland_servers_%.s userland_entry_%.s standalone_%.s userland_detective_%.s
endif
