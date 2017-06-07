
all:
	mingw32-gcc -c PromIf.c -o ./obj/PromIf.o -mwindows -Wall -Wextra -pedantic -DUNICODE -mthreads -D_MT
	mingw32-gcc -c PromTool.c -o ./obj/PromTool.o -mwindows -Wall -Wextra -pedantic -DUNICODE -mthreads -D_MT
	mingw32-gcc -c InfoBuffer.c -o ./obj/InfoBuffer.o -mwindows -Wall -Wextra -pedantic -DUNICODE -mthreads -D_MT
	mingw32-gcc ./obj/PromIf.o ./obj/PromTool.o ./obj/InfoBuffer.o -o PromTool.exe -mwindows -Wall -Wextra -pedantic -DUNICODE -mthreads -D_MT

