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
        OneToOne(std::function<int(int,MPI_Comm)> a, bool pos): Operation(pos), func(a) {}
        int operator() (int a, MPI_Comm b) {return func(a, b);} 
    private:
        std::function<int(int,MPI_Comm)> func;
};

class OneToAll : public Operation
{
    public:
        OneToAll(std::function<int(int, MPI_Comm)> a, bool pos): Operation(pos), func(a) {}
        int operator() (int a, MPI_Comm b) {return func(a, b);}
    private:
        std::function<int(int, MPI_Comm)> func;
};

class AllToOne : public Operation
{
    public:
        AllToOne(std::function<int(int, MPI_Comm)> a, bool pos): Operation(pos), func(a) {}
        int operator() (int a, MPI_Comm b) {return func(a, b);}
    private:
        std::function<int(int, MPI_Comm)> func;
};

class AllToAll : public Operation
{
    public:
        AllToAll(std::function<int(MPI_Comm)> a, bool pos, std::pair<AllToOne, OneToAll> decomp): Operation(pos), func(a), decomposed(decomp) {}
        int operator() (MPI_Comm a) {return func(a);}
        std::pair<AllToOne, OneToAll> decomp(MPI_Comm a, int root) {return decomposed;}
    private:
        std::function<int(MPI_Comm)> func;
        std::pair<AllToOne, OneToAll> decomposed;
};

class FileOp : public Operation
{
    public:
        FileOp(std::function<int(MPI_File)> a, bool pos): Operation(pos), func(a) {}
        int operator() (MPI_File a) {return func(a);}
    private:
        std::function<int(MPI_File)> func;
};

class WinOp : public Operation
{
    public:
        WinOp(std::function<int(int, MPI_Win)> a, bool pos): Operation(pos), func(a) {}
        int operator() (int a, MPI_Win b) {return func(a, b);}
    private:
        std::function<int(int, MPI_Win)> func;
};

class WinOpColl : public Operation
{
    public:
        WinOpColl(std::function<int(MPI_Win)> a, bool pos): Operation(pos), func(a) {}
        int operator() (MPI_Win a) {return func(a);}
    private:
        std::function<int(MPI_Win)> func;
};

#endif