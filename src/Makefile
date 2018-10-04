BINDIR = ../bin
OBJDIR = ../objs

PROGS = ${BINDIR}/pslSerialize ${BINDIR}/pslDeserialize
FLATBUF_CPP = psl_generated.h


MYSQL_LIBS = $(shell mysql_config --libs)
KENT_SRC = ${HOME}/kent/src
KENT_INCLS = -I${KENT_SRC}/inc
KENT_LIBS = ${KENT_SRC}/lib/x86_64/jkhgap.a ${KENT_SRC}/lib/x86_64/jkweb.a ${MYSQL_LIBS} -lssl -lcrypto ${KENT_SRC}/htslib/libhts.a -lz -lm


CPPFLAGS = -I/usr/local/include ${KENT_INCLS}

SRCS = pslSerialize.cc pslDeserialize.cc
pslSerialize_OBJS =  ${OBJDIR}/pslSerialize.o
pslDeserialize_OBJS =  ${OBJDIR}/pslDeserialize.o
DEPENDS =  ${SRCS:%.cc=%.depend}

CXXFLAGS = ${CPPFLAGS} -g -Wall -Wno-sign-compare

all: ${PROGS}


${BINDIR}/%: ${OBJDIR}/%.o
	@mkdir -p $(dir $@)
	${CXX} ${CXXFLAGS} -o $@ ${${*}_OBJS} ${KENT_LIBS} ${LIBS}


# dependency file is generate as part of compile
${OBJDIR}/%.o: %.cc ${FLATBUF_CPP}
	@mkdir -p $(dir $@)
	${CXX} ${CXXFLAGS} -c -MM -MT $@ $< >$*.depend
	${CXX} ${CXXFLAGS} -c -o $@ $<


%_generated.h: %.fbs
	flatc -c psl.fbs 

clean:
	rm -fr ${FLATBUF_CPP} ${OBJS} ${PROGS} ${PROGS:%=%.dSYM} ${DEPENDS}
	rm -rf ${BINDIR} ${OBJDIR}



# don't fail on missing dependencies, they are first time the .o is generates
-include ${DEPENDS}


# Local Variables:
# mode: makefile-gmake
# End:
