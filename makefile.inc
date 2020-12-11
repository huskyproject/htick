# $Id$
#
# Common makefiles part (include-file) for htick

# Use defines before including this file:
# _OBJ  object files suffix begins with point (".o", ".obj", ... )
# _LIB  library files suffix begins with point (".a", ".lib", ... )
# _DLL  dinamic linking library files suffix begins with point (".dll", ".so", ...)
# LIBPREFIX  library filename prefix ("lib", "", ...)
# LIBSUFFIX  library filename suffix (compiler ID, three characters, somewhere empty)
# DLLPREFIX  dynamic linking library prefix (usually empty)
# DLLSUFFIX  dynamic linking library suffix (compiler ID or empty)
# O, B, L, H prefixes to use with names of objects, binaries, libraries and headers respectively

# Module name
LIBNAME = htick

# Version
VERMAJOR= 1
VERMINOR= 9
VERPATCH= 0
VERH    = $(VERMAJOR).$(VERMINOR)
VER     = $(VERH).$(VERPATCH)

# Sources directory
_SRC_DIR= ../src/
# Headers directory
H_DIR   = ../h/

# Header files
HEADERS = $(H)add_desc.h $(H)htickafix.h ../cvsdate.h $(H)fcommon.h \
          $(H)filecase.h $(H)filelist.h $(H)global.h $(H)hatch.h $(H)htick.h \
          $(H)report.h $(H)scan.h $(H)seenby.h $(H)toss.h $(H)version.h

# Object files (please sort list to easy check by human)
OBJS1   = $(O)global$(_OBJ) $(O)add_desc$(_OBJ) $(O)htickafix$(_OBJ) $(O)clean$(_OBJ)
OBJS2   = $(O)fcommon$(_OBJ) $(O)filecase$(_OBJ) $(O)filelist$(_OBJ)
OBJS3   = $(O)hatch$(_OBJ) $(O)htick$(_OBJ) $(O)report$(_OBJ)
OBJS4   = $(O)scan$(_OBJ) $(O)seenby$(_OBJ) $(O)toss$(_OBJ)
OBJS    = $(OBJS1) $(OBJS2) $(OBJS3) $(OBJS4)

# Object files: programs
PRGOBJS = $(OBJS)

# Binary file(s) to build from sources
PROGRAMS= $(B)$(LIBNAME)$(_EXE)

# List of libraries required to build binary file(s)
LIBS    =
