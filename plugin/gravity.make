#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# Gravity Defied (C++ port).

GRAVITY_SRCDIR = $(APPSDIR)/plugins/gravity
GRAVITY_OBJDIR = $(BUILDDIR)/apps/plugins/gravity

GRAVITY_SRC := $(call preprocess, $(GRAVITY_SRCDIR)/SOURCES)
GRAVITY_OBJ := $(call c2obj, $(GRAVITY_SRC))

OTHER_SRC += $(GRAVITY_SRC)

ROCKS += $(GRAVITY_OBJDIR)/gravity.rock

# C++ flags: start from plugin flags, drop C-only options, add C++ options.
GRAVITY_CXXFLAGS = $(filter-out -std=% -Wstrict-prototypes -Wmissing-prototypes \
	-Wdeclaration-after-statement -Wno-pointer-sign -Wimplicit-function-declaration \
	-Wold-style-definition -Wmissing-field-initializers -Wnested-externs, \
	$(PLUGINFLAGS)) \
	-std=gnu++17 -fno-exceptions -fno-rtti -fno-use-cxa-atexit \
	-fno-threadsafe-statics -fno-builtin -fno-strict-aliasing -Wno-unused-parameter

$(GRAVITY_OBJDIR)/gravity.rock: $(GRAVITY_OBJ)

# compile this plugin's C++ sources
$(GRAVITY_OBJDIR)/%.o: $(GRAVITY_SRCDIR)/%.cpp
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CXX $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(GRAVITY_CXXFLAGS) -c $< -o $@
