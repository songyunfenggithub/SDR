# Microsoft Developer Studio Generated NMAKE File, Based on myWave.dsp
!IF "$(CFG)" == ""
CFG=myWave - Win32 Debug
!MESSAGE No configuration specified. Defaulting to myWave - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "myWave - Win32 Release" && "$(CFG)" != "myWave - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "myWave.mak" CFG="myWave - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "myWave - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "myWave - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "myWave - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\myWave.exe"


CLEAN :
	-@erase "$(INTDIR)\Devices.obj"
	-@erase "$(INTDIR)\File.obj"
	-@erase "$(INTDIR)\myDebug.obj"
	-@erase "$(INTDIR)\myWave.obj"
	-@erase "$(INTDIR)\myWave.pch"
	-@erase "$(INTDIR)\myWave.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\Wave.obj"
	-@erase "$(INTDIR)\WinMain.obj"
	-@erase "$(OUTDIR)\myWave.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Fp"$(INTDIR)\myWave.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x804 /fo"$(INTDIR)\myWave.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\myWave.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\myWave.pdb" /machine:I386 /out:"$(OUTDIR)\myWave.exe" 
LINK32_OBJS= \
	"$(INTDIR)\Devices.obj" \
	"$(INTDIR)\myDebug.obj" \
	"$(INTDIR)\myWave.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\Wave.obj" \
	"$(INTDIR)\WinMain.obj" \
	"$(INTDIR)\myWave.res" \
	"$(INTDIR)\File.obj"

"$(OUTDIR)\myWave.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "myWave - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\myWave.exe"


CLEAN :
	-@erase "$(INTDIR)\Devices.obj"
	-@erase "$(INTDIR)\File.obj"
	-@erase "$(INTDIR)\myDebug.obj"
	-@erase "$(INTDIR)\myWave.obj"
	-@erase "$(INTDIR)\myWave.pch"
	-@erase "$(INTDIR)\myWave.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\Wave.obj"
	-@erase "$(INTDIR)\WinMain.obj"
	-@erase "$(OUTDIR)\myWave.exe"
	-@erase "$(OUTDIR)\myWave.ilk"
	-@erase "$(OUTDIR)\myWave.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Fp"$(INTDIR)\myWave.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x804 /fo"$(INTDIR)\myWave.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\myWave.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\myWave.pdb" /debug /machine:I386 /out:"$(OUTDIR)\myWave.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\Devices.obj" \
	"$(INTDIR)\myDebug.obj" \
	"$(INTDIR)\myWave.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\Wave.obj" \
	"$(INTDIR)\WinMain.obj" \
	"$(INTDIR)\myWave.res" \
	"$(INTDIR)\File.obj"

"$(OUTDIR)\myWave.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("myWave.dep")
!INCLUDE "myWave.dep"
!ELSE 
!MESSAGE Warning: cannot find "myWave.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "myWave - Win32 Release" || "$(CFG)" == "myWave - Win32 Debug"
SOURCE=.\Devices.cpp

"$(INTDIR)\Devices.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\myWave.pch"


SOURCE=.\File.cpp

"$(INTDIR)\File.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\myWave.pch"


SOURCE=.\myDebug.cpp

"$(INTDIR)\myDebug.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\myWave.pch"


SOURCE=.\myWave.cpp

"$(INTDIR)\myWave.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\myWave.pch"


SOURCE=.\myWave.rc

"$(INTDIR)\myWave.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "myWave - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Fp"$(INTDIR)\myWave.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\myWave.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "myWave - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Fp"$(INTDIR)\myWave.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\myWave.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Wave.cpp

"$(INTDIR)\Wave.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\myWave.pch"


SOURCE=.\WinMain.cpp

"$(INTDIR)\WinMain.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\myWave.pch"



!ENDIF 

