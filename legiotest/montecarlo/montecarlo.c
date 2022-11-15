#include <mpi.h>  
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

#define TOSSNUM 1200000

long Toss (long numProcessTosses, int myRank);

int main(int argc, char** argv) {
   int myRank, numProcs;
   long totalNumTosses, numProcessTosses, processNumberInCircle, totalNumberInCircle;
   long totalPerfTosses;
   double start, finish, loc_elapsed, elapsed, piEstimate;
   double PI25DT = 3.141592653589793238462643;         /* 25-digit-PI*/
   
   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
   MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
  
   
   totalNumTosses = TOSSNUM;
   
   numProcessTosses = totalNumTosses/numProcs; 
   
   MPI_Barrier(MPI_COMM_WORLD);
   start = MPI_Wtime();
   processNumberInCircle = Toss(numProcessTosses, myRank);
   finish = MPI_Wtime();
   loc_elapsed = finish-start;
   MPI_Reduce(&loc_elapsed, &elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD); 
 
   MPI_Reduce(&processNumberInCircle, &totalNumberInCircle, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

   MPI_Reduce(&numProcessTosses, &totalPerfTosses, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
   
   if (myRank == 0) {
	   //piEstimate = (4*totalNumberInCircle)/((double) totalNumTosses);
       piEstimate = (4*totalNumberInCircle)/((double) totalPerfTosses);
	   printf("Elapsed time = %f seconds \n", elapsed);
	   printf("Pi is approximately %.16f, Error is %.16f\n", piEstimate, fabs(piEstimate - PI25DT));
       if(totalPerfTosses != totalNumTosses)
         printf("Lost some computation: %ld instead of %ld\n", totalPerfTosses, totalNumTosses);
   }
   MPI_Finalize(); 
   return 0;
}  

/* Function implements Monte Carlo version of tossing darts at a board */
long Toss (long processTosses, int myRank){
	long toss, numberInCircle = 0;        
	double x,y;
	unsigned int seed = (unsigned) time(NULL);
	srand(seed + myRank);
	for (toss = 0; toss < processTosses; toss++) {
	   x = rand_r(&seed)/(double)RAND_MAX;
	   y = rand_r(&seed)/(double)RAND_MAX;
	   if((x*x+y*y) <= 1.0 ) numberInCircle++;
       if(myRank == 2 && toss == 20)
        raise(SIGINT);
    }
    return numberInCircle;
}