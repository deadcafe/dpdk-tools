GIT_HASH = $(shell git rev-parse HEAD | tr -d "\n")
BEEF_CPPFLAGS = -I. \
	        -D_GNU_SOURCE -DBEEF_GITHASH=\"$(GIT_HASH)\"
BEEF_CFLAGS = -g -std=gnu11 -Werror -Wall -Wextra -Wunused -frecord-gcc-switches
BEEF_OPT = -O0
CPPFLAGS += $(BEEF_CPPFLAGS)
CFLAGS += $(BEEF_OPT) $(BEEF_CFLAGS)
