#include "intercomm_utils.hpp"
#include <string.h>
#include <algorithm>
#include <vector>
#include "config.hpp"
#include "log.hpp"
#include "mpi.h"

using namespace legio;

/*
int get_root_level(int own, int max_level)
{
    int trail = 1;
    while (!(own % (1 << trail)) && trail - 1 < max_level)
        trail++;
    return trail - 1;
}
*/

// Search Bit Twiddling Hacks for an explanation
// Number of trailing zeros
int legio::get_root_level(int own, int max_level)
{
    uint32_t v = static_cast<unsigned>(own);
    static const int Mod37BitPosition[] = {32, 0, 1,  26, 2,  23, 27, 0,  3,  16, 24, 30, 28,
                                           11, 0, 13, 4,  7,  17, 0,  25, 22, 31, 15, 29, 10,
                                           12, 6, 0,  21, 14, 9,  5,  20, 8,  19, 18};
    return (Mod37BitPosition[(-v & v) % 37] > max_level ? max_level
                                                        : Mod37BitPosition[(-v & v) % 37]);
}

void legio::get_range(int prefix, int level, int size, int* low, int* high)
{
    unsigned u_prefix = static_cast<unsigned>(prefix);
    unsigned all_one = 0 - 1;
    u_prefix &= (all_one << level);
    unsigned one = 1;
    *low = static_cast<int>(u_prefix);
    *high = static_cast<int>(u_prefix | ((one << (level)) - 1));
    if (*high >= size)
        *high = size - 1;
}

MPI_Group legio::deeper_check_tree(MPI_Group to_check, MPI_Comm actual_comm)
{
    MPI_Group actual;
    int check_rank, translated, size;
    MPI_Comm_group(actual_comm, &actual);
    MPI_Group_rank(to_check, &check_rank);
    MPI_Group_size(to_check, &size);
    int* ranks_list = static_cast<int*>(malloc(sizeof(int) * size));
    memset(ranks_list, -1, size * sizeof(int));
    ranks_list[check_rank] = check_rank;
    int max_level = 0;
    while (size > (1 << max_level))
        max_level++;
    // printf(">>>>%d<<<< Inside deeper check, max_level = %d, size = %d\n", check_rank, max_level,
    // size);
    int effective_rank = check_rank;
    for (int level = 1; level <= max_level; level++)
    {
        int root_level = get_root_level(effective_rank, max_level);
        if (root_level < level)
        {
            // printf(">>>>%d<<<< Level %d, I am no root\n", check_rank, level);
            // We're leaf, gotta send info and then wait for response
            int low, high;
            int low_own, high_own;
            get_range(effective_rank, level, size, &low, &high);
            get_range(effective_rank, level - 1, size, &low_own, &high_own);
            int target_rank;
            int used_root;
            for (used_root = low; used_root <= high; used_root++)
            {
                if (used_root == effective_rank)
                    break;
                else
                {
                    // printf(">>>>%d<<<< Level %d, checking %d as root\n", check_rank, level,
                    // used_root);
                    MPI_Group_translate_ranks(to_check, 1, &used_root, actual, &target_rank);
                    PMPI_Send(&(ranks_list[low_own]), high_own - low_own + 1, MPI_INT, target_rank,
                              0, actual_comm);
                    int rc = PMPI_Recv(ranks_list, size, MPI_INT, target_rank, 0, actual_comm,
                                       MPI_STATUSES_IGNORE);
                    if (rc == MPI_SUCCESS)
                    {
                        // printf(">>>>%d<<<< Level %d, Found %d as root\n", check_rank, level,
                        // used_root);
                        break;  // All done, just need to propagate to top
                    }
                }
            }
            if (used_root == effective_rank)
            {
                // printf(">>>>%d<<<< Level %d, No roots found\n", check_rank, level);
                // No roots found, gotta become the new root of the subtree
                effective_rank = low;
                level--;
                continue;  // This way we repeat the iteration with a new effective
            }
            else
                break;
        }
        else
        {
            // printf(">>>>%d<<<< Level %d, I am root\n", check_rank, level);
            int receiver, target_rank;
            int low, high;
            get_range(effective_rank + (1 << level - 1), level - 1, size, &low, &high);
            for (receiver = low; receiver <= high; receiver++)
            {
                if (receiver == check_rank)
                    break;
                // printf(">>>>%d<<<< Level %d, checking %d as leaf\n", check_rank, level,
                // receiver);
                MPI_Group_translate_ranks(to_check, 1, &receiver, actual, &target_rank);
                int rc = PMPI_Recv(&(ranks_list[low]), high - low + 1, MPI_INT, target_rank, 0,
                                   actual_comm, MPI_STATUS_IGNORE);
                if (rc == MPI_SUCCESS)
                {
                    // printf(">>>>%d<<<< Level %d, found %d as leaf\n", check_rank, level,
                    // receiver);
                    break;
                }
            }
        }
    }
    // printf("]]]]%d[[[[ Starting up propagation, size = %d, values are :", check_rank, size);
    // for(int i = 0; i < size; i++)
    // printf("%d ", ranks_list[i]);
    // printf("\n");
    int level_root = get_root_level(effective_rank, max_level);
    int level_check = get_root_level(check_rank, max_level);
    int low, high;
    for (int level = level_root; level >= 1; level--)
    {
        if (level == level_check)
            effective_rank = check_rank;
        get_range(effective_rank + (1 << level - 1), level - 1, size, &low, &high);
        // printf("]]]]%d[[[[ Searching at Level %d\n", check_rank, level);
        int root_searched, target;
        for (root_searched = low; root_searched <= high; root_searched++)
            if (ranks_list[root_searched] != -1)
                break;
        if (root_searched > high || root_searched == check_rank)
            continue;
        // printf("]]]]%d[[[[ Searching at Level %d, found leaf %d\n", check_rank, level,
        // root_searched);
        MPI_Group_translate_ranks(to_check, 1, &root_searched, actual, &target);
        PMPI_Send(ranks_list, size, MPI_INT, target, 0, actual_comm);
    }
    int* compacted = static_cast<int*>(malloc(sizeof(int) * size));
    int position = 0;
    for (int i = 0; i < size; i++)
        if (ranks_list[i] != -1)
            compacted[position++] = ranks_list[i];
    free(ranks_list);
    MPI_Group result;
    MPI_Group_incl(to_check, position, compacted, &result);
    free(compacted);
    return result;
}

