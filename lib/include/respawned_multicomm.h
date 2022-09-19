#ifndef RESPAWNED_MULTICOMM_H
#define RESPAWNED_MULTICOMM_H

#include "mpi.h"
#include "complex_comm.h"
#include "multicomm.h"
#include <list>
#include <set>
#include <unordered_map>
#include <functional>

class RespawnedMulticomm : public Multicomm
{
    public:
        void translate_ranks(int, ComplexComm*, int*);
        std::map<int, RespawnedSupportedComm> supported_comms;
};

#endif