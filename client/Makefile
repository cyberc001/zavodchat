# -sENVIRONMENT=worklet is important so it doesn't detect environment as shell and throw an error
# -sSINGLE_FILE is important so wasm doesn't fail to fetch (cause svelte is being used, and URLs for fetching are complicated)
INCLUDE := -I/3rdparty/rnnoise/include
LIBS := /3rdparty/rnnoise/.libs/librnnoise.so
FLAGS := -sEXPORT_ES6 -sMODULARIZE -sENVIRONMENT=worklet -sSINGLE_FILE -sALLOW_MEMORY_GROWTH -sNO_EXIT_RUNTIME -s "EXPORTED_RUNTIME_METHODS=['ccall', 'HEAPF32']" -s "EXPORTED_FUNCTIONS=['_malloc', '_free', '_main', '_process', '_get_buf']" -sSTACK_SIZE=131072 -Os
CC := emcc $(FLAGS) $(INCLUDE) $(LIBS)


all: static/js/vc_mic_denoiser.js

clean:
	-rm static/js/vc_mic_denoiser.js

static/js/vc_mic_denoiser.js: static/js/vc_mic_denoiser.c
	$(CC) $< -o $@