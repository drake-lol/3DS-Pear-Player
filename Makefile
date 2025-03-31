# Basic 3DS Homebrew Makefile

include $(DEVKITPRO)/libctru/common.mk

TARGET := hello_world
BUILD  := build
SOURCE := source
INCLUDES := include

CXXFILES := $(wildcard $(SOURCE)/*.cpp)
CFILES   := $(wildcard $(SOURCE)/*.c)

OBJS := $(CXXFILES:.cpp=.o) $(CFILES:.c=.o)

# Compiling options
CFLAGS   := -g -Wall -O2
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
LDFLAGS  := -specs=3dsx.specs

# Output
$(TARGET).3dsx: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(TARGET).3dsx $(OBJS)