CC:=clang -target i386-pc-none-gnu
CXX:=clang -target i386-pc-none-gnu
LD:=i686-pc-skift-ld
LDFLAGS:= \
	--sysroot=$(SYSROOT)
AR:=i686-pc-skift-ar
ARFLAGS:=rcs
AS=nasm
ASFLAGS=-f elf32
STRIP:=i686-pc-skift-strip
