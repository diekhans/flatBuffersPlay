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

static flatbuffers::Offset<Psl> pslToFlatb(flatbuffers::FlatBufferBuilder& builder,
                                           const struct psl *psl) {
    return CreatePsl(builder,
                     psl->match,
                     psl->misMatch,
                     psl->repMatch,
                     psl->nCount,
                     psl->qNumInsert,
                     psl->qBaseInsert,
                     psl->tNumInsert,
                     psl->tBaseInsert,
                     builder.CreateString(psl->strand),
                     builder.CreateString(psl->qName),
                     psl->qSize,
                     psl->qStart,
                     psl->qEnd,
                     builder.CreateString(psl->tName),
                     psl->tSize,
                     psl->tStart,
                     psl->tEnd,
                     psl->blockCount,
                     builder.CreateVector(psl->blockSizes, psl->blockCount),
                     builder.CreateVector(psl->qStarts, psl->blockCount),
                     builder.CreateVector(psl->tStarts, psl->blockCount));
}

static void serializePsl(flatbuffers::FlatBufferBuilder& builder,
                         const struct psl *psl,
                         ostream &outFh) {
    builder.Clear();
    auto root = pslToFlatb(builder, psl);
    builder.FinishSizePrefixed(root);

    auto buf = builder.GetBufferPointer();
    outFh.write(reinterpret_cast<const char*>(buf), builder.GetSize());
}


static void serializePsls(struct psl *inPsls,
                          ostream &outFh) {
    flatbuffers::FlatBufferBuilder builder;
    for (struct psl *psl = inPsls; psl != NULL; psl = psl->next) {
        serializePsl(builder, psl, outFh);
    }
}
                       


int main(int argc, const char* argv[]) {
    if (argc != 3) {
        cerr << "Wrong # args: " << argv[0] << " textPslFile flatbPslFile" <<endl;
        exit(1);
    }
    string textPslFile = string(argv[1]);
    string flatbPslFile = string(argv[2]);
    struct psl *inPsls = pslLoadAll(const_cast<char*>(textPslFile.c_str()));
    ofstream outFh;
    outFh.open(flatbPslFile, std::ios::binary | std::ios::out);
    serializePsls(inPsls, outFh);
    outFh.close();
}
