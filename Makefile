all:
	cd src && ${MAKE} all

test: all
	cd tests && ${MAKE} test

clean:
	cd src && ${MAKE} clean
	cd tests && ${MAKE} clean
