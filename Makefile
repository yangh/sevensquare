all: app

build/Makefile: seven-square.pro seven-square.qrc
	mkdir -p build
	(cd build && qmake-qt4 -o Makefile ../seven-square.pro)

src/keymap-generated.h: src/keycodes.h
	./contributes/generate-keymap.sh

app: build/Makefile src/keymap-generated.h
	(cd build && make)

install:
	cp -vf build/seven-square /usr/bin/

clean:
	(cd build && make clean)
