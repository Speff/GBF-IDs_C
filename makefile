CC=g++
CFLAGS=-I$(INCDIR) -m32 -Wall -Wno-comment -O2 #-mwindows
STATIC_FLAGS=

INCDIR=./include
BINDIR=./bin
SRCDIR=./src
OBJDIR=./obj
LIBDIR=./lib

# Things on the left have dependencies fulfiled by things on the right
LIBS=-static -static-libgcc -static-libstdc++ -lm -lcrypto -lgdi32 -loauth -lcurldll -lSOIL -lglfw3dll -lopengl32 -lfreetype.dll

_DEPS=Window.h gl_core_3_3.h ft2build.h
DEPS=$(patsubst %,$(INCDIR)/%,$(_DEPS))

_OBJ=gl_core_3_3.o Window.o oauth.o xmalloc.o hash.o
OBJ=$(patsubst %,$(OBJDIR)/%,$(_OBJ))

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(DEPS)
	$(CC) $(STATIC_FLAGS) -c -o $@ $< $(CFLAGS)

GBF-IDs: $(OBJ)
	$(CC) -o $(BINDIR)/$@ $^ $(INCDIR)/progSettings.o $(CFLAGS) -L$(LIBDIR) $(LIBS)

.PHONY: clean

clean:
	rm -f $(OBJDIR)/*.o *~ core $(INCDIR)/*~
