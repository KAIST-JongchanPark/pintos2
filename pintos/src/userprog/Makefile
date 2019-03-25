include ../Makefile.kernel
$(PROGS): LDFLAGS += -nostdlib -static -Wl,-T,$(LDSCRIPT) -Wl,--build-id=none
