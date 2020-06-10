#ifndef OPERATIONS_H
#define OPERATIONS_H

#include "mpi.h"
#include <functional>
#include "adv_comm.h"


class Operation{
    public:
        Operation(bool pos): positional(pos) {}
        bool isPositional() {return positional;}
    private:
        bool positional;
};

class OneToOne : public Operation
{
    public:
        OneToOne(std::function<int(int,MPI_Comm, AdvComm*)> a, bool pos): Operation(pos), func(a) {}
        int operator() (int a, MPI_Comm b, AdvComm* c) {return func(a, b, c);} 
    private:
        std::function<int(int,MPI_Comm, AdvComm*)> func;
};

class OneToAll : public Operation
{
    public:
        OneToAll(std::function<int(int, MPI_Comm, AdvComm*)> a, bool pos): Operation(pos), func(a) {}
        int operator() (int a, MPI_Comm b, AdvComm* c) {return func(a, b, c);}
    private:
        std::function<int(int, MPI_Comm, AdvComm*)> func;
};

class AllToOne : public Operation
{
    public:
        AllToOne(std::function<int(int, MPI_Comm, AdvComm*)> a, bool pos): Operation(pos), func(a) {}
        int operator() (int a, MPI_Comm b, AdvComm* c) {return func(a, b, c);}
    private:
        std::function<int(int, MPI_Comm, AdvComm*)> func;
};

class AllToAll : public Operation
{
    public:
        AllToAll(std::function<int(MPI_Comm, AdvComm*)> a, bool pos, std::pair<AllToOne, OneToAll> decomp): Operation(pos), func(a), decomposed(decomp) {}
        int operator() (MPI_Comm a, AdvComm* b) {return func(a, b);}
        std::pair<AllToOne, OneToAll> decomp(MPI_Comm a, int root) {return decomposed;}
    private:
        std::function<int(MPI_Comm, AdvComm*)> func;
        std::pair<AllToOne, OneToAll> decomposed;
};

class FileOp : public Operation
{
    public:
        FileOp(std::function<int(MPI_File, AdvComm*)> a, bool pos): Operation(pos), func(a) {}
        int operator() (MPI_File a, AdvComm* b) {return func(a, b);}
    private:
        std::function<int(MPI_File, AdvComm*)> func;
};

class WinOp : public Operation
{
    public:
        WinOp(std::function<int(int, MPI_Win, AdvComm*)> a, bool pos): Operation(pos), func(a) {}
        int operator() (int a, MPI_Win b, AdvComm* c) {return func(a, b, c);}
    private:
        std::function<int(int, MPI_Win, AdvComm*)> func;
};

class WinOpColl : public Operation
{
    public:
        WinOpColl(std::function<int(MPI_Win, AdvComm*)> a, bool pos): Operation(pos), func(a) {}
        int operator() (MPI_Win a, AdvComm* b) {return func(a, b);}
    private:
        std::function<int(MPI_Win, AdvComm*)> func;
};

#endif