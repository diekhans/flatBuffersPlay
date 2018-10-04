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

// allow for multiple roots 
inline const Kent::Psl *GetPsl(const void *buf) {
  return flatbuffers::GetRoot<Kent::Psl>(buf);
}

// allow for multiple roots 
inline const Kent::PslPile *GetPslPile(const void *buf) {
  return flatbuffers::GetRoot<Kent::PslPile>(buf);
}

static void errnoExit(const string& msg) {
    cerr << msg << ": " << ::strerror(errno) << endl;
    exit(1);
}

class MmappedFile {
    private:
    int fd;
    public:
    const string fname;
    size_t size;
    void *fmem;
    MmappedFile(const string& fname):
        fname(fname) {
        fd = ::open(fname.c_str(), O_RDONLY);
        if (fd < 0) {
            errnoExit("Can't open flatbuffers file: " + fname);
        }
        struct stat fileStat;
        if (::fstat(fd, &fileStat) < 0) {
            errnoExit("stat() failed on: " + fname);
        }
        size = fileStat.st_size;
        fmem = mmap(0, fileStat.st_size, PROT_READ, MAP_SHARED|MAP_FILE, fd, 0);
        if (fmem == MAP_FAILED) {
            errnoExit("mmap() failed on: " + fname);
        }
    }

    ~MmappedFile() {
        ::munmap(const_cast<void*>(fmem), size);
        ::close(fd);
    }

};

// step through this to see scalar access (not static to avoid inline)
uint32_t scalarAccess(const Psl *pslfb) {
    uint32_t n = pslfb->blockCount();
    return n;
}


// step through this to see vector access (not static to avoid inline)
uint32_t vectorAccess(const Psl *pslfb,
                      int i) {
    uint32_t n = pslfb->blockCount();
    return n;
}

static struct psl* flatbToPsl(const Psl *pslfb) {
    scalarAccess(pslfb);
    vectorAccess(pslfb, 0);
    
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
        psl->blockSizes[i] = pslfb->blockSizes()->Get(i);
        psl->qStarts[i] = pslfb->qStarts()->Get(i);
        psl->tStarts[i] = pslfb->tStarts()->Get(i);
        psl->blockCount++;
    }
    return psl;
}

static void pslfbTabWrite(const Psl *pslfb,
                          FILE *outFh) {
    struct psl* psl = flatbToPsl(pslfb);
    pslTabOut(psl, outFh);
    pslFree(&psl);
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
    pslfbTabWrite(GetPsl(buf), outFh);
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
    pslfbTabWrite(GetPsl(buf), outFh);
}

static void deserializePslsMmap(const string& flatbPslFile,
                                FILE *outFh) {
    MmappedFile pslsfb(flatbPslFile);
    size_t offset = 0;
    while (offset < pslsfb.size) {
        uint32_t recSize = *reinterpret_cast<const uint32_t*>(ptrAdd(pslsfb.fmem, offset));
        deserializePslMmap(ptrAdd(pslsfb.fmem, offset + sizeof(uint32_t)), outFh);
        offset += sizeof(uint32_t) + recSize;
    }
}

static void deserializePslsPile(const string& flatbPslFile,
                                FILE *outFh) {
    MmappedFile pslsfb(flatbPslFile);
    const PslPile *pslPile = GetPslPile(pslsfb.fmem);
    for (int i = 0; i < pslPile->psls()->size(); i++) {
        pslfbTabWrite(pslPile->psls()->Get(i), outFh);
    }
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
    } else if (mode == "pile") {
        deserializePslsPile(flatbPslFile, outFh);
    } else {
        cerr << "Error: invalid mode " << mode << ", expected one of 'stream', 'mmap', or 'pile'" <<endl;
        exit(1);
    }
    carefulClose(&outFh);
}

