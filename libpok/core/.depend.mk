allocator.o: allocator.c ../..//libpok/include/core/allocator.h \
 ../..//libpok/include/core/dependencies.h ../..//libpok/include/types.h \
 ../..//libpok/include/arch/x86/types.h
assert.o: assert.c
errno.o: errno.c ../..//libpok/include/types.h \
 ../..//libpok/include/arch/x86/types.h
errorconfirm.o: errorconfirm.c ../..//libpok/include/core/dependencies.h \
 ../..//libpok/include/core/error.h ../..//libpok/include/errno.h \
 ../..//libpok/include/types.h ../..//libpok/include/arch/x86/types.h
errorget.o: errorget.c ../..//libpok/include/core/dependencies.h \
 ../..//libpok/include/core/error.h ../..//libpok/include/errno.h \
 ../..//libpok/include/types.h ../..//libpok/include/arch/x86/types.h \
 ../..//libpok/include/core/syscall.h
errorhandlercreate.o: errorhandlercreate.c \
 ../..//libpok/include/core/error.h \
 ../..//libpok/include/core/dependencies.h ../..//libpok/include/errno.h \
 ../..//libpok/include/types.h ../..//libpok/include/arch/x86/types.h \
 ../..//libpok/include/core/syscall.h ../..//libpok/include/core/thread.h
errorhandlerworker.o: errorhandlerworker.c \
 ../..//libpok/include/core/error.h \
 ../..//libpok/include/core/dependencies.h ../..//libpok/include/errno.h \
 ../..//libpok/include/types.h ../..//libpok/include/arch/x86/types.h \
 ../..//libpok/include/core/partition.h \
 ../..//libpok/include/core/syscall.h ../..//libpok/include/core/thread.h \
 ../..//libpok/include/libc/stdio.h ../..//libpok/include/libc/stdarg.h \
 ../..//libpok/include/libc/string.h
errorignore.o: errorignore.c ../..//libpok/include/core/dependencies.h \
 ../..//libpok/include/core/error.h ../..//libpok/include/errno.h \
 ../..//libpok/include/types.h ../..//libpok/include/arch/x86/types.h
errorlog.o: errorlog.c ../..//libpok/include/core/dependencies.h \
 ../..//libpok/include/core/error.h ../..//libpok/include/errno.h \
 ../..//libpok/include/types.h ../..//libpok/include/arch/x86/types.h \
 ../..//libpok/include/core/time.h ../..//libpok/include/core/syscall.h
errorraise.o: errorraise.c ../..//libpok/include/core/dependencies.h \
 ../..//libpok/include/core/syscall.h ../..//libpok/include/errno.h \
 ../..//libpok/include/types.h ../..//libpok/include/arch/x86/types.h
eventbroadcast.o: eventbroadcast.c
eventcreate.o: eventcreate.c
eventlock.o: eventlock.c
eventsignal.o: eventsignal.c
eventunlock.o: eventunlock.c
eventwait.o: eventwait.c
main.o: main.c ../..//libpok/include/errno.h \
 ../..//libpok/include/types.h ../..//libpok/include/arch/x86/types.h
mutexcreate.o: mutexcreate.c
mutexlock.o: mutexlock.c
mutextrylock.o: mutextrylock.c
mutexunlock.o: mutexunlock.c
semcreate.o: semcreate.c ../..//libpok/include/core/dependencies.h \
 ../..//libpok/include/core/lockobj.h ../..//libpok/include/types.h \
 ../..//libpok/include/arch/x86/types.h \
 ../..//libpok/include/core/semaphore.h ../..//libpok/include/errno.h \
 ../..//libpok/include/core/syscall.h
semsignal.o: semsignal.c ../..//libpok/include/core/dependencies.h \
 ../..//libpok/include/core/lockobj.h ../..//libpok/include/types.h \
 ../..//libpok/include/arch/x86/types.h \
 ../..//libpok/include/core/semaphore.h ../..//libpok/include/errno.h \
 ../..//libpok/include/core/syscall.h
