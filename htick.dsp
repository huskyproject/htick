# Microsoft Developer Studio Project File - Name="htick" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=htick - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "htick.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "htick.mak" CFG="htick - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "htick - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "htick - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "htick - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\nd_r\bin"
# PROP Intermediate_Dir "..\nd_r\obj\htick"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I ".." /I ".\h" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "__NT__" /D "_MAKE_DLL" /FR /YX /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 msvcrt.lib Kernel32.lib user32.lib smapimvc.lib fconfmvc.lib /nologo /subsystem:console /machine:I386 /nodefaultlib /libpath:"..\nd_r\lib"

!ELSEIF  "$(CFG)" == "htick - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\nd_d\bin"
# PROP Intermediate_Dir "..\nd_d\obj\htick"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I ".." /I ".\h" /I "..\h" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "__NT__" /D "_MAKE_DLL" /D _WIN32_WINNT=0x0400 /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 msvcrtd.lib Kernel32.lib user32.lib smapimvc.lib fconfmvc.lib /nologo /subsystem:console /profile /debug /machine:I386 /nodefaultlib /libpath:"..\nd_d\lib"

!ENDIF 

# Begin Target

# Name "htick - Win32 Release"
# Name "htick - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\src\add_desc.c
# End Source File
# Begin Source File

SOURCE=.\src\areafix.c
# End Source File
# Begin Source File

SOURCE=.\src\clean.c
# End Source File
# Begin Source File

SOURCE=.\src\fcommon.c
# End Source File
# Begin Source File

SOURCE=.\src\filecase.c
# End Source File
# Begin Source File

SOURCE=.\src\filelist.c
# End Source File
# Begin Source File

SOURCE=.\src\global.c
# End Source File
# Begin Source File

SOURCE=.\src\hatch.c
# End Source File
# Begin Source File

SOURCE=.\src\htick.c
# End Source File
# Begin Source File

SOURCE=.\src\report.c
# End Source File
# Begin Source File

SOURCE=.\src\scan.c
# End Source File
# Begin Source File

SOURCE=.\src\seenby.c
# End Source File
# Begin Source File

SOURCE=.\src\toss.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\h\add_desc.h
# End Source File
# Begin Source File

SOURCE=.\h\areafix.h
# End Source File
# Begin Source File

SOURCE=.\h\fcommon.h
# End Source File
# Begin Source File

SOURCE=.\h\filecase.h
# End Source File
# Begin Source File

SOURCE=.\h\filelist.h
# End Source File
# Begin Source File

SOURCE=.\h\global.h
# End Source File
# Begin Source File

SOURCE=.\h\hatch.h
# End Source File
# Begin Source File

SOURCE=.\h\htick.h
# End Source File
# Begin Source File

SOURCE=.\h\report.h
# End Source File
# Begin Source File

SOURCE=.\h\scan.h
# End Source File
# Begin Source File

SOURCE=.\h\seenby.h
# End Source File
# Begin Source File

SOURCE=.\h\toss.h
# End Source File
# Begin Source File

SOURCE=.\h\version.h
# End Source File
# End Group
# End Target
# End Project