// Search Bit Twiddling Hacks for an explanation
unsigned legio::next_pow_2(int number)
{
    uint32_t v = static_cast<unsigned>(number);
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

MPI_Group legio::deeper_check_cube(MPI_Group to_check, MPI_Comm actual_comm)
{
    char errstr[MPI_MAX_ERROR_STRING];
    int len;
    MPI_Group actual;
    int check_rank, translated, size, own_rank;
    MPI_Comm_group(actual_comm, &actual);
    MPI_Group_rank(to_check, &check_rank);
    MPI_Group_size(to_check, &size);
    PMPI_Comm_rank(actual_comm, &own_rank);

    unsigned u_rank = static_cast<unsigned>(check_rank);
    unsigned next_power = next_pow_2(size);
    int* ranks_list = static_cast<int*>(malloc(sizeof(int) * next_power));
    memset(ranks_list, -1, next_power * sizeof(int));
    ranks_list[check_rank] = check_rank;

    std::vector<unsigned> roles;
    std::vector<unsigned> future_roles;
    roles.push_back(u_rank);

    int level = 0;
    for (unsigned i = 1; i < next_power; i <<= 1, level += 1)
    {
        std::sort(roles.begin(), roles.end());
        for (unsigned role : roles)
        {
            unsigned target = role xor i;
            int target_index = static_cast<int>(target);
            int target_rank;
            if (target_index >= size)
                target_rank = MPI_UNDEFINED;
            else
                PMPI_Group_translate_ranks(to_check, 1, &target_index, actual, &target_rank);
            int low_index, high_index, rc;
            if (target_rank == own_rank)
                continue;
            if (target < role)
            {
                printf("Rank %d, communicating with %u, role %u, rank %d\n", check_rank, target,
                       role, target_rank);
                get_range(static_cast<int>(role), level, next_power, &low_index, &high_index);
                PMPI_Send(&(ranks_list[low_index]), high_index - low_index + 1, MPI_INT,
                          target_rank,
                          (target < role ? target + next_power : role + next_power) >> level,
                          actual_comm);
                get_range(static_cast<int>(target), level, next_power, &low_index, &high_index);
                rc = PMPI_Recv(&(ranks_list[low_index]), high_index - low_index + 1, MPI_INT,
                               target_rank,
                               (target < role ? target + next_power : role + next_power) >> level,
                               actual_comm, MPI_STATUS_IGNORE);
            }
            else
            {
                printf("Rank %d, communicating with %u, role %u, rank %d\n", check_rank, target,
                       role, target_rank);
                get_range(static_cast<unsigned>(target), level, next_power, &low_index,
                          &high_index);
                rc = PMPI_Recv(&(ranks_list[low_index]), high_index - low_index + 1, MPI_INT,
                               target_rank,
                               (target < role ? target + next_power : role + next_power) >> level,
                               actual_comm, MPI_STATUS_IGNORE);
                get_range(static_cast<int>(role), level, next_power, &low_index, &high_index);
                PMPI_Send(&(ranks_list[low_index]), high_index - low_index + 1, MPI_INT,
                          target_rank,
                          (target < role ? target + next_power : role + next_power) >> level,
                          actual_comm);
            }
            if (rc != MPI_SUCCESS)
            {
                printf("Rank %d, ERROR with %u, role %u\n", check_rank, target, role);
                bool found = std::find(roles.begin(), roles.end(), static_cast<unsigned>(target)) !=
                             roles.end();
                for (unsigned j = 1; j < i && !found; j <<= 1)
                {
                    unsigned adjusted_target = target xor j;
                    int adjusted_target_index = static_cast<int>(adjusted_target);
                    if (adjusted_target_index >= size)
                        target_rank = MPI_UNDEFINED;
                    else
                        PMPI_Group_translate_ranks(to_check, 1, &adjusted_target_index, actual,
                                                   &target_rank);
                    if (target_rank == own_rank)
                        continue;
                    if (adjusted_target < role)
                    {
                        printf("Rank %d, ERROR with %u, adjusting to %u, role %u\n", check_rank,
                               target, adjusted_target, role);
                        get_range(static_cast<int>(role), level, next_power, &low_index,
                                  &high_index);
                        PMPI_Send(
                            &(ranks_list[low_index]), high_index - low_index + 1, MPI_INT,
                            target_rank,
                            (target < role ? target + next_power : role + next_power) >> level,
                            actual_comm);
                        get_range(static_cast<int>(target), level, next_power, &low_index,
                                  &high_index);
                        rc = PMPI_Recv(
                            &(ranks_list[low_index]), high_index - low_index + 1, MPI_INT,
                            target_rank,
                            (target < role ? target + next_power : role + next_power) >> level,
                            actual_comm, MPI_STATUS_IGNORE);
                    }
                    else
                    {
                        printf("Rank %d, ERROR with %u, adjusting to %u, role %u\n", check_rank,
                               target, adjusted_target, role);
                        get_range(static_cast<int>(target), level, next_power, &low_index,
                                  &high_index);
                        rc = PMPI_Recv(
                            &(ranks_list[low_index]), high_index - low_index + 1, MPI_INT,
                            target_rank,
                            (target < role ? target + next_power : role + next_power) >> level,
                            actual_comm, MPI_STATUS_IGNORE);
                        get_range(static_cast<int>(role), level, next_power, &low_index,
                                  &high_index);
                        PMPI_Send(
                            &(ranks_list[low_index]), high_index - low_index + 1, MPI_INT,
                            target_rank,
                            (target < role ? target + next_power : role + next_power) >> level,
                            actual_comm);
                    }
                    if (rc == MPI_SUCCESS)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    printf("Rank %d, role %u, add role %u\n", check_rank, role, target);
                    future_roles.push_back(target);
                }
            }
        }
        while (!future_roles.empty())
        {
            roles.push_back(future_roles[future_roles.size() - 1]);
            future_roles.pop_back();
        }
    }
    int* compacted = static_cast<int*>(malloc(sizeof(int) * size));
    int position = 0;
    printf("Rank %d, result:", check_rank);
    for (int i = 0; i < size; i++)
    {
        printf(" %d", ranks_list[i]);
        if (ranks_list[i] != -1)
            compacted[position++] = ranks_list[i];
    }
    printf("\n");
    free(ranks_list);
    MPI_Group result;
    MPI_Group_incl(to_check, position, compacted, &result);
    free(compacted);
    return result;
}