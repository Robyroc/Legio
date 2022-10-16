#ifndef RESPAWN_MULTICOMM_H
#define RESPAWN_MULTICOMM_H

#include "mpi.h"
#include "complex_comm.h"
#include "multicomm.h"
#include <list>
#include <vector>
#include <unordered_map>
#include <functional>

class RespawnMulticomm : public Multicomm
{
    public:
        RespawnMulticomm(int size, std::vector<int> failed);
        std::map<int, RespawnedSupportedComm> supported_comms;
        virtual void translate_ranks(int, ComplexComm*, int*);
};

#endif