#include "intercomm_utils.h"
#include "mpi.h"
#include "string.h"
#include "stdio.h"

int get_root_level(int own, int max_level)
{
    int trail = 1;
    while(!(own % (1 << trail)) && trail - 1 < max_level)
        trail++;
    return trail-1;
}

void get_range(int prefix, int level, int size, int* low, int* high)
{
    unsigned u_prefix = static_cast<unsigned>(prefix);
    unsigned all_one = 0-1;
    u_prefix &= (all_one << level);
    unsigned one = 1;
    *low = static_cast<int> (u_prefix);
    *high = static_cast<int> (u_prefix | ((one << (level))-1));
    if(*high >= size)
        *high = size - 1;
}

MPI_Group deeper_check(MPI_Group to_check, MPI_Comm actual_comm)
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
    while(size > (1 << max_level))
        max_level++;
    printf(">>>>%d<<<< Inside deeper check, max_level = %d, size = %d\n", check_rank, max_level, size);
    int effective_rank = check_rank;
    for(int level = 1; level <= max_level; level++)
    {
        int root_level = get_root_level(effective_rank, max_level);
        if(root_level < level)
        {
            printf(">>>>%d<<<< Level %d, I am no root\n", check_rank, level);
            //We're leaf, gotta send info and then wait for response
            int low, high;
            int low_own, high_own;
            get_range(effective_rank, level, size, &low, &high);
            get_range(effective_rank, level-1, size, &low_own, &high_own);
            int target_rank;
            int used_root;
            for(used_root = low; used_root <= high; used_root++)
            {
                if(used_root == effective_rank)
                    break;
                else
                {
                    printf(">>>>%d<<<< Level %d, checking %d as root\n", check_rank, level, used_root);
                    MPI_Group_translate_ranks(to_check, 1, &used_root, actual, &target_rank);
                    PMPI_Send(&(ranks_list[low_own]), high_own - low_own + 1, MPI_INT, target_rank, 0, actual_comm);
                    int rc = PMPI_Recv(ranks_list, size, MPI_INT, target_rank, 0, actual_comm, MPI_STATUSES_IGNORE);
                    if(rc == MPI_SUCCESS)
                    {
                        printf(">>>>%d<<<< Level %d, Found %d as root\n", check_rank, level, used_root);
                        break; //All done, just need to propagate to top
                    }
                }
            }
            if(used_root == effective_rank)
            {
                printf(">>>>%d<<<< Level %d, No roots found\n", check_rank, level);
                //No roots found, gotta become the new root of the subtree
                effective_rank = low;
                level--;
                continue; //This way we repeat the iteration with a new effective
            }
            else
                break;
        }
        else
        {
            printf(">>>>%d<<<< Level %d, I am root\n", check_rank, level);
            int receiver, target_rank;
            int low, high;
            get_range(effective_rank + (1<< level-1), level-1, size, &low, &high);
            for(receiver = low; receiver <= high; receiver++)
            {
                if(receiver == check_rank)
                    break;
                printf(">>>>%d<<<< Level %d, checking %d as leaf\n", check_rank, level, receiver);
                MPI_Group_translate_ranks(to_check, 1, &receiver, actual, &target_rank);
                int rc = PMPI_Recv(&(ranks_list[low]), high-low+1, MPI_INT, target_rank, 0, actual_comm, MPI_STATUS_IGNORE);
                if(rc == MPI_SUCCESS)
                {
                    printf(">>>>%d<<<< Level %d, found %d as leaf\n", check_rank, level, receiver);
                    break;
                }
            }
        }
    }
    printf("]]]]%d[[[[ Starting up propagation, size = %d, values are :", check_rank, size);
    for(int i = 0; i < size; i++)
        printf("%d ", ranks_list[i]);
    printf("\n");
    int level_root = get_root_level(effective_rank, max_level);
    int level_check = get_root_level(check_rank, max_level);
    int low, high;
    for(int level = level_root; level >= 1; level--)
    {
        if(level == level_check)
            effective_rank = check_rank;
        get_range(effective_rank + (1<< level-1), level-1, size, &low, &high);
        printf("]]]]%d[[[[ Searching at Level %d\n", check_rank, level);
        int root_searched, target;
        for(root_searched = low; root_searched <= high; root_searched++)
            if(ranks_list[root_searched] != -1)
                break;
        if(root_searched > high || root_searched == check_rank)
            continue;
        printf("]]]]%d[[[[ Searching at Level %d, found leaf %d\n", check_rank, level, root_searched);
        MPI_Group_translate_ranks(to_check, 1, &root_searched, actual, &target);
        PMPI_Send(ranks_list, size, MPI_INT, target, 0, actual_comm);
    }
    int* compacted = static_cast<int*>(malloc(sizeof(int)* size));
    int position = 0;
    for(int i = 0; i < size; i++)
        if(ranks_list[i] != -1)
            compacted[position++] = ranks_list[i];
    free(ranks_list);
    MPI_Group result;
    MPI_Group_incl(to_check, position, compacted, &result);
    free(compacted);
    return result;
}

void check_group(ComplexComm cur_comm, MPI_Group group, MPI_Group *first_clean, MPI_Group *second_clean)
{
    MPI_Group failed_group, original_group = cur_comm.get_group(), actual_group;
    int own_rank;
    MPI_Comm_group(cur_comm.get_comm(), &actual_group);
    MPI_Group_rank(actual_group, &own_rank);
    MPI_Group_difference(original_group, actual_group, &failed_group);
    MPI_Group_difference(group, failed_group, first_clean);
    MPI_Group_free(&failed_group);
    int size;
    MPI_Group_size(*first_clean, &size);
    printf("___%d___ First clean, size: %d\n", own_rank, size);
    //Up to this we removed all the previously detected failures in the communicator
    //Now we need to remove all the failures in the group
    //To do so we use the second algorithm (the DK one)
    *second_clean = deeper_check(*first_clean, cur_comm.get_alias());
    MPI_Group_size(*second_clean, &size);
    printf("___%d___ Second clean, size: %d\n", own_rank, size);
}