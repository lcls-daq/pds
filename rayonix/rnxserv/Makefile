RAYONIX_ROOT = /opt/rayonix
CRAYDL_INC = $(RAYONIX_ROOT)/include
RAYONIX_LIB = $(RAYONIX_ROOT)/lib64

# hello: Makefile hello.cc
# 	$(CXX)    -Wall -I $(CRAYDL_INC) -o hello.o -c hello.cc
# 	$(CXX)    -Wall hello.o -o hello -rdynamic -L $(RAYONIX_LIB) -lcraydl

all: rnxserv

# Notes to build on rhel7 add the flag -lboost_thread-mt for now: Instructions for Michael Blum 08/2018
#   - this should be fixed in a future release of craydl
rnxserv: rnxserv.o helper.o workThread.o triggerThread.o rnxlog.o
	g++ -o rnxserv rnxserv.o helper.o workThread.o triggerThread.o rnxlog.o -lpthread -std=c++0x -Wall -rdynamic -L $(RAYONIX_LIB) -lcraydl

rnxserv.o: rnxserv.c helper.h workThread.h triggerThread.h rnxState.h rnxlog.h rnxcommon.h
	g++ -o rnxserv.o rnxserv.c -c -ansi -pedantic -std=c++0x -Wall

rnxlog.o: rnxlog.c rnxlog.h
	g++ -o rnxlog.o rnxlog.c -c -ansi -pedantic -std=c++0x -Wall

rnxtrigger.o: rnxtrigger.c rnxtrigger.h
	g++ -o rnxtrigger.o rnxtrigger.c -c -ansi -pedantic -std=c++0x -Wall

helper.o: helper.c helper.h
	g++ -o helper.o helper.c -c -ansi -pedantic -std=c++0x -Wall

workThread.o: workThread.c workThread.h rnxState.h rnxlog.h rnxcommon.h rnxdata.h
	g++ -I $(CRAYDL_INC) -o workThread.o workThread.c -c -ansi -std=c++0x -Wall

triggerThread.o: triggerThread.c triggerThread.h workThread.h rnxlog.h rnxcommon.h
	g++ -o triggerThread.o triggerThread.c -c -ansi -pedantic -std=c++0x -Wall

clean:
	rm -f *.o *~

distclean: clean
	rm -f rnxserv
