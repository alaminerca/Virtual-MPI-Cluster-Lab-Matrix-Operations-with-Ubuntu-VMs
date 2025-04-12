/* A simple MPI program that multiplies a matrix A (16x16) with a vector X (16x1)  */
/* The program distributes rows of matrix A among processes using MPI_Scatter     */
/* Each process computes its portion of the result and sends it back to master    */
/* Process 0 combines the results to form the final output vector                 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

#define N 16     /* Matrix size N x N */
#define NAMELEN 80   /* Max length of machine name */

int main(int argc, char* argv[]) {
  int i, j, np, me;
  const int resulttag = 45;    /* Tag value for sending result data */
  const int root = 0;         /* Root process in scatter */
  MPI_Status status;          /* Status object for receive */

  char myname[NAMELEN];       /* Local host name string */
  char hostname[4][NAMELEN];  
  float matA[N][N];           /* Full matrix A on root process */
  float matX[N];              /* Vector X to be multiplied */
  float result[N];            /* Final result vector on root */
  
  float localA[N/4][N];       /* Local portion of matrix A (assuming 4 processes) */
  float localResult[N/4];     /* Local result vector */
  
  int rows_per_proc;          /* Number of rows per process */

  MPI_Init(&argc, &argv);                /* Initialize MPI */
  MPI_Comm_size(MPI_COMM_WORLD, &np);    /* Get nr of processes */
  MPI_Comm_rank(MPI_COMM_WORLD, &me);    /* Get own identifier */
  


  gethostname(myname, NAMELEN);    /* Get host name */
  
  /* Calculate how many rows each process gets */
  rows_per_proc = N / np;
  
  if (me == 0) {    /* Process 0 does this */
    printf("Number of processors: %d\n", np);
    
    /* Initialize the matrix A and vector X */
    for (i = 0; i < N; i++) {
      for (j = 0; j < N; j++) {
        matA[i][j] = i * N + j;  /* Simple initialization */
      }
      matX[i] = i + 1;  /* Initialize X with simple values */
    }
    
    /* Print matrix A for verification */
    printf("Matrix A:\n");
    for (i = 0; i < N; i++) {
      for (j = 0; j < N; j++) {
        printf("%6.2f ", matA[i][j]);
      }
      printf("\n");
    }
    
    /* Print vector X for verification */
    printf("Vector X:\n");
    for (i = 0; i < N; i++) {
      printf("%6.2f\n", matX[i]);
    }
    
    /* Scatter the matrix A among processes */
    MPI_Scatter(&matA[0][0], rows_per_proc * N, MPI_FLOAT, 
                &localA[0][0], rows_per_proc * N, MPI_FLOAT, 
                root, MPI_COMM_WORLD);
                
    /* Broadcast vector X to all processes */
    MPI_Bcast(&matX[0], N, MPI_FLOAT, root, MPI_COMM_WORLD);
    
    /* Master also computes its portion */
    for (i = 0; i < rows_per_proc; i++) {
      localResult[i] = 0.0;
      for (j = 0; j < N; j++) {
        localResult[i] += localA[i][j] * matX[j];
      }
    }
    
    /* Copy master's results to the final result vector */
    for (i = 0; i < rows_per_proc; i++) {
      result[i] = localResult[i];
    }
    
    /* Receive results from other processes */
    for (i = 1; i < np; i++) {
      MPI_Recv(&result[i * rows_per_proc], rows_per_proc, MPI_FLOAT,
              i, resulttag, MPI_COMM_WORLD, &status);
    }
    
    /* Print the final result vector */
    printf("\nMatrix-Vector Multiplication Result (A * X):\n");
    for (i = 0; i < N; i++) {
      printf("%6.2f\n", result[i]);
    }
    
  } else { /* All other processes do this */
    
    /* Receive scattered matrix rows */
    MPI_Scatter(&matA[0][0], rows_per_proc * N, MPI_FLOAT, 
                &localA[0][0], rows_per_proc * N, MPI_FLOAT, 
                root, MPI_COMM_WORLD);
    
    /* Receive broadcasted vector X */
    MPI_Bcast(&matX[0], N, MPI_FLOAT, root, MPI_COMM_WORLD);
    
    /* Compute local portion of the result */
    for (i = 0; i < rows_per_proc; i++) {
      localResult[i] = 0.0;
      for (j = 0; j < N; j++) {
        localResult[i] += localA[i][j] * matX[j];
      }
    }
    
    /* Print local portion for debugging */
    printf("Process %d on host %s computed results:\n", me, myname);
    for (i = 0; i < rows_per_proc; i++) {
      printf("%6.2f\n", localResult[i]);
    }
    
    /* Send local results back to master */
    MPI_Send(&localResult[0], rows_per_proc, MPI_FLOAT, 
            0, resulttag, MPI_COMM_WORLD);
  }

  MPI_Finalize();
  return 0;
}
