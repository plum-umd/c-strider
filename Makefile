TOOLS_DIR = tools/

DIST_SRC = src
DIST_DEST = bin

DIST_INCLUDE = cstrider_api.h
DIST_LIB = libcstrider.a 
DIST_FILES =  $(DIST_LIB) $(DIST_INCLUDE)


INSTALL_PATH_LIB = /usr/local/lib/
INSTALL_PATH_BIN = /usr/local/bin/
INSTALL_PATH_INCLUDE = /usr/local/include/

DIST_DEST_FILES = $(addprefix $(DIST_DEST)/, $(DIST_FILES))
all: $(DIST_DEST_FILES) buildtools

.PHONY: buildlib buildtools libclean clean distclean


buildlib:
	$(MAKE) -C $(DIST_SRC)

buildtools:
	$(MAKE) -C $(TOOLS_DIR)

$(DIST_DEST)/% : buildlib
	cp $(DIST_SRC)/$* $@

libclean:
	rm -f $(DIST_DEST_FILES)

clean: libclean 
	$(MAKE) -C $(DIST_SRC) clean
	$(MAKE) -C $(TOOLS_DIR) clean

distclean: clean
	$(MAKE) -C $(TOOLS_DIR) distclean

install: buildtools $(DIST_DEST_FILES) install-lib install-bin install-include install-tools

install-lib: $(addprefix $(DIST_DEST)/, $(DIST_LIB))
	install $^ $(INSTALL_PATH_LIB)


install-include: $(addprefix $(DIST_DEST)/, $(DIST_INCLUDE))
	install -m 644 $^ $(INSTALL_PATH_INCLUDE)

install-tools: 
	install bin/xfgen $(INSTALL_PATH_BIN)
	install bin/ktcc $(INSTALL_PATH_BIN)
	install bin/kttjoin $(INSTALL_PATH_BIN)

install-clean: 
	-rm $(addprefix $(INSTALL_PATH_BIN)/,  xfgen kttjoin ktcc) $(addprefix $(INSTALL_PATH_INCLUDE)/,$(DIST_INCLUDE)) $(addprefix $(INSTALL_PATH_LIB)/,$(DIST_LIB))
