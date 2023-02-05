PROJECT = tattoo_power_supply
BUILD_DIR = bin

SHARED_DIR = ../lib
CFILES = main.c
CFILES += stusb4500.c

# TODO - you will need to edit these two lines!
DEVICE=stm32f103c8t6
OOCD_INTERFACE	?= stlink
OOCD_TARGET	?= stm32f1x
#STLINK_PORT	?= :4242

# You shouldn't have to edit anything below here.
VPATH += $(SHARED_DIR)
INCLUDES += $(patsubst %,-I%, . $(SHARED_DIR))
OPENCM3_DIR=../libopencm3

include $(OPENCM3_DIR)/mk/genlink-config.mk
include ../rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk