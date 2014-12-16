include ../shared.mk

EKCC_OPTS = --doglobalreg 
TEST=profiletest
SRC=main.c 
CSTR_SRC=perfaction.c profile.c
OBJ=$(patsubst %.c,%.o,$(SRC))

.PHONY: run-test
all: run-test

%.ktt: %.c
	$(EKCC) $(EKCC_OPTS) $(CSTR_FLAGS) $(EKINC) -c $< --doktsavetypes --typesfile-out=$@

.c.o:
	$(EKCC) $(EKCC_OPTS) $(CSTR_FLAGS) $(EKINC) -c $^


dsu.c: 
	$(EKGEN) $@ main.ktt main.ktt

$(TEST): $(OBJ) dsu.c
	$(CC) -o $(TEST) $^ $(CSTR_SRC)  -ggdb  -I../../src -I../../contrib/uthash/  $(EKLIBTH) -ldl -lpthread 

run-test: $(TEST)
	./$(TEST)

clean:
	rm -f *.o *.so *.ktt dsu.c dsu.h profiletest
