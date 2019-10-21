STRIP ?= strip
BINDIR = ../bin
CFLAGS = -D_GNU_SOURCE -fno-exceptions -O2 -std=c++11 -Wall -g
LFLAGS = -lole32
CCOPT = -D_WIN32_WINNT=0x0600 -DUNICODE -D_UNICODE
CXX = g++
ifdef RELEASE
LFLAGS += -static -static-libgcc -static-libstdc++
endif

OBJNAMES=common.obj Helpers.obj LocalSock.obj TerminalState.obj WinHelper.obj wslbridge2.obj GetVmId.obj WinsockModule.obj
OBJS=$(patsubst %.obj, $(BINDIR)/%.obj, $(OBJNAMES))

all : $(BINDIR) $(BINDIR)/wslbridge2.exe
$(BINDIR)/wslbridge2.exe: $(OBJS)
	$(CXX) $^ $(LFLAGS) -o $@
$(BINDIR)/%.obj: %.cpp $(wildcard *.hpp)
	$(CXX) $(CFLGAS) $(CCOPT) -c $< -o $@
$(BINDIR):
	mkdir -p $(BINDIR)
