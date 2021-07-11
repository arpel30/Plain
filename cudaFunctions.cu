#include <cuda_runtime.h>
#include "allFunctions.h"
#include <stdio.h>
#include <string.h>

// strings to lowercase
__device__ char tolowerCase(unsigned char ch) {
    if (ch >= 'A' && ch <= 'Z')
        ch = 'a' + (ch - 'A');
    return ch;
}

// compare strings not sensitive to case
__device__ int strcasecmpr(const char *s1, const char *s2) {
    const unsigned char *us1 = (const u_char *)s1,
                        *us2 = (const u_char *)s2;

    while (tolowerCase(*us1) == tolowerCase(*us2++))
        if (*us1++ == '\0')
            return (0);
    return (tolowerCase(*us1) - tolowerCase(*--us2));
}

__device__ int search_noExpand(char* valid, long size, char* word, int word_size)
{
	printf("word = %s\n", word);
	int offset=0, isSearch=1;
	for(int i=0; i<size; i++){
		if(valid[i] < '0' || valid[i] > '9')	
		{
			if(isSearch)
			{
				if(valid[i] == word[offset])
				{
					offset++;
					if(offset == word_size)
						return 1;
				}else
				{
					offset = 0;
					isSearch = 0;
				}
			}else
			{

			}
		}	
		else
		{
			isSearch = 1;
		}
	}
	return 0;
}

// check if the word is valid (only English letters)
__device__ int is_letters(char* word, int word_size)
{
	int i;
	for(i=0; i<word_size; i++)
	{
		if(!((word[i] >= 'a' && word[i] <= 'z') || (word[i] >= 'A' && word[i] <= 'Z')))
		{
			return 0;
		}
	}
	return 1;
}

// search word in 1d dictionary
__device__ int find_word(char* valid, long valid_size, char* word, int word_size)
{
	if(word_size <= 2) // statistically, there will be many errors in decoding that will result in words of 1-3 random letters
		return 0;
	if(!is_letters(word, word_size)) // search only if word contains only letters
		return 0;
	char* dict_word = (char*)malloc(20 * sizeof(char));
	if(!dict_word)
		return 0;
	int i=0, size=0, digits=1;
	while(i<valid_size)
	{
		// printf("find_word : %s, %d, %d\n", word, word_size, i);
		if(valid[i] >= '0' && valid[i] <= '9')
		{
			if(!strcasecmpr(word, dict_word)) // words are equal
			{
				free(dict_word);
				return 1;
			}
			size = size*(digits-1)*10 + valid[i] - '0';
			digits++;
			i++;
		}else if(valid[i] != 127) // DEL char
		{
			digits = 1;
			for(int j=0; j<size; j++)
			{
				dict_word[j] = valid[i];
				i++;
			}
			dict_word[size] = '\0';
		}else{
			i++;
		}
	}
	free(dict_word);
	return 0;
}

// search word in 2d dictionary
__device__ int search_word(char** dict, long size, char* word)
{
	for(int i=0; i<size; i++)
	{
		if(!strcasecmpr(word, dict[i]))
		{
			printf("%s = %s\n", word, dict[i]);
			return 1;
		}
	}
	return 0;
}

// expand 1d dictionary into 2d
__device__ void expand(char** dict, unsigned long valid_size, char* valid, int* longest_word)
{
	int size=0, line=0, digits=1;
	long i=0;
	*longest_word = 0;
	while(i<valid_size)
	{
		if(valid[i] >= '0' && valid[i] <= '9')
		{
			size = size*(digits-1)*10 + valid[i] - '0';
			digits++;
			i++;
			if(size > *longest_word)
				*longest_word = size;
		}else if(valid[i] != 127)
		{
			digits = 1;
			dict[line] = (char*)malloc((size+1) * sizeof(char));
			for(int j=0; j<size; j++)
			{
				dict[line][j] = valid[i];
				i++;
			}
			dict[line][size] = '\0';
			line++;
		}else{
			i++;
		}
	}
}

// check decrypted text in 2d dictionary
__device__ void is_english(int* ans, char** dict, long dict_size, char* plain, int size, int longest_word)
{
	int i=0, offset=0;
	char* word = (char*)malloc(longest_word * sizeof(char));
	while(i<size)
	{
		if(offset == longest_word)
			offset = 0;
		if(plain[i] == ' ')
		{
			word[offset] = '\0';
			*ans += search_word(dict, dict_size, word);
			offset = 0;
		}else{
			word[offset] = plain[i];
			offset++;
		}
		i++;
	}
	free(word);

}

