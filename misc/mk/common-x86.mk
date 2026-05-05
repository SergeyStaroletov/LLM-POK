CC = /opt/homebrew/Cellar/i686-elf-gcc/15.1.0/bin/i686-elf-gcc
CFLAGS	=	$(CONFIG_CFLAGS) -nostdinc -iwithprefix include -fno-builtin -DPOK_ARCH_X86 $(KIND_CFLAGS) $(GENERIC_FLAGS) -Wall -g -O -Wuninitialized -fno-stack-protector -ffreestanding -nostdlib

# FIXME: architecture should not be hardcoded...
ADAFLAGS = -gnaty -gnata -m32 -I $(POK_PATH)/libpok/ada/arinc653

#REMAINDER
#-fno-stack-protector and -nostdlib is added for Ubuntu

LDFLAGS	=	-m $(ELF_MODE)

