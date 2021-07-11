build:
	mpicxx -fopenmp -c main.c -o main.o
	mpicxx -fopenmp -c cFunctions.c -o cFunctions.o
	nvcc -I./inc -c cudaFunctions.cu -o cudaFunctions.o
	mpicxx -fopenmp -o PLAIN  main.o cFunctions.o cudaFunctions.o  /usr/local/cuda-9.1/lib64/libcudart_static.a -ldl -lrt

clean:
	rm -f *.o ./mpiCudaOpenMP

run:
	mpiexec -np 16 ./PLAIN 24 cipher.txt words.txt

runPlain:
	mpiexec -np 16 ./PLAIN 24 plain.txt words.txt

runLawyer:
	mpiexec -np 24 ./PLAIN 8 lawyerJoke.e words.txt

runLawyerLong:
	mpiexec -np 24 ./PLAIN 8 lawyerJoke.e

runMovie:
	mpiexec -np 24 ./PLAIN 32 movie_quotes.e words.txt

run32:
	mpiexec -np 16 ./PLAIN 32 cipher32.txt words.txt

runLongFile:
	mpiexec -np 16 ./PLAIN 24 cipher.txt words

runOn2:
	mpiexec -np 2 -machinefile  mf  -map-by  node  ./PLAIN

