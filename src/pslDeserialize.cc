#include <iostream>
#include <fstream>
#include <string>
#include "psl_generated.h"
extern "C" {
#define new newxxx
#include "psl.h"
#undef new
}

using namespace std;
using namespace Kent;

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

static bool deserializePsl(istream &inFh,
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


static void deserializePsls(const string& flatbPslFile,
                            FILE *outFh) {
    ifstream inFh;
    inFh.open(flatbPslFile, std::ios::binary | std::ios::in);
    while (deserializePsl(inFh, outFh)) {
        continue;
    }
    inFh.close();
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
        deserializePsls(flatbPslFile, outFh);
    } else {
        cerr << "Error: invalid mode " << mode << ", expected one of 'stream'" <<endl;
        exit(1);
    }
    carefulClose(&outFh);
}

