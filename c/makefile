
all: dm.exe dm_raw.exe dm_raw_auto.exe dmenc.exe dmdec.exe dedm.exe dedm_raw.exe dmplay.exe

experimental: dm_raw1.exe dm_rawlr.exe dm_raw_autodelta.exe calcdyn.exe

tst.obj:  tst.c tst.h

dm.obj:   dm.c tst.h dmcommon.h

dm_raw.obj:   dm_raw.c tst.h dmcommon.h

dm_raw_auto.obj:   dm_raw_auto.c tst.h dmcommon.h

dm_raw_autodelta.obj:   dm_raw_autodelta.c tst.h dmcommon.h

dmenc.obj:   dmenc.c tst.h dmcommon.h

dmdec.obj:   dmdec.c tst.h dmcommon.h

dm_raw1.obj:   dm_raw1.c tst.h dmcommon.h

dm_rawlr.obj:   dm_rawlr.c tst.h dmcommon.h

dedm.obj:   dedm.c tst.h dmcommon.h

dedm_raw.obj:   dedm_raw.c tst.h dmcommon.h

dmplay_win.obj:   dmplay_win.c tst.h dmcommon.h

dmcommon.obj:  dmcommon.c dmcommon.h

calcdyn.obj: calcdyn.c tst.h

dm.exe: dm.obj dmcommon.obj tst.obj 
	link $(LFLAGS) dm dmcommon tst /nologo /out:dm.exe
	
dm_raw.exe: dm_raw.obj dmcommon.obj tst.obj 
	link $(LFLAGS) dm_raw dmcommon tst /nologo /out:dm_raw.exe
	
dm_raw1.exe: dm_raw1.obj dmcommon.obj tst.obj 
	link $(LFLAGS) dm_raw1 dmcommon tst /nologo /out:dm_raw1.exe

dm_rawlr.exe: dm_rawlr.obj dmcommon.obj tst.obj 
	link $(LFLAGS) dm_rawlr dmcommon tst /nologo /out:dm_rawlr.exe

dm_raw_auto.exe: dm_raw_auto.obj dmcommon.obj tst.obj 
	link $(LFLAGS) dm_raw_auto dmcommon tst /nologo /out:dm_raw_auto.exe
	
dm_raw_autodelta.exe: dm_raw_autodelta.obj dmcommon.obj tst.obj 
	link $(LFLAGS) dm_raw_autodelta dmcommon tst /nologo /out:dm_raw_autodelta.exe
	
dmenc.exe: dmenc.obj dmcommon.obj tst.obj 
	link $(LFLAGS) dmenc dmcommon tst /nologo /out:dmenc.exe

dmdec.exe: dmdec.obj dmcommon.obj tst.obj 
	link $(LFLAGS) dmdec dmcommon tst /nologo /out:dmdec.exe
	
dedm.exe: dedm.obj dmcommon.obj tst.obj 
	link $(LFLAGS) dedm dmcommon tst /nologo /out:dedm.exe

dedm_raw.exe: dedm_raw.obj dmcommon.obj tst.obj 
	link $(LFLAGS) dedm_raw dmcommon tst /nologo /out:dedm_raw.exe

dmplay.exe: dmplay_win.obj dmcommon.obj tst.obj 
	link $(LFLAGS) dmplay_win dmcommon tst winmm.lib /nologo /out:dmplay.exe	

calcdyn.exe: calcdyn.obj tst.obj 
	link $(LFLAGS) calcdyn tst /nologo /out:calcdyn.exe

.c.obj:
	cl $(CFLAGS) -nologo -c -O2 $*.c

clean:  
	del *.exe
	del *.obj