// check decrypted text in 1d dictionary
__device__ void is_english_noExpand(int* ans, char* dict, long dict_size, char* plain, int size, int longest_word)
{
	int i=0, offset=0;
	char* word = (char*)malloc(longest_word * sizeof(char));
	if(!word)
		return;
	while(i<size)
	{
		if(offset == longest_word)
			offset = 0;
		if(plain[i] == ' ')
		{
			word[offset] = '\0';
			*ans += find_word(dict, dict_size, word, offset);
			offset = 0;
		}else{
			word[offset] = plain[i];
			offset++;
		}
		i++;
	}
	free(word);

}

// xor decryption with key
__device__ void xor_decrypt(char* cipher, unsigned long size, int key_size, long key_val, char* plain)
{
	char* key = (char*)malloc((key_size+1)*sizeof(char));
    for(int j=0;j<key_size;j++)
	{
        key[j]=(int)(key_val/powf(2,(j)*8))%(int)powf(2,(j+1)*8);
    }
		key[key_size] = '\0';
	char tmp;
	int i;
	for(i=0; i<size; i++)
	{
		tmp = cipher[i]^key[i%key_size];
		if((tmp == '\n' || tmp == '.' || tmp == '\r' || tmp == ','))
			plain[i] = ' ';
		else
			plain[i] = tmp; 
	}
	free(key);
}

__global__ void cuda_brute(char* cipher, unsigned long size, int key_size, char* valid, long valid_size, long num_rows, unsigned long partStart, unsigned long partStop) {

	unsigned long i = blockDim.x * blockIdx.x + threadIdx.x + partStart;
	int ans=0, longest_word=20;
	if(i < partStop+1) // just in key range
	{
		char* plain = (char*)malloc(size * sizeof(char));
		xor_decrypt(cipher, size, key_size, i, plain);
		if(num_rows <= 10)
		{
			char** dict = (char**)malloc(num_rows * sizeof(char*));
			expand(dict, valid_size, valid, &longest_word);
			is_english(&ans, dict, num_rows, plain, size, longest_word);
			for(int q=0; q<num_rows; q++)
			{
				free(dict[q]);
			}
			free(dict);
		}else{
			is_english_noExpand(&ans, valid, valid_size, plain, size, longest_word);
		}
		if(ans > 3)
			printf("answer = %d\n", ans);
		free(plain);
	}
}

int compute_on_gpu(char* cipher, long size, int key_size, char* valid, long valid_size, long num_rows, unsigned long partStart, unsigned long partStop) {
    	// Error code to check return values for CUDA calls
    	cudaError_t err = cudaSuccess;
		cudaDeviceReset();
    	// Allocate memory on GPU to copy the data from the host
    	char *private_cipher;
		char *private_valid;
		
	err = cudaMalloc((void **)&private_cipher, size);
    	if (err != cudaSuccess) {
		fprintf(stderr, "Failed to allocate device memory - %s\n", cudaGetErrorString(err));
		exit(EXIT_FAILURE);
        }
        
	err = cudaMalloc((void**)&private_valid, sizeof(char*) * valid_size);
    	if (err != cudaSuccess) {
		fprintf(stderr, "Failed to allocate device memory dictionary - %s\n", cudaGetErrorString(err));
		exit(EXIT_FAILURE);
        }

    	// Copy data from host to the GPU memory
    err = cudaMemcpy(private_cipher, cipher, size, cudaMemcpyHostToDevice);
    	if (err != cudaSuccess) {
			fprintf(stderr, "Failed to copy data from host to device - %s\n", cudaGetErrorString(err));
			exit(EXIT_FAILURE);
    	}

	err = cudaMemcpy(private_valid, valid, strlen(valid), cudaMemcpyHostToDevice);
    	if (err != cudaSuccess) {
			fprintf(stderr, "Failed to copy data from host to device - %s\n", cudaGetErrorString(err));
			exit(EXIT_FAILURE);
    	}


    	// Launch the Kernel
    	int threadsPerBlock = 256;
    	int blocksPerGrid =((partStop - partStart) + threadsPerBlock - 1) / threadsPerBlock;
    	cuda_brute<<<blocksPerGrid, threadsPerBlock>>>(private_cipher, size, key_size, private_valid, valid_size, num_rows, partStart, partStop);
    	err = cudaGetLastError();
    	if (err != cudaSuccess) {
			fprintf(stderr, "Failed to launch vectorAdd kernel -  %s\n", cudaGetErrorString(err));
			exit(EXIT_FAILURE);
    	}

		
    	// Free allocated memory on GPU
    if (cudaFree(private_cipher) != cudaSuccess) {
		// fprintf(stderr, "Failed to free device data - cipher - %s\n", cudaGetErrorString(err));
		//exit(EXIT_FAILURE);
		// return 0;
	}
		
	if (cudaFree(private_valid) != cudaSuccess) {
		// fprintf(stderr, "Failed to free device data - valid - %s\n", cudaGetErrorString(err));
		//exit(EXIT_FAILURE);
		// return 0;
	}
    return 0;
}
