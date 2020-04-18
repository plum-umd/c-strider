# Redis Makefile
# Copyright (C) 2009 Salvatore Sanfilippo <antirez at gmail dot com>
# This file is released under the BSD license, see the COPYING file

LASTVERSION=2.0.2
#LASTVERSION=2.0.1
VERSION=2.0.2

# EKIDEN
EKINC  = -I../../bin/
EKLIB  = ../../bin/libcstrider.a
CSTRLINK  = -Wall -ldl -I../../src -I../../contrib/uthash/ 
CFLAGS = -O2 -Wall -ldl  $(EKINC) 
EKCC = ../../bin/ktcc --doglobalreg --keepunused -include fmacros.h -include cstrider_api.h 
EKJOIN = ../../bin/kttjoin
EKGEN = ../../bin/xfgen

USER_XFORM=redis.xf
GENERATED_XFORM=dsu.c

release_hdr := $(shell sh -c './mkreleasehdr.sh')
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
OPTIMIZATION?=-O2
ifeq ($(uname_S),SunOS)
  CFLAGS?= -std=c99 -pedantic $(OPTIMIZATION) -Wall -W -D__EXTENSIONS__ -D_XPG6
  CCLINK?= -ldl -lnsl -lsocket -lm -lpthread -lrt
else
  CFLAGS?= -std=c99 -pedantic $(OPTIMIZATION) -Wall -W $(ARCH) $(PROF)
  CCLINK?= -lm -pthread -lrt
endif
CCOPT= $(CFLAGS) $(CCLINK) $(ARCH) $(PROF)
DEBUG?= -g -rdynamic -ggdb 

OBJ = adlist.o ae.o anet.o dict.o redis.o sds.o zmalloc.o lzf_c.o lzf_d.o pqsort.o zipmap.o sha1.o
BENCHOBJ = ae.o anet.o redis-benchmark.o sds.o adlist.o zmalloc.o
CLIOBJ = anet.o sds.o adlist.o redis-cli.o zmalloc.o linenoise.o
CHECKDUMPOBJ = redis-check-dump.o lzf_c.o lzf_d.o
CHECKAOFOBJ = redis-check-aof.o

RUNLIBNAME = redis-server
BENCHPRGNAME = redis-benchmark
CLIPRGNAME = redis-cli
CHECKDUMPPRGNAME = redis-check-dump
CHECKAOFPRGNAME = redis-check-aof

all: redis-server #redis-benchmark redis-cli redis-check-dump redis-check-aof

# Deps (use make dep to generate this)
adlist.o: adlist.c adlist.h zmalloc.h
ae.o: ae.c ae.h zmalloc.h config.h ae_kqueue.c
ae_epoll.o: ae_epoll.c
ae_kqueue.o: ae_kqueue.c
ae_select.o: ae_select.c
anet.o: anet.c fmacros.h anet.h
dict.o: dict.c fmacros.h dict.h zmalloc.h
dict2.o: dict2.c fmacros.h dict2.h zmalloc.h
linenoise.o: linenoise.c fmacros.h
lzf_c.o: lzf_c.c lzfP.h
lzf_d.o: lzf_d.c lzfP.h
pqsort.o: pqsort.c
printraw.o: printraw.c
redis-benchmark.o: redis-benchmark.c fmacros.h ae.h anet.h sds.h adlist.h \
  zmalloc.h
redis-check-aof.o: redis-check-aof.c fmacros.h config.h
redis-check-dump.o: redis-check-dump.c lzf.h
redis-cli.o: redis-cli.c fmacros.h anet.h sds.h adlist.h zmalloc.h \
  linenoise.h
redis.o: redis.c fmacros.h config.h redis.h ae.h sds.h anet.h dict.h \
  adlist.h zmalloc.h lzf.h pqsort.h zipmap.h staticsymbols.h sha1.h
sds.o: sds.c sds.h zmalloc.h
zipmap.o: zipmap.c zmalloc.h
zmalloc.o: zmalloc.c config.h

redis-server: $(OBJ) dsu.o perfaction.o
	$(CC)  -o $(RUNLIBNAME) $(CSTRLINK) $(DEBUG) $^  $(EKINC) -I../../../contrib/uthash/ $(EKLIB) $(CCLINK) 

dsu.o: dsu.c
	$(CC)  -c $^ -ggdb  -I../../src -I../../contrib/uthash/  -ldl -lpthread 

perfaction.o: perfaction.c dsu.c logprinter.o
	$(CC)  -c -ggdb perfaction.c -I../../contrib/uthash/

logprinter.o:
	$(CC) logprinter.c -o logprinter

# redis-benchmark: $(BENCHOBJ)
# 	$(CC) -o $(BENCHPRGNAME) $(CCOPT) $(DEBUG) $(BENCHOBJ)

# redis-cli: $(CLIOBJ)
# 	$(CC) -o $(CLIPRGNAME) $(CCOPT) $(DEBUG) $(CLIOBJ)

# redis-check-dump: $(CHECKDUMPOBJ)
# 	$(CC) -o $(CHECKDUMPPRGNAME) $(CCOPT) $(DEBUG) $(CHECKDUMPOBJ)

# redis-check-aof: $(CHECKAOFOBJ)
# 	$(CC) -o $(CHECKAOFPRGNAME) $(CCOPT) $(DEBUG) $(CHECKAOFOBJ)

redis-$(VERSION).ktt: $(OBJ)
	$(EKJOIN) $@ $(patsubst %.o,%.ktt,$(OBJ))

$(GENERATED_XFORM): redis-$(VERSION).ktt $(USER_XFORM)
	$(EKGEN) $@ ../redis-$(LASTVERSION)/redis-$(LASTVERSION).ktt redis-$(VERSION).ktt $(USER_XFORM)

.c.o:
	$(EKCC) -c $(CFLAGS) -fvisibility=hidden $(DEBUG) $(COMPILE_TIME) $< --doktsavetypes --typesfile-out=$*.ktt

clean:
	rm -rf $(RUNLIBNAME) $(BENCHPRGNAME) $(CLIPRGNAME) $(CHECKDUMPPRGNAME) $(CHECKAOFPRGNAME) *.o *.cil.c *.i *.gcda *.gcno *.gcov *.ktt $(GENERATED_XFORM) logprinter 

dep:
	$(CC) -MM *.c

staticsymbols:
	tclsh utils/build-static-symbols.tcl > staticsymbols.h

test:
	tclsh8.5 tests/test_helper.tcl --tags "${TAGS}"

bench:
	./redis-benchmark

log:
	git log '--pretty=format:%ad %s (%cn)' --date=short > Changelog

32bit:
	@echo ""
	@echo "WARNING: if it fails under Linux you probably need to install libc6-dev-i386"
	@echo ""
	make ARCH="-m32"

gprof:
	make PROF="-pg"

gcov:
	make PROF="-fprofile-arcs -ftest-coverage"

noopt:
	make OPTIMIZATION=""

32bitgprof:
	make PROF="-pg" ARCH="-arch i386"
