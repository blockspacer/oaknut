
ifndef EMSCRIPTEN_ROOT
$(error EMSCRIPTEN_ROOT must point to the root of the Emscripten SDK)
endif

HTML_FILE := $(OAKNUT_DIR)/src/platform/web/web.html
OUTPUT_DIR := $(BUILD_DIR)/webroot
OBJS:=$(OBJS:.o=.bc)

# Remove obj-c sources
OBJS:=$(filter-out %.m.bc,$(OBJS))
OBJS:=$(filter-out %.mm.bc,$(OBJS))

#OPTS+=$(if $(DEBUG),-O0 --profiling -s DEMANGLE_SUPPORT=1,-O3)
OPTS+= -O0 --profiling -s DEMANGLE_SUPPORT=1
OPTS+= -s USE_PTHREADS=1 
#PThreads is broken on WASM cos there's no Atomics support (it exists but is disabled cos of Spectre)
#OPTS+= -s USE_PTHREADS=1

ALLOPTS:= $(CFLAGS) -DPLATFORM_WEB=1 -DEMSCRIPTEN $(OPTS) \
         -I$(OAKNUT_DIR)/src \
         -isystem $(EMSCRIPTEN_ROOT)/system/include \
         -s USE_PTHREADS=1 -s TOTAL_MEMORY=33554432

$(PCH).dep : $(OAKNUT_DIR)/src/oaknut.h
	@mkdir -p $(dir $@)
	@$(EMSCRIPTEN_ROOT)/emcc $(ALLOPTS) -std=c++11 -x c++-header -E -M -MT $(PCH) $< -o $(PCH).dep
	echo "	$(EMSCRIPTEN_ROOT)/emcc $(ALLOPTS) -std=c++11 -x c++-header $(OAKNUT_DIR)/src/oaknut.h -emit-pch -o $(PCH)" >>$@

DEPS := $(PCH).dep $(DEPS)


#$(OBJ_DIR)%.bc : %

$(OBJ_DIR)%.bc : % $(OBJ_DIR)%.dep
	@echo $(PLATFORM): Compiling $(notdir $<)
	@mkdir -p $(dir $@)
	@$(EMSCRIPTEN_ROOT)/emcc  \
        -MT $@ -MD -MP -MF $(@:.bc=.Td) \
        $(if $(filter $(suffix $<),.cpp),-std=c++11,) \
        $(ALLOPTS) \
	-include-pch $(PCH) \
	$< -o $@
	@mv -f $(@:.bc=.Td) $(@:.bc=.dep) && touch $@


EXECUTABLE=$(OUTPUT_DIR)/xx.html

ASSET_DIRS = $(shell find $(PROJECT_ROOT)/assets/ -type d)
ASSET_FILES = $(shell find $(PROJECT_ROOT)/assets -type f -name '*')

$(EXECUTABLE): $(PCH).dep $(PCH) $(OBJS) $(HTML_FILE) $(ASSET_DIRS) $(ASSET_FILES)
	@echo $(PLATFORM): Linking app
	@mkdir -p $(dir $(EXECUTABLE))
	@$(EMSCRIPTEN_ROOT)/emcc --bind $(OPTS) --emrun --preload-file $(PROJECT_ROOT)/assets@/assets --shell-file $(HTML_FILE) $(OBJS) -o $@
