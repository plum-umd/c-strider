include ../shared.mk

EKCC_OPTS = --doglobalreg
TEST=fig4
SRC=main.c 
CSTR_SRC=perfaction.c serial.c
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
	$(CC) -o $@ $^ $(CSTR_SRC)  -ggdb  -I../../src -I../../contrib/uthash/ $(EKLIBTH) -ldl -lpthread 


run-test: $(TEST)
	touch ser.txt
	rm ser.txt
	echo "****WRITING****"
	./$(TEST) S ser.txt
	ls -la ser.txt
	echo "****READING****"
	./$(TEST) D ser.txt

clean:
	rm -f *.o *.so *.ktt dsu.c dsu.h $(TEST) ser.txt 
