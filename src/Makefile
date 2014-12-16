UTHASH_DIR = ../contrib/uthash

CC=gcc 
CFLAGS=-Wall -Werror -ggdb -fPIC -I. -I../contrib/uthash -D_XOPEN_SOURCE=500 -ldl 


LIB_SRC = transform.c queue.c cstrider_api.c
LIB_OBJ = ${LIB_SRC:.c=.o}
LIB_NAME = libcstrider.a


all:  $(LIB_NAME)  

$(LIB_OBJ): %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(LIB_NAME): $(LIB_OBJ)
	ar rcs $@ $(LIB_OBJ) 

clean:
	rm -f *.o *.a  

