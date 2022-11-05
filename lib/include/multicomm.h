#ifndef MULTICOMM_H
#define MULTICOMM_H

#include <unordered_map>
#include <map>
#include "mpi.h"
#include <functional>
#include <set>
#include "struct_selector.h"
#include "supported_comm.h"
#include <stdio.h>

class ComplexComm;

class Multicomm
{
    public:
        int add_comm(MPI_Comm);
        int add_comm(ComplexComm);
        ComplexComm* translate_into_complex(MPI_Comm);
        void remove(MPI_Comm, std::function<int(MPI_Comm*)>);
        void part_of(MPI_Comm, int*);
        Multicomm(int size);
        Multicomm() = default;

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
                //std::unordered_map<int, int>::iterator res2 = comms_order.find(res->second);
                auto res2 = comms.find(res->second);
                if(res2 != comms.end())
                    return &(res2->second);
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
        ComplexComm* get_comm_by_c2f(int c2f) {
            return &comms.find(c2f)->second;
        }

        void change_comm(ComplexComm*, MPI_Comm);
        bool respawned = false;
        std::vector<int> to_respawn;
        std::map<int, int> supported_comms;
        std::vector<SupportedComm> supported_comms_vector;
        std::vector<Rank> get_ranks() {
            return ranks;
        }
        void set_failed_rank(int world_rank) {
            ranks.at(world_rank).failed = true;
            for (auto &supported_comm : supported_comms) {
                supported_comms_vector[supported_comm.second].set_failed(world_rank);
            }
        }
        virtual void translate_ranks(int, ComplexComm*, int*);

        // Shift a translated rank considering the failed ranks inside the world
        int untranslate_world_rank(int translated) {
            // 1 2 3 4
            // 1 x 3 4
            // 2 = translated -> needs to become three
            int i = 0, source = translated;
            while (i <= translated) {
                if (ranks.at(i).failed)
                    source++;
                i++;
            }
            
            return source;
        }
    protected:
        std::vector<Rank> ranks;
    private:
        std::unordered_map<int, ComplexComm> comms;
        std::array<std::unordered_map<int,int>, 3> maps;
};

#endif