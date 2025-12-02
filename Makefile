SRCS = $(wildcard ecelinux/*.cpp)
SRCS += $(wildcard ecelinux/*.h)
SRCS += $(wildcard ecelinux/*.hpp)
SRCS += $(wildcard ecelinux/*.sh)
SRCS += $(wildcard ecelinux/*.tcl)
SRCS += $(wildcard ecelinux/Makefile)
SRCS += $(wildcard ecelinux/data)
SRCS += $(wildcard zedboard/*.cpp)
SRCS += $(wildcard zedboard/*.h)
SRCS += $(wildcard zedboard/*.hpp)
SRCS += $(wildcard zedboard/*.sh)
SRCS += $(wildcard zedboard/*.tcl)
SRCS += $(wildcard zedboard/Makefile)
SRCS += $(wildcard zedboard/data)

.PHONY: all

all: hft.zip

hft.zip: $(SRCS)
	zip -r $@ $^
                      