ULFM_PREFIX ?= $(CURDIR)/ompi/install
ULFM_FILE = $(ULFM_PREFIX)/bin/mpiexec
CC = $(ULFM_PREFIX)/bin/mpicc

.PHONY = run ulfm

ulfm: $(ULFM_FILE)
	@echo $(CC)

$(ULFM_FILE):
	git clone --depth 1 --recursive -j 8 https://github.com/open-mpi/ompi
	cd ompi; \
	git checkout v5.0.x; \
	git submodule update --init; \
	./autogen.pl; \
	mkdir build; \
	mkdir install; \
	cd build; \
	../configure --with-ft=ulfm --prefix=$(ULFM_PREFIX); \
	make all; \
	make install

run: $(NAME)
	$(ULFM_PREFIX)/bin/mpiexec $(NAME) 