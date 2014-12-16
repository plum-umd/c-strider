include ../shared.mk

EKCC_OPTS = --doglobalreg
TEST=regroot
SRC=main.c 
CSTR_SRC=perfaction.c
OBJ=$(patsubst %.c,%.o,$(SRC))

.PHONY: run-test
all: run-test

%.ktt: %.c
	$(EKCC) $(EKCC_OPTS) $(CFLAGS) $(EKINC) -c $< --doktsavetypes --typesfile-out=$@

.c.o:
	$(EKCC) $(EKCC_OPTS) $(CFLAGS) $(EKINC) -c $^


dsu.c:
	$(EKGEN) $@ main.ktt main.ktt


$(TEST): $(OBJ) dsu.c
	$(CC) -o $@ $^ $(CSTR_SRC)  -ggdb  -I../../src -I../../contrib/uthash/ $(EKLIBTH) -ldl -lpthread 


run-test: $(TEST)
	./$(TEST) 

clean:
	rm -f *.o *.so *.ktt dsu.c dsu.h $(TEST)
