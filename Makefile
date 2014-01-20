CFLAGS = -I./include
OBJECTS = _build_/ga.o _build_/ga_stream.o _build_/gau.o _build_/common/gc_common.o _build_/common/gc_thread.o _build_/devices/ga_openal.o

all: _build_/libgorilla.so

_build_/libgorilla.so: init $(OBJECTS)
	gcc -shared $(CFLAGS) -o $@ $(OBJECTS) -lpthread `pkg-config --libs openal vorbisfile`

_build_/%.o: src/%.c
	gcc -c -fPIC $(CFLAGS) -o $@ $<

_build_/ga.o: src/ga.c
	gcc -c -fPIC $(CFLAGS) -DENABLE_OPENAL -o $@ $<

_build_/devices/ga_openal.o: src/devices/ga_openal.c
	gcc -c -fPIC $(CFLAGS) `pkg-config --cflags openal` -o $@ $<

init:
	@mkdir -p _build_/common/
	@mkdir -p _build_/devices/

clean:
	rm -rf _build_/
