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
        AllToAll(std::function<int(MPI_Comm)> a, bool pos): Operation(pos), func(a) {}
        int operator() (MPI_Comm a) {return func(a);}
    private:
        std::function<int(MPI_Comm)> func;
};

class FileOp : public Operation
{
    public:
        FileOp(std::function<int(MPI_File, MPI_Comm)> a, bool pos): Operation(pos), func(a) {}
        int operator() (MPI_File a, MPI_Comm b) {return func(a, b);}
    private:
        std::function<int(MPI_File, MPI_Comm)> func;
};

class FileOpColl : public Operation
{
    public:
        FileOpColl(std::function<int(MPI_File, MPI_Comm)> a, bool pos): Operation(pos), func(a) {}
        int operator() (MPI_File a, MPI_Comm b) {return func(a, b);}
    private:
        std::function<int(MPI_File, MPI_Comm)> func;
};

class WinOp : public Operation
{
    public:
        WinOp(std::function<int(int, MPI_Win, MPI_Comm)> a, bool pos): Operation(pos), func(a) {}
        int operator() (int a, MPI_Win b, MPI_Comm c) {return func(a, b, c);}
    private:
        std::function<int(int, MPI_Win, MPI_Comm)> func;
};

class WinOpColl : public Operation
{
    public:
        WinOpColl(std::function<int(MPI_Win, MPI_Comm)> a, bool pos): Operation(pos), func(a) {}
        int operator() (MPI_Win a, MPI_Comm b) {return func(a, b);}
    private:
        std::function<int(MPI_Win, MPI_Comm)> func;
};

#endif