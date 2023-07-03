PROJECT = tattoo_power_supply
BUILD_DIR = bin

SHARED_DIR = ../lib

INC_HEADERS = ../inc
FREERTOS_SRC = ../FreeRTOS
FREERTOS_HEADERS = ../FreeRTOS/include
U8G2_HEADERS = ../u8g2/csrc

INCLUDES += -I$(INC_HEADERS)
INCLUDES += -I$(FREERTOS_HEADERS)
INCLUDES += -I$(FREERTOS_SRC)/portable/GCC/ARM_CM3
INLCUDES += -I$(FREERTOS_SRC)/portable/MemMang
INCLUDES += -I$(U8G2_HEADERS)
INCLUDES += -I$(SHARED_DIR)/ee_libopencm3

VPATH += $(INC_HEADERS)

TGT_CFLAGS	+= -I$(FREERTOS_HEADERS)
TGT_CXXFLAGS	+= -I$(FREERTOS_HEADERS)

CFILES = main.c
CFILES += peripherals.c
CFILES += display.c
CFILES += stusb4500.c
CFILES += DACx3202.c
CFILES += power.c

CFILES +=$(FREERTOS_SRC)/portable/GCC/ARM_CM3/port.c  $(FREERTOS_SRC)/tasks.c  $(FREERTOS_SRC)/list.c $(FREERTOS_SRC)/queue.c $(FREERTOS_SRC)/portable/MemMang/heap_2.c 
CFILES += $(wildcard ../u8g2/csrc/*.c)

CFILES += $(SHARED_DIR)/ee_libopencm3/ee.c

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
