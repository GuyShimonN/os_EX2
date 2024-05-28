DIRS = 1 2 3 3.5 4 6

.PHONY: all clean $(DIRS)

all: $(DIRS)

$(DIRS):
	$(MAKE) -C $@

clean:
	for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done