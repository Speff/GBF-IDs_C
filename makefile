CC=g++
CFLAGS=-I$(INCDIR) -m32 -Wall -O2 -static-libgcc -static-libstdc++
STATIC_FLAGS=

INCDIR=./include
BINDIR=./bin
SRCDIR=./src
OBJDIR=./obj
LIBDIR=./lib

LIBS=-lm -lfreetype.dll -lglfw3dll -lopengl32

_DEPS=Window.h gl_core_3_3.h ft2build.h
DEPS=$(patsubst %,$(INCDIR)/%,$(_DEPS))

_OBJ=gl_core_3_3.o Window.o
OBJ=$(patsubst %,$(OBJDIR)/%,$(_OBJ))

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(DEPS)
	$(CC) $(STATIC_FLAGS) -c -o $@ $< $(CFLAGS)

GBF-IDs: $(OBJ)
	$(CC) -o $(BINDIR)/$@ $^ $(CFLAGS) -L$(LIBDIR) $(LIBS)

.PHONY: clean

clean:
	rm -f $(OBJDIR)/*.o *~ core $(INCDIR)/*~
