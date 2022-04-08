#ifndef MULTICOMM_H
#define MULTICOMM_H

#include <unordered_map>
#include <map>
#include "mpi.h"
#include <functional>
#include "struct_selector.h"

class ComplexComm;

class Multicomm
{
    public:
        int add_comm(MPI_Comm, MPI_Comm, std::function<int(MPI_Comm, MPI_Comm*)>, std::function<int(MPI_Comm, MPI_Comm, MPI_Comm*)> = nullptr, MPI_Comm = MPI_COMM_NULL);
        int add_comm(ComplexComm);
        ComplexComm* translate_into_complex(MPI_Comm);
        void remove(MPI_Comm, std::function<int(MPI_Comm*)>);
        void part_of(MPI_Comm, int*);
        Multicomm();

        template<class MPI_T>
        bool add_structure(ComplexComm* comm, MPI_T elem, std::function<int(MPI_Comm, MPI_T*)> func)
        {
            int id = MPI_Comm_c2f(comm->get_alias());
            auto res = maps[handle_selector<MPI_T>::get()].insert({c2f<MPI_T>(elem), id});
            if(res.second) 
                comm->add_structure(elem, func);
            return res.second;
        }

        void remove_window(MPI_Win*);
        void remove_file(MPI_File*);
        void remove_request(MPI_Request*);

        template<class MPI_T>
        ComplexComm* get_complex_from_structure(MPI_T elem)
        {
            std::unordered_map<int, int>::iterator res = maps[handle_selector<MPI_T>::get()].find(c2f<MPI_T>(elem));
            if(res != maps[handle_selector<MPI_T>::get()].end())
            {
                std::unordered_map<int, int>::iterator res2 = comms_order.find(res->second);
                if(res2 != comms_order.end())
                    return &(comms.find(res2->second)->second);
                else
                {
                    printf("IMPOSSIBLE BEHAVIOUR!!!\n");
                    return NULL;
                }
            }
            else
            {
                printf("STRUCTURE NOT MAPPED\n");
                return NULL;
            }
        }

        void change_comm(ComplexComm*, MPI_Comm);
    private:
        std::map<int, ComplexComm> comms;
        std::array<std::unordered_map<int,int>, 3> maps;
        std::unordered_map<int, int> comms_order;
        int size;
};

#endif