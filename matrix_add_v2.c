/* A simple MPI example program that uses MPI_Scatter.                  */

/* The program should be run with an even number of processes.          */
/* Process zero initializes arrays A and B of 48 integers and distributes */
/* the arrays evenly among all processes using MPI_Scatter. Each process */
/* receives 12 elements from each array, adds them, and sends results back. */
/* Process 0 receives messages containing hostnames and the sum results, */
/* and prints out the received messages                                 */

/* Compile the program with 'mpicc mulscatter.c -o mulscatter'          */
/* Run the program with 'mpirun -machinefile hostfile -np 4 mulscatter' */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#define MAXPROC 8    /* Max number of procsses */
#define NAMELEN 80   /* Max length of machine name */
#define LENGTH 48    /* Length of arrays A and B - 48 elements each */

main(int argc, char* argv[]) {
  int i, j, np, me;
  const int nametag  = 42;    /* Tag value for sending name */
  const int datatag  = 43;    /* Tag value for sending data */
  const int root = 0;         /* Root process in scatter */
  MPI_Status status;          /* Status object for receive */

  char myname[NAMELEN];             /* Local host name string */
  char hostname[MAXPROC][NAMELEN];  /* Received host names */

  int A[LENGTH];            /* First array to distribute */
  int B[LENGTH];            /* Second array to distribute */
  int localA[LENGTH];       /* Local portion of A */
  int localB[LENGTH];       /* Local portion of B */
  int localSum[LENGTH];     /* Local sum of A and B portions */

  MPI_Init(&argc, &argv);                /* Initialize MPI */
  MPI_Comm_size(MPI_COMM_WORLD, &np);    /* Get nr of processes */
  MPI_Comm_rank(MPI_COMM_WORLD, &me);    /* Get own identifier */
  
  gethostname(myname, NAMELEN);    /* Get host name */

  if (me == 0) {    /* Process 0 does this */
    
    /* Initialize the array A with values 0 .. LENGTH-1 */
    for (i=0; i<LENGTH; i++) {
      A[i] = i;
    }
    
    /* Initialize the array B with values LENGTH..2*LENGTH-1 */
    for (i=0; i<LENGTH; i++) {
      B[i] = LENGTH + i;
    }

    /* Check that we have valid number of processes */
    if (np>MAXPROC || LENGTH % np != 0) {
      printf("You need to use a number of processes that divides %d evenly (at most %d)\n", 
             LENGTH, MAXPROC);
      MPI_Finalize();
      exit(0);
    }

    printf("Process %d on host %s is distributing arrays A and B to all %d processes\n\n", 
           me, myname, np);

    /* Scatter the arrays A and B to all processes */
    MPI_Scatter(A, LENGTH/np, MPI_INT, localA, LENGTH/np, MPI_INT, root, MPI_COMM_WORLD);
    MPI_Scatter(B, LENGTH/np, MPI_INT, localB, LENGTH/np, MPI_INT, root, MPI_COMM_WORLD);

    /* Add the local portions and store in localSum */
    for (i=0; i<LENGTH/np; i++) {
      localSum[i] = localA[i] + localB[i];
    }

    /* Print out own portion of the scattered arrays and their sum */
    printf("Process %d on host %s has:\n", me, myname);
    printf("  A elements:");
    for (i=0; i<LENGTH/np; i++) {
      printf(" %d", localA[i]);
    }
    printf("\n  B elements:");
    for (i=0; i<LENGTH/np; i++) {
      printf(" %d", localB[i]);
    }
    printf("\n  Sum elements:");
    for (i=0; i<LENGTH/np; i++) {
      printf(" %d", localSum[i]);
    }
    printf("\n\n");

    /* Receive messages with hostname and the sums from all other processes */
    for (i=1; i<np; i++) {
      MPI_Recv(&hostname[i], NAMELEN, MPI_CHAR, i, nametag, MPI_COMM_WORLD, &status);
      MPI_Recv(localSum, LENGTH/np, MPI_INT, i, datatag, MPI_COMM_WORLD, &status);
      
      printf("Process %d on host %s has sum elements:", i, hostname[i]);
      for (j=0; j<LENGTH/np; j++) {
        printf(" %d", localSum[j]);
      }
      printf("\n");
    }
    
    printf("Ready\n");

  } else { /* all other processes do this */

    /* Check configuration */
    if (np>MAXPROC || LENGTH % np != 0) {
      MPI_Finalize();
      exit(0);
    }

    printf("Process %d on host %s receiving scattered arrays\n", me, myname);

    /* Receive the scattered arrays from process 0 */
    MPI_Scatter(A, LENGTH/np, MPI_INT, localA, LENGTH/np, MPI_INT, root, MPI_COMM_WORLD);
    MPI_Scatter(B, LENGTH/np, MPI_INT, localB, LENGTH/np, MPI_INT, root, MPI_COMM_WORLD);
    
    /* Add the local portions */
    for (i=0; i<LENGTH/np; i++) {
      localSum[i] = localA[i] + localB[i];
    }
    
    /* Print local data */
    printf("Process %d on host %s has:\n", me, myname);
    printf("  A elements:");
    for (i=0; i<LENGTH/np; i++) {
      printf(" %d", localA[i]);
    }
    printf("\n  B elements:");
    for (i=0; i<LENGTH/np; i++) {
      printf(" %d", localB[i]);
    }
    printf("\n  Sum elements:");
    for (i=0; i<LENGTH/np; i++) {
      printf(" %d", localSum[i]);
    }
    printf("\n\n");
    
    /* Send own name back to process 0 */
    MPI_Send(myname, NAMELEN, MPI_CHAR, 0, nametag, MPI_COMM_WORLD);
    
    /* Send the calculated sum back to process 0 */
    MPI_Send(localSum, LENGTH/np, MPI_INT, 0, datatag, MPI_COMM_WORLD);
    
    printf("Process %d on host %s has sent name and sum array back\n", me, myname);
  }

  MPI_Finalize();
  exit(0);
}
