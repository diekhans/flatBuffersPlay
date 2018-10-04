#include <iostream>
#include <fstream>
#include <string>
#include <errno.h>
#include <sys/mman.h>
#include "psl_generated.h"
extern "C" {
#define new newxxx
#include "psl.h"
#undef new
}

using namespace std;
using namespace Kent;

static void errnoExit(const string& msg) {
    cerr << msg << ": " << ::strerror(errno) << endl;
    exit(1);
}

static struct psl* flatbToPsl(const Psl *pslfb) {
    uint32_t blockCnt = pslfb->blockCount();
    struct psl* psl = pslNew(const_cast<char*>(pslfb->qName()->c_str()),
                             pslfb->qSize(), pslfb->qStart(), pslfb->qEnd(),
                             const_cast<char*>(pslfb->tName()->c_str()),
                             pslfb->tSize(), pslfb->tStart(), pslfb->tEnd(),
                             const_cast<char*>(pslfb->strand()->c_str()),
                             blockCnt, 0);
    psl->match = pslfb->match();
    psl->misMatch = pslfb->misMatch();
    psl->repMatch = pslfb->repMatch();
    psl->nCount = pslfb->nCount();
    psl->qNumInsert = pslfb->qNumInsert();
    psl->qBaseInsert = pslfb->qBaseInsert();
    psl->tNumInsert = pslfb->tNumInsert();
    psl->tBaseInsert = pslfb->tBaseInsert();
    for (unsigned i = 0; i < pslfb->blockCount(); i++) {
        psl->blockSizes[i] = (*pslfb->blockSizes())[i];
        psl->qStarts[i] = (*pslfb->qStarts())[i];
        psl->tStarts[i] = (*pslfb->tStarts())[i];
        psl->blockCount++;
    }
    return psl;
}

static bool deserializePslStream(istream &inFh,
                                 FILE *outFh) {
    uint32_t size;
    inFh.read(reinterpret_cast<char*>(&size), sizeof(size));
    if (!inFh) {
        return false;
    }
    char buf[size];
    inFh.read(reinterpret_cast<char*>(&buf), size);
    if (!inFh) {
        cerr << "Error: premature EOF" << endl;
        exit(1);
    }
    const Psl *pslfb = GetPsl(buf);
    struct psl* psl = flatbToPsl(pslfb);
    pslTabOut(psl, outFh);
    pslFree(&psl);
    return true;
}


static void deserializePslsStream(const string& flatbPslFile,
                                  FILE *outFh) {
    ifstream inFh;
    inFh.open(flatbPslFile, std::ios::binary | std::ios::in);
    while (deserializePslStream(inFh, outFh)) {
        continue;
    }
    inFh.close();
}

static inline const void* ptrAdd(const void* ptr, size_t amt) {
    return reinterpret_cast<const unsigned char*>(ptr) + amt;
}

static void deserializePslMmap(const void* buf,
                               FILE *outFh) {
    const Psl *pslfb = GetPsl(buf);
    struct psl* psl = flatbToPsl(pslfb);
    pslTabOut(psl, outFh);
    pslFree(&psl);
}

static void deserializePslsMmap(const string& flatbPslFile,
                                FILE *outFh) {
    int fd = ::open(flatbPslFile.c_str(), O_RDONLY);
    if (fd < 0) {
        errnoExit("Can't open flatbuffers file: " + flatbPslFile);
    }
    struct stat fileStat;
    if (::fstat(fd, &fileStat) < 0) {
        errnoExit("stat() failed on: " + flatbPslFile);
    }
    const void* buf = mmap(0, fileStat.st_size, PROT_READ, MAP_SHARED|MAP_FILE, fd, 0);
    if (buf == MAP_FAILED) {
        errnoExit("mmap() failed on: " + flatbPslFile);
    }
    size_t offset = 0;
    while (offset < fileStat.st_size) {
        uint32_t recSize = *reinterpret_cast<const uint32_t*>(ptrAdd(buf, offset));
        deserializePslMmap(ptrAdd(buf, offset + sizeof(uint32_t)), outFh);
        offset += sizeof(uint32_t) + recSize;
    }
    ::munmap(const_cast<void*>(buf), fileStat.st_size);
    ::close(fd);
}

int main(int argc, const char* argv[]) {
    if (argc != 4) {
        cerr << "Wrong # args: " << argv[0] << " mode flatbPslFile textPslFile" <<endl;
        exit(1);
    }
    string mode = string(argv[1]);
    string flatbPslFile = string(argv[2]);
    string textPslFile = string(argv[3]);
    FILE *outFh = mustOpen(const_cast<char*>(textPslFile.c_str()), const_cast<char*>("w"));
    if (mode == "stream") {
        deserializePslsStream(flatbPslFile, outFh);
    } else if (mode == "mmap") {
        deserializePslsMmap(flatbPslFile, outFh);
    } else {
        cerr << "Error: invalid mode " << mode << ", expected one of 'stream', or 'mmap'" <<endl;
        exit(1);
    }
    carefulClose(&outFh);
}

