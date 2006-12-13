# $Id$
#
# Common makefiles part (include-file) for htick

# Use defines before including this file:
# _OBJ	object files suffix begins with point (".o", ".obj", ... )
# _LIB	library files suffix begins with point (".a", ".lib", ... )
# _DLL	dinamic linking library files suffix begins with point (".dll", ".so", ...)
# LIBPREFIX  library filename prefix ("lib", "", ...)
# LIBSUFFIX  library filename suffix (compiler ID, three characters, somewhere empty)
# DLLPREFIX  dinamic linking library prefix (usually empty)
# DLLSUFFIX  dinamic linking library suffix (compiler ID or empty)
# _SRC_DIR  sources dir


LIBNAME = htick

# Version
VERMAJOR= 1
VERMINOR= 9
VERPATCH= 0
VERH	= $(VERMAJOR).$(VERMINOR)
VER	= $(VERH).$(VERPATCH)

# Object files (please sort list to easy check by human)
OBJS1	= add_desc$(_OBJ) htickafix$(_OBJ) clean$(_OBJ) fcommon$(_OBJ)
OBJS2	= filecase$(_OBJ) filelist$(_OBJ) global$(_OBJ) hatch$(_OBJ)
OBJS3	= htick$(_OBJ) report$(_OBJ) scan$(_OBJ) seenby$(_OBJ) toss$(_OBJ)
OBJS	= $(OBJS1) $(OBJS2) $(OBJS3)

# Header files
HEADERS	= add_desc.h cvsdate.h fcommon.h filecase.h filelist.h global.h \
	  hatch.h htick.h htickafix.h report.h scan.h seenby.h toss.h version.h
#areafix.h 

# Sources directory
_SRC_DIR= ../src/

# Headers directory
H_DIR	= ../h/

# Binary file(s) to build from sources
PROGRAMS= $(LIBNAME)$(_EXE)

# List of libraries required to build binary file(s)
LIBS	=
