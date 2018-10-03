BINDIR = ../bin
pslSerialize = ${BINDIR}/pslSerialize
pslDeserialize = ${BINDIR}/pslDeserialize

test: testSerial

testSerial:
	@mkdir -p output
	${pslSerialize} input/test.psl output/$@.flatb
	${pslDeserialize} output/$@.flatb output/$@.psl
	diff input/test.psl output/$@.psl

clean:
	rm -rf output
