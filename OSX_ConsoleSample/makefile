# User can set VERBOSE variable to have all commands echoed to console for debugging purposes.
ifdef VERBOSE
    Q :=
else
    Q := @
endif

# Useful macros
OBJS = $(patsubst $(ROOT)/%,$2/%,$(addsuffix .o,$(basename $(wildcard $1/*.c $1/*.m))))
MAKEDIR = mkdir -p $(dir $@)
REMOVE = rm
REMOVE_DIR = rm -r -f
QUIET = > /dev/null 2>&1 ; exit 0

# Tool flags
CLANG_FLAGS := -g -Wall -Werror -MMD -MP

# .o object files are to be placed in obj/ directory.
# .a lib files are to be placed in lib/ directory.
OBJDIR := obj
LIBDIR := lib

# Root of Depot.
ROOT := ..

# Setup variables to use for building lib/libmipcapi_osxble.a
LIBMIPCAPI_OSXBLE := lib/libmipcapi_osxble.a
LIBMIPCAPI_OSXBLE_OBJ := $(call OBJS,$(ROOT)/MiP-Capi/mipcapi,$(OBJDIR))
LIBMIPCAPI_OSXBLE_OBJ += $(call OBJS,$(ROOT)/MiP-Capi/osxble,$(OBJDIR))
DEPS := $(patsubst %.o,%.d,$(LIBMIPCAPI_OSXBLE_OBJ))

# Build this sample.
CONSOLE_APP := WallFollow
CONSOLE_OBJ := $(call OBJS,$(ROOT)/OSX_ConsoleSample,$(OBJDIR))
DEPS += $(patsubst %.o,%.d,$(CONSOLE_OBJ))
FRAMEWORKS := -framework Foundation -framework AppKit -framework CoreBluetooth


# Rules
.PHONY : clean all

all : $(CONSOLE_APP)

$(LIBMIPCAPI_OSXBLE) : $(LIBMIPCAPI_OSXBLE_OBJ)
	@echo Building $@
	$Q $(MAKEDIR) $(QUIET)
	$Q ar -rc $@ $?

$(CONSOLE_APP) : $(CONSOLE_OBJ) $(LIBMIPCAPI_OSXBLE)
	@echo Building $@
	$Q $(MAKEDIR) $(QUIET)
	$Q clang $(FRAMEWORKS) $^ -o $@

clean :
	@echo Cleaning libmipcapi
	$Q $(REMOVE_DIR) $(OBJDIR) $(QUIET)
	$Q $(REMOVE_DIR) $(LIBDIR) $(QUIET)

# *** Pattern Rules ***
$(OBJDIR)/%.o : $(ROOT)/%.c
	@echo Compiling $<
	$Q $(MAKEDIR) $(QUIET)
	$Q clang $(CLANG_FLAGS) -I ../MiP-Capi/include -c $< -o $@

$(OBJDIR)/%.o : $(ROOT)/%.m
	@echo Compiling $<
	$Q $(MAKEDIR) $(QUIET)
	$Q clang $(CLANG_FLAGS) -I ../MiP-Capi/include -c $< -o $@

# *** Pull in header dependencies if not performing a clean build. ***
ifneq "$(findstring clean,$(MAKECMDGOALS))" "clean"
    -include $(DEPS)
endif
