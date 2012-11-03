all: qmk keymap app

qmk: seven-square.pro
	qmake -o Makefile.qmake seven-square.pro

keymap: keycodes.h
	./generate-keymap.sh

app:
	make -f Makefile.qmake

clean:
	make -f Makefile.qmake clean
	#rm -f Makefile.qmake
	#rm -f keymap-generated.h
