#include <mpi.h>  
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <restart.h>
#include <chrono>

#define TOSSNUM 1200000000
struct checkpoint
{
   long toss;
   long numberincircle;
};

long Toss (long numProcessTosses, int myRank);
long TossRestart (long processTosses, int myRank, long toss, long numberInCircle);
void writeToFile(FILE *file, long toss, long numberInCircle);
void readFromFile(int myRank, struct checkpoint *cp);

int main(int argc, char** argv) {
   int myRank, numProcs;
   long totalNumTosses, numProcessTosses, processNumberInCircle, totalNumberInCircle;
   long totalPerfTosses;
   double start, finish, loc_elapsed, elapsed, piEstimate;
   double PI25DT = 3.141592653589793238462643;         /* 25-digit-PI*/
   
   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
   MPI_Comm_rank(MPI_COMM_WORLD, &myRank);

   if (myRank == 1) 
      start = MPI_Wtime();
   
   totalNumTosses = TOSSNUM;
   numProcessTosses = totalNumTosses/numProcs;
   
   if (is_respawned()) {
      struct checkpoint *cp = new checkpoint;
      readFromFile(myRank, cp);
      processNumberInCircle = TossRestart(numProcessTosses, myRank, cp->toss, cp->numberincircle);
   }
   else {
      MPI_Barrier(MPI_COMM_WORLD);
      processNumberInCircle = Toss(numProcessTosses, myRank);
   }
   
   MPI_Reduce(&processNumberInCircle, &totalNumberInCircle, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

   MPI_Reduce(&numProcessTosses, &totalPerfTosses, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

   if (myRank == 1) {
      finish = MPI_Wtime();
      FILE *file_p = fopen("time-montercarlo-restart.csv", "a");
      fprintf(file_p, "%f\n", finish-start);
      fclose(file_p);
    }
   
   if (myRank == 0) {
	   //piEstimate = (4*totalNumberInCircle)/((double) totalNumTosses);
      piEstimate = (4*totalNumberInCircle)/((double) totalPerfTosses);
	   printf("Pi is approximately %.16f, Error is %.16f\n", piEstimate, fabs(piEstimate - PI25DT));
      if(totalPerfTosses != totalNumTosses)
         printf("Lost some computation: %ld instead of %ld\n", totalPerfTosses, totalNumTosses);
   }

   printf("\n\n For rank %d, Pi is approximately %.16f\n", myRank, (4*processNumberInCircle)/((double) totalNumTosses));
   MPI_Finalize(); 
   return 0;
}  

/* Function implements Monte Carlo version of tossing darts at a board */
long Toss (long processTosses, int myRank){
	long toss, numberInCircle = 0;        
	double x,y;
   char buf[12];
   snprintf(buf, 12, "checkpoint%d", myRank);
   FILE* file = fopen(buf, "w+");
	unsigned int seed = (unsigned) time(NULL);
	srand(seed + myRank);
	for (toss = 0; toss < processTosses; toss++) {
	   x = rand_r(&seed)/(double)RAND_MAX;
	   y = rand_r(&seed)/(double)RAND_MAX;
	   if((x*x+y*y) <= 1.0 ) numberInCircle++;
      if(myRank == 0 && toss == 50000000)
         raise(SIGINT);
      if(myRank == 0 && toss % 1000 == 0) 
         writeToFile(file, toss, numberInCircle);
    }
    return numberInCircle;
}

long TossRestart (long processTosses, int myRank, long toss, long numberInCircle) {
	double x,y;
	unsigned int seed = (unsigned) time(NULL);
	srand(seed + myRank);
	for (; toss < processTosses; toss++) {
	   x = rand_r(&seed)/(double)RAND_MAX;
	   y = rand_r(&seed)/(double)RAND_MAX;
	   if((x*x+y*y) <= 1.0 ) numberInCircle++;
    }
    return numberInCircle;
}



void writeToFile(FILE *file, long toss, long numberInCircle) {
   fseek(file, 0, SEEK_SET);
   struct checkpoint cp = {toss, numberInCircle};
   fwrite(&cp, sizeof(struct checkpoint), 1, file);
}

void readFromFile(int myRank, struct checkpoint *cp) {
   char buf[12];
   snprintf(buf, 12, "checkpoint%d", myRank);
   FILE* file = fopen(buf, "r");
   fread(cp, sizeof(struct checkpoint), 1, file);
}