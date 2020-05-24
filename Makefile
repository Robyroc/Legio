ULFM_PREFIX ?= $(CURDIR)/./ulfm/build
ULFM_FILE = $(ULFM_PREFIX)/bin/mpiexec
CC = $(ULFM_PREFIX)/bin/mpicc

.PHONY = run ulfm

ulfm: $(ULFM_FILE)
	@echo $(CC)

$(ULFM_FILE):
	git clone --recursive https://bitbucket.org/icldistcomp/ulfm2/src/ulfm/
	cd ulfm; \
	./autogen.pl; \
	mkdir build; \
	./configure --with-ft --prefix=$(ULFM_PREFIX); \
	make all; \
	make install

run: $(NAME)
	$(ULFM_PREFIX)/bin/mpiexec $(NAME) 