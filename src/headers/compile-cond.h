
#ifndef H_CCOND
#define H_CCOND

#include "typedefs.h"

void cc_parse(Parser* p);

struct CCLoop {
    Array* items;
    Chunk* start;
    Idf* idf1;
    Idf* idf2;
    Idf* idf3;
    Idf* idf4;
    int idf_type;
    int length;
    int index;
};

struct CCObjectProp {
    ClassProp* prop;
    Decl* on;
};

#endif