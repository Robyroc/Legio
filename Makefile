ULFM_PREFIX ?= ./ulfm/build
ULFM_FILE = $(ULFM_PREFIX)/bin/mpirun
NAME = name
CC = $(ULFM_PREFIX)/bin/mpicc
CXX = $(ULFM_PREFIX)/bin/mpicxx
COMMONFLAGS = -Wall -O3 $(INCFLAGS)
CFLAGS = $(COMMONFLAGS)
CPPFLAGS = -std=c++11 $(COMMONFLAGS)
LDFLAGS = -lm -L$(ULFM_PREFIX)/lib
SRCDIR = ./src
INCFLAGS = -I$(ULFM_PREFIX)/include -I./include
BINDIR = /usr/local/bin
OBJDIR = ./build
CSOURCES = $(wildcard $(SRCDIR)/*.c)
CPPSOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(CSOURCES))
OBJECTS += $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(CPPSOURCES))
TESTDIR = ./testsrc
TESTSRC = $(TESTDIR)/$(TEST).c
TESTOBJ = $(OBJDIR)/$(TEST).o
TUTORIALNAME = tutorial
LEGIOTESTCSRC = $(wildcard $(LEGIOTESTDIR)/*.c)
LEGIOTESTCPPSRC = $(wildcard $(LEGIOTESTDIR)/*.cpp)
LEGIOTESTOBJECTS = $(patsubst $(LEGIOTESTDIR)/%.c, $(OBJDIR)/%.o, $(LEGIOTESTCSRC))
LEGIOTESTOBJECTS += $(patsubst $(LEGIOTESTDIR)/%.cpp, $(OBJDIR)/%.o, $(LEGIOTESTCPPSRC))


.PHONY = all clean install uninstall run test

all: $(ULFM_FILE) $(NAME)

$(TUTORIALNAME): $(ULFM_FILE) $(TESTSRC) 
	$(CXX) $(word 2,$^) -o $@ $(CPPFLAGS) $(LDFLAGS)

$(TESTOBJ): $(TESTSRC)
	$(CC) -c $< -o $@ $(CFLAGS) $(LDFLAGS)

$(NAME): $(OBJECTS)
	$(CXX) $^ -o $@ $(CPPFLAGS) $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS) $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) -c $< -o $@ $(CPPFLAGS) $(LDFLAGS)

$(ULFM_FILE):
	git clone --recursive https://bitbucket.org/icldistcomp/ulfm2/src/ulfm/
	cd ulfm; \
	./autogen.pl; \
	mkdir build; \
	./configure --with-ft --prefix="$(shell pwd)"/$(ULFM_PREFIX); \
	make all; \
	make install
clean:
	rm -f $(NAME)
	rm -f $(OBJDIR)/*.o
	rm -f $(TUTORIALNAME)
install: all
	cp $(NAME) $(BINDIR)
uninstall:
	rm -f $(BINDIR)/$(NAME)

legio_test: $(LEGIOTESTOBJECTS) $(OBJECTS)
	echo  $(LEGIOTESTOBJECTS)
	$(CXX) $^ -o $(NAME) $(CPPFLAGS) $(LDFLAGS)

$(OBJDIR)/%.o: $(LEGIOTESTDIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS) $(LDFLAGS)

$(OBJDIR)/%.o: $(LEGIOTESTDIR)/%.cpp
	$(CXX) -c $< -o $@ $(CPPFLAGS) $(LDFLAGS)

run: all
	$(ULFM_PREFIX)/bin/mpiexec $(NAME) 
	
run_tutorial: $(TUTORIALNAME)
	$(ULFM_PREFIX)/bin/mpiexec $(TUTORIALNAME)
