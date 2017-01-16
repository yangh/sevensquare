all: qmk keymap app

qmk: seven-square.pro
	mkdir -p build
	(cd build && qmake-qt4 -o Makefile ../seven-square.pro)

keymap: src/keycodes.h
	./contributes/generate-keymap.sh

app:
	(cd build && make)

install:
	cp -vf build/seven-square /usr/bin/

clean:
	(cd build && make clean)
	#rm -f Makefile.qmake
	#rm -f keymap-generated.h
