#include <mpi.h>
#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <string.h>
#include "allFunctions.h"
#include <math.h>

#define DEFAULT_FILE "words"
#define ROOT 0
int main(int argc, char *argv[]) 
{
	int size, rank, i, key_size, num_threads=4, numWords_val;
	long cipher_size, valid_size, key_val;
	unsigned long partStart, partStop, part;
	char *cipher, **valid, *key, *plain;
	char *cipher_filename, *valid_filename;
	long *all_keys, *keyNum;
	int *all_numWords, *numWords;
	MPI_Status  status; 
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
 
	if(rank == ROOT)
	{
		if(rank==ROOT)
			print_banner();
		cipher_filename = (char*)malloc(MAX_FILENAME_SIZE * sizeof(char));
		valid_filename = (char*)malloc(MAX_FILENAME_SIZE * sizeof(char));
		if(!cipher_filename || !valid_filename)
		{
			printf("malloc failed, try again...\n");
			exit(EXIT_FAILURE);
		}
		// key = (char*)malloc(MAX_FILENAME_SIZE * sizeof(char));
		if(argc == 3) 
		{
			if(rank==ROOT)
	      		printf("Brute forcing %s ...\n", argv[2]);
			key_size = atoi(argv[1]);
			strcpy(cipher_filename, argv[2]);
			strcpy(valid_filename, DEFAULT_FILE);
	   	}
	   	else if(argc == 4) 
		{	
			if(rank==ROOT)
				printf("Brute forcing %s with wordlist %s ...\n", argv[2], argv[3]);
			key_size = atoi(argv[1]);
			strcpy(cipher_filename, argv[2]);
			strcpy(valid_filename, argv[3]);
	   	}
		else
		{
			if(rank==ROOT)
				printf("This program requires 2-3 arguments.\n");
			return -1;
		}

	}
	
	if(rank==ROOT){
		cipher = readFile(cipher_filename, &cipher_size);
		valid = words_to_struct(valid_filename, &valid_size);
		if(!cipher || !valid)
		{
			printf("malloc failed, try again...\n");
			exit(EXIT_FAILURE);
		}
		int j;
		partStart = 0;
		part = (pow(2,key_size) - partStart)/(size);
		for(j=1; j<size; j++)
		{
			partStop = partStart+part;
			MPI_Send(&partStart, 1, MPI_UNSIGNED_LONG, j, 0, MPI_COMM_WORLD);
			MPI_Send(&partStop, 1, MPI_UNSIGNED_LONG, j, 0, MPI_COMM_WORLD);
			
			MPI_Send(&key_size, 1, MPI_INT, j, 0, MPI_COMM_WORLD);

			MPI_Send(&cipher_size, 1, MPI_LONG, j, 0, MPI_COMM_WORLD);
			MPI_Send(cipher, cipher_size, MPI_CHAR, j, 0, MPI_COMM_WORLD);

			MPI_Send(&valid_size, 1, MPI_LONG, j, 0, MPI_COMM_WORLD);
			int z;
			for(z=0; z<valid_size; z++){
				MPI_Send(valid[z], MAX_LINE_SIZE, MPI_CHAR, j, 0, MPI_COMM_WORLD);

			}
			partStart+=part;			
		}
		partStop = (pow(2,key_size));
	}
	else
	{	
		MPI_Recv(&partStart, 1, MPI_UNSIGNED_LONG, ROOT, 0, MPI_COMM_WORLD, &status);
		MPI_Recv(&partStop, 1, MPI_UNSIGNED_LONG, ROOT, 0, MPI_COMM_WORLD, &status);

		MPI_Recv(&key_size, 1, MPI_INT, ROOT, 0, MPI_COMM_WORLD, &status);
		
		MPI_Recv(&cipher_size, 1, MPI_LONG, ROOT, 0, MPI_COMM_WORLD, &status);
		cipher = (char*)malloc(sizeof(char) * cipher_size);
		if(!cipher)
		{
			printf("malloc failed, try again...\n");
			exit(EXIT_FAILURE);
		}
		MPI_Recv(cipher, cipher_size, MPI_CHAR, ROOT, 0, MPI_COMM_WORLD, &status);

		MPI_Recv(&valid_size, 1, MPI_LONG, ROOT, 0, MPI_COMM_WORLD, &status);
		valid = (char**)malloc((valid_size)*sizeof(char*));
		if(!valid)
		{
			printf("malloc failed, try again...\n");
			exit(EXIT_FAILURE);
		}
		int z;
		for(z=0; z<valid_size; z++){
			valid[z] = (char*)malloc(MAX_LINE_SIZE*sizeof(char));
			if(!valid[z])
			{
				printf("malloc failed, try again...\n");
				exit(EXIT_FAILURE);
			}
			MPI_Recv(valid[z], MAX_LINE_SIZE, MPI_CHAR, ROOT, 0, MPI_COMM_WORLD, &status);
		}
	}

		long tmp_size; 
		if(rank==ROOT){
			double time = MPI_Wtime();
			char* tmp = chageDimension(valid, valid_size, &tmp_size);
			// printf("part size = %ld\n", (partStop-partStart+1));
			compute_on_gpu(cipher, cipher_size, key_size/8, tmp, tmp_size, valid_size, partStart, partStop);
			numWords = (int*)calloc(num_threads, sizeof(int));
			if(!numWords)
				exit(EXIT_FAILURE);
			keyNum = (long*)calloc(num_threads, sizeof(long));
			if(!keyNum)
				exit(EXIT_FAILURE);
			getBestResults(num_threads, numWords, keyNum, &key_val, &numWords_val);

			all_numWords = (int*)calloc(size, sizeof(int));
			if(!all_numWords)
				exit(EXIT_FAILURE);
			all_keys = (long*)calloc(size, sizeof(long));
			if(!all_keys)
				exit(EXIT_FAILURE);

			MPI_Gather(&numWords_val, 1, MPI_INT, all_numWords, 
			1, MPI_INT, ROOT, MPI_COMM_WORLD);

			MPI_Gather(&key_val, 1, MPI_LONG, all_keys, 
			1, MPI_LONG, ROOT, MPI_COMM_WORLD);

			getBestResults(size, all_numWords, all_keys, &key_val, &numWords_val);
			check_key(cipher, cipher_size, key_size/8, valid, valid_size, key_val, 1);
		}else{
			double time = MPI_Wtime();
			numWords = (int*)calloc(num_threads, sizeof(int));
			if(!numWords)
				exit(EXIT_FAILURE);
			keyNum = (long*)calloc(num_threads, sizeof(long));
			if(!keyNum)
				exit(EXIT_FAILURE);
			brute_force_omp(cipher, cipher_size, key_size/8, valid, valid_size, partStart, partStop, num_threads, numWords, keyNum);
			getBestResults(num_threads, numWords, keyNum, &key_val, &numWords_val);
			MPI_Gather(&numWords_val, 1, MPI_INT, all_numWords, 
			1, MPI_INT, ROOT, MPI_COMM_WORLD);

			MPI_Gather(&key_val, 1, MPI_LONG, all_keys, 
			1, MPI_LONG, ROOT, MPI_COMM_WORLD);
		}
		for(int z=0;z<valid_size;z++)
		{
			free(valid[z]);
		}

    if(rank == ROOT)
	{
		free(valid_filename);
		free(cipher_filename);
		free(all_numWords);
		free(all_keys);
	}
	free(numWords);
	free(keyNum);
	free(valid);
	free(cipher);
	
	// free(plain);
	// free(key);

	MPI_Finalize();

	return 0;

}
