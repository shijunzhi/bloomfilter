NAME = libbloomfilter.so

SRC_DIR = ./src
INCLUDE_DIR = ./include

SRC_FILES = $(wildcard $(SRC_DIR)/*.c)

CFLAGS = \
	-fPIC -Wall -Wextra -Werror --std=c11 -I$(INCLUDE_DIR)
LDFLAGS = -lm

all: build

build:
	$(CC) $(CFLAGS) $(SRC_FILES) -o $(NAME) -shared

clean:
	rm -rf $(NAME)
