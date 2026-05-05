blackboardclear.o: blackboardclear.c
blackboardcreate.o: blackboardcreate.c \
 ../..//libpok/include/core/dependencies.h
blackboarddisplay.o: blackboarddisplay.c \
 ../..//libpok/include/core/dependencies.h
blackboardid.o: blackboardid.c
blackboardinit.o: blackboardinit.c
blackboardread.o: blackboardread.c \
 ../..//libpok/include/core/dependencies.h
blackboardstatus.o: blackboardstatus.c
buffercreate.o: buffercreate.c ../..//libpok/include/core/dependencies.h
bufferid.o: bufferid.c
bufferinit.o: bufferinit.c
bufferreceive.o: bufferreceive.c \
 ../..//libpok/include/core/dependencies.h
buffersend.o: buffersend.c ../..//libpok/include/core/dependencies.h
bufferstatus.o: bufferstatus.c
portqueueingcreate.o: portqueueingcreate.c \
 ../..//libpok/include/core/dependencies.h \
 ../..//libpok/include/core/syscall.h ../..//libpok/include/errno.h \
 ../..//libpok/include/types.h ../..//libpok/include/arch/x86/types.h \
 ../..//libpok/include/middleware/port.h \
 ../..//libpok/include/core/lockobj.h
portqueueingreceive.o: portqueueingreceive.c \
 ../..//libpok/include/core/dependencies.h \
 ../..//libpok/include/core/syscall.h ../..//libpok/include/errno.h \
 ../..//libpok/include/types.h ../..//libpok/include/arch/x86/types.h \
 ../..//libpok/include/middleware/port.h \
 ../..//libpok/include/core/lockobj.h
portqueueingsend.o: portqueueingsend.c \
 ../..//libpok/include/core/dependencies.h \
 ../..//libpok/include/core/syscall.h ../..//libpok/include/errno.h \
 ../..//libpok/include/types.h ../..//libpok/include/arch/x86/types.h \
 ../..//libpok/include/middleware/port.h \
 ../..//libpok/include/core/lockobj.h
portsamplingcreate.o: portsamplingcreate.c \
 ../..//libpok/include/core/dependencies.h
portsamplingread.o: portsamplingread.c \
 ../..//libpok/include/core/dependencies.h
portsamplingwrite.o: portsamplingwrite.c \
 ../..//libpok/include/core/dependencies.h
portvirtualcreate.o: portvirtualcreate.c \
 ../..//libpok/include/core/dependencies.h
portvirtualdestination.o: portvirtualdestination.c \
 ../..//libpok/include/core/dependencies.h
portvirtualgetglobal.o: portvirtualgetglobal.c \
 ../..//libpok/include/core/dependencies.h
portvirtualnbdestinations.o: portvirtualnbdestinations.c \
 ../..//libpok/include/core/dependencies.h
portvirtualutils.o: portvirtualutils.c \
 ../..//libpok/include/core/syscall.h ../..//libpok/include/errno.h \
 ../..//libpok/include/types.h ../..//libpok/include/arch/x86/types.h \
 ../..//libpok/include/middleware/port.h \
 ../..//libpok/include/core/dependencies.h \
 ../..//libpok/include/core/lockobj.h
queueinit.o: queueinit.c
ressources.o: ressources.c
