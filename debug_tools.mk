CC := clang
CFLAGS := -Wall -O0 -g -I.

TARGETS := debug_launcher fbneo_debug_display

.PHONY: all clean

all: $(TARGETS)

debug_launcher: debug_launcher.c debug_controller.c
	$(CC) $(CFLAGS) $^ -o $@
	cp $@ ../../../

fbneo_debug_display: ../../../debug_preview.c
	$(CC) $(CFLAGS) $^ -o $@
	cp $@ ../../../

clean:
	rm -f $(TARGETS) ../../../debug_launcher ../../../fbneo_debug_display 