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

static void serializePslStream(flatbuffers::FlatBufferBuilder& builder,
                               const struct psl *psl,
                               ostream &outFh) {
    builder.Clear();
    auto root = pslToFlatb(builder, psl);
    builder.FinishSizePrefixed(root);

    auto buf = builder.GetBufferPointer();
    outFh.write(reinterpret_cast<const char*>(buf), builder.GetSize());
}


static void serializePslsStream(struct psl *inPsls,
                                const string& flatbPslFile) {
    ofstream outFh;
    outFh.open(flatbPslFile, std::ios::binary | std::ios::out);
    flatbuffers::FlatBufferBuilder builder;
    for (struct psl *psl = inPsls; psl != NULL; psl = psl->next) {
        serializePslStream(builder, psl, outFh);
    }
    outFh.close();
}
                       

static void serializePslsPile(struct psl *inPsls,
                              const string& flatbPslFile) {
    flatbuffers::FlatBufferBuilder builder;
    vector<flatbuffers::Offset<Psl>> psls;
    
    for (struct psl *psl = inPsls; psl != NULL; psl = psl->next) {
        psls.push_back(pslToFlatb(builder, psl));
    }
    auto pslPile = CreatePslPile(builder, builder.CreateVector(psls));
    builder.Finish(pslPile);
    ofstream outFh;
    outFh.open(flatbPslFile, std::ios::binary | std::ios::out);
    auto buf = builder.GetBufferPointer();
    outFh.write(reinterpret_cast<const char*>(buf), builder.GetSize());
    outFh.close();
}


int main(int argc, const char* argv[]) {
    if (argc != 4) {
        cerr << "Wrong # args: " << argv[0] << " mode textPslFile flatbPslFile" <<endl;
        exit(1);
    }
    string mode = string(argv[1]);
    string textPslFile = string(argv[2]);
    string flatbPslFile = string(argv[3]);
    struct psl *inPsls = pslLoadAll(const_cast<char*>(textPslFile.c_str()));
    if (mode == "stream") {
        serializePslsStream(inPsls, flatbPslFile);
    } else if (mode == "pile") {
        serializePslsPile(inPsls, flatbPslFile);
    } else {
        cerr << "Error: invalid mode " << mode << ", expected one of 'stream', or 'pile'" <<endl;
        exit(1);
    }
}