semstatus.o: semstatus.c ../..//libpok/include/core/dependencies.h \
 ../..//libpok/include/core/semaphore.h \
 ../..//libpok/include/core/lockobj.h ../..//libpok/include/types.h \
 ../..//libpok/include/arch/x86/types.h ../..//libpok/include/errno.h
semwait.o: semwait.c ../..//libpok/include/core/dependencies.h \
 ../..//libpok/include/core/lockobj.h ../..//libpok/include/types.h \
 ../..//libpok/include/arch/x86/types.h \
 ../..//libpok/include/core/semaphore.h ../..//libpok/include/errno.h \
 ../..//libpok/include/core/syscall.h
shutdown.o: shutdown.c ../..//libpok/include/core/dependencies.h
threadattrinit.o: threadattrinit.c \
 ../..//libpok/include/core/dependencies.h \
 ../..//libpok/include/core/thread.h ../..//libpok/include/core/syscall.h \
 ../..//libpok/include/errno.h ../..//libpok/include/types.h \
 ../..//libpok/include/arch/x86/types.h
threadcreate.o: threadcreate.c ../..//libpok/include/core/dependencies.h \
 ../..//libpok/include/arch.h ../..//libpok/include/types.h \
 ../..//libpok/include/arch/x86/types.h \
 ../..//libpok/include/core/syscall.h ../..//libpok/include/errno.h \
 ../..//libpok/include/core/thread.h
threaddelayedstart.o: threaddelayedstart.c \
 ../..//libpok/include/core/dependencies.h ../..//libpok/include/arch.h \
 ../..//libpok/include/types.h ../..//libpok/include/arch/x86/types.h \
 ../..//libpok/include/core/syscall.h ../..//libpok/include/errno.h \
 ../..//libpok/include/core/thread.h
threadid.o: threadid.c ../..//libpok/include/core/dependencies.h \
 ../..//libpok/include/arch.h ../..//libpok/include/types.h \
 ../..//libpok/include/arch/x86/types.h \
 ../..//libpok/include/core/syscall.h ../..//libpok/include/errno.h \
 ../..//libpok/include/core/thread.h
threadperiod.o: threadperiod.c ../..//libpok/include/core/dependencies.h \
 ../..//libpok/include/core/syscall.h ../..//libpok/include/errno.h \
 ../..//libpok/include/types.h ../..//libpok/include/arch/x86/types.h
threadpriority.o: threadpriority.c \
 ../..//libpok/include/core/dependencies.h ../..//libpok/include/arch.h \
 ../..//libpok/include/types.h ../..//libpok/include/arch/x86/types.h \
 ../..//libpok/include/core/syscall.h ../..//libpok/include/errno.h \
 ../..//libpok/include/core/thread.h
threadresume.o: threadresume.c ../..//libpok/include/core/dependencies.h \
 ../..//libpok/include/arch.h ../..//libpok/include/types.h \
 ../..//libpok/include/arch/x86/types.h \
 ../..//libpok/include/core/syscall.h ../..//libpok/include/errno.h \
 ../..//libpok/include/core/thread.h
threadsleep.o: threadsleep.c ../..//libpok/include/core/syscall.h \
 ../..//libpok/include/errno.h ../..//libpok/include/types.h \
 ../..//libpok/include/arch/x86/types.h
threadstatus.o: threadstatus.c ../..//libpok/include/core/dependencies.h \
 ../..//libpok/include/arch.h ../..//libpok/include/types.h \
 ../..//libpok/include/arch/x86/types.h \
 ../..//libpok/include/core/syscall.h ../..//libpok/include/errno.h \
 ../..//libpok/include/core/thread.h
timecomputedeadline.o: timecomputedeadline.c
timeget.o: timeget.c ../..//libpok/include/core/dependencies.h \
 ../..//libpok/include/core/time.h ../..//libpok/include/core/syscall.h \
 ../..//libpok/include/errno.h ../..//libpok/include/types.h \
 ../..//libpok/include/arch/x86/types.h
