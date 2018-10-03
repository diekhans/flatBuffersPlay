BINDIR = ../bin
pslSerialize = ${BINDIR}/pslSerialize
pslDeserialize = ${BINDIR}/pslDeserialize

test: testStream

testStream:
	@mkdir -p output
	${pslSerialize} stream input/test.psl output/$@.flatb
	${pslDeserialize} stream output/$@.flatb output/$@.psl
	diff input/test.psl output/$@.psl

clean:
	rm -rf output
