OPTS+= -s BINARYEN_TRAP_MODE=clamp -s ALLOW_MEMORY_GROWTH=1

include $(OAKNUT_DIR)/build/web.make

web_wasm: $(EXECUTABLE)
