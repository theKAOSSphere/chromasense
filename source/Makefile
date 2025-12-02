# Source-level build rules for chromasense LV2

CXX ?= g++
AR  ?= ar

NAME = chromasense

# Main plugin sources
SRC  = chromasense.cpp chromasense_pitch_tracker.cpp

# Bundled zita-resampler sources (no external dependency)
ZITA_SRC = zita-resampler/resampler.cc zita-resampler/resampler-table.cc
ZITA_OBJ = zita-resampler/resampler.o zita-resampler/resampler-table.o

OBJ  = $(SRC:.cpp=.o) $(ZITA_OBJ)
LIB  = $(NAME).so

# Include path for bundled zita-resampler headers
CXXFLAGS += -Wall -Wextra -fvisibility=hidden -fPIC -std=c++11 -O3 -I./zita-resampler
LDFLAGS  += -shared
LIBS      = -lm -lfftw3f -lpthread

# Installation settings - default to /usr/local/lib/lv2 to match other plugins
INSTALL_PATH ?= /usr/local/lib/lv2
BUNDLE = chromasense.lv2

all: $(LIB)

$(LIB): $(OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

install: $(LIB)
	mkdir -p $(DESTDIR)$(INSTALL_PATH)/$(BUNDLE)
	cp $(LIB) $(DESTDIR)$(INSTALL_PATH)/$(BUNDLE)/
	cp -r $(BUNDLE)/* $(DESTDIR)$(INSTALL_PATH)/$(BUNDLE)/

clean:
	-rm -f $(OBJ) $(LIB)
	-rm -f zita-resampler/*.o

.PHONY: all clean install
