#include <stdio.h>
#include <stdlib.h>
#include "allFunctions.h"
#include <mpi.h>
#include <string.h>
#include <math.h>
#include <omp.h>

// printing banner
int print_banner()
{
	FILE *file = fopen(banner_file, "r");
    	int c;

    	if (file == NULL) return -1; //could not open file
    	while ((c = fgetc(file)) != EOF) {
		printf("%c", c);
    	}
	printf(RED "\n\tParallel Brute Forcing tool\n\n" RESET);
	fclose(file);
	return 1;
}

// read file's content (using it to read encrypted & example files)
char* readFile(char* filename, long* size)
{
    	FILE *file = fopen(filename, "r");
    	char *arr;
    	int n = 0;
    	char c;

    	if (file == NULL) return NULL; //could not open file
    	fseek(file, 0, SEEK_END);
    	long f_size = ftell(file);
    	fseek(file, 0, SEEK_SET);
    	arr = (char*)malloc((f_size)*sizeof(char));
		if(!arr)
			return NULL;

    for(n=0;n<f_size-1;n++)
	{
		(c = fgetc(file));
        arr[n] = (char)c;
    	}
	*size = n;
    	arr[n] = '\0';        
	fclose(file);
    	return arr;
}

// strings to lowercase
char tolower(unsigned char ch) {
    if (ch >= 'A' && ch <= 'Z')
        ch = 'a' + (ch - 'A');
    return ch;
}

// compare strings not sensitive to case
int strcasecmp(const char *s1, const char *s2) {
    const unsigned char *us1 = (const u_char *)s1,
                        *us2 = (const u_char *)s2;

    while (tolower(*us1) == tolower(*us2++))
        if (*us1++ == '\0')
            return (0);
    return (tolower(*us1) - tolower(*--us2));
}

// compare words
static int compare(const void* a, const void* b) 
{ 
  
    return strcasecmp(*(const char**)a, *(const char**)b); 
} 

// sort the words
void sort(char** arr, int n)
{
	qsort(arr, n, sizeof(const char*), compare);
}

// binary search a word
int search(char* arr[], int n, char* word)
{
	char*  w;	
	w = (char*)bsearch(&word, arr, n, sizeof(const char*), compare);
	if(w != NULL)
		return 1;
	else
		return 0;
}

// read example words into a data structure
char** words_to_struct(char* filename, long* size)
{
	FILE *file = fopen(filename, "r");
    char **arr;
    int n = 0;
    char *c= (char*)malloc(sizeof(int)*sizeof(char));
	if(!c)
		return NULL;
	char buff[80];
    if (file == NULL) {printf("Could not read file\n"); return NULL; }//could not open file
    fscanf(file, "%ld", size);
    arr = (char**)malloc((*size)*sizeof(char*));
	if(!arr)
		return NULL;
	fgets(buff, 80, file);
    for(n=0;n<*size;n++)
	{
        arr[n] = (char*)malloc(MAX_LINE_SIZE*sizeof(char));
		if(!arr[n])
			return NULL;
		fgets(arr[n], MAX_LINE_SIZE, file);		
		arr[n][strlen(arr[n])-1] = '\0';
    }
	fclose(file);
	free(c);
	sort(arr, *size);
    	return arr;
}

// check if a key is correct 
int check_key(char* cipher, long size, int key_size, char* valid[], long valid_size, long i, int last)
{
	char* key = (char*)malloc((key_size+1)*sizeof(char));
	if(!key)
	{
		printf("malloc failed, try again...\n");
		exit(EXIT_FAILURE);
	}
        for(int j=0;j<key_size;j++){
            		key[j]=(int)(i/pow(2,(j)*8))%(int)pow(2,(j+1)*8);
      	}
		key[key_size] = '\0';
		char *plain = xor_decrypt(cipher, size, key, key_size, 0);	
		int res = is_english(plain, 1, valid, valid_size, i);
		if(res > 8 && last == 0){
			printf("The key might be %ld - %s, result = %d\n", i, key, res);
		}else if(last == 1){
			char *tmp = xor_decrypt(cipher, size, key, key_size, 1);
			printf("The key is %ld - %s, result = %d\nPlain : \n %s\n", i, key, res, tmp);
			free(tmp);
		}
        free(key);
		return res;
}


// brute forcing keys with omp
void brute_force_omp(char* cipher, long size, int key_size, char* valid[], long valid_size, long partStart, long partStop, int num_threads, int* numWords, long* keyNum)
{
	omp_set_num_threads(num_threads);
	#pragma omp parallel
	{
		int myID;
		long i;
		#pragma omp for
		for(i=partStart; i<partStop; i++)
		{
			myID = omp_get_thread_num();
			int res = check_key(cipher, size, key_size, valid, valid_size, i, 0);
			if(numWords[myID] < res){
				numWords[myID] = res;
				keyNum[myID] = i;
			}
		}
	}
}

// get best key and num words he decrypted
void getBestResults(int size, int* numWords, long* keyNum, long* key_val, int* numWords_val)
{
	*numWords_val = numWords[0];
	*key_val = keyNum[0];
	int i;
	for(i=1; i<size; i++)
	{
		if(*numWords_val < numWords[i]){
			*numWords_val = numWords[i];
			*key_val = keyNum[i];
		}
	}
}

// decrypt xor cipher with key
// mode : 0 - for replacing characters with space (then strtok dividing by space)
// mode : 1 - for decrypting normally
char* xor_decrypt(char* cipher, int size, char* key, int key_size, int mode)
{
	char* plain = (char*)malloc(size*sizeof(char));
	if(!plain)
	{
		printf("malloc failed, try again...\n");
		exit(EXIT_FAILURE);
	}
	int i;
	char tmp;
	int key_bytes=1;
	for(i=0; i<size; i++)
	{
		tmp = cipher[i]^key[i%key_size];
		if((tmp == '\n' || tmp == '.' || tmp == '\r' || tmp == ',') && mode == 0)
			plain[i] = ' ';
		else
			plain[i] = tmp; 
	}
	return plain;
}

// checking if the plain text is valid (according to example file)
int is_english(char* plain, int size, char* valid[], int size_valid, long i)
{
	int result = 0;
	const char d[2] = " ";
	char* token = plain;
	token = strtok(token, d);
	while( token != NULL ) 
	{
    	int tmp=0;
		tmp = search(valid, size_valid, token); 
		result += tmp;
		token = strtok(NULL, d);
		
   	}
	free(plain);
	return result;
}

// printing the right key & plaintext
void print_result(char* plain, char* key)
{
	printf("Found key : ");
	printf(RED "%s\n" RESET, key);
	printf("Plain text :\n%s\n", plain);
}

// 2d char array -> 1d char array
char* chageDimension(char* valid[], long valid_size, long* return_size)
{
	int i, totalSize=0, addSize, size;
	char sizeC[3];
	size = strlen(valid[0]);
	snprintf(sizeC, sizeof(sizeC), "%d", size);
	addSize = size+strlen(sizeC)+1;
	totalSize += addSize;
	char* arr1d = (char*)malloc((addSize+1) * sizeof(char));
	if(!arr1d)
	{
		printf("malloc failed, try again...\n");
		exit(EXIT_FAILURE);
	}
	copy(sizeC, arr1d, addSize-size-1);
	copy(valid[0], &(arr1d[addSize-size-1]), size);
	for(i=1; i<valid_size; i++)
	{
		size = strlen(valid[i]);
		snprintf(sizeC, sizeof(sizeC), "%d", size);
		addSize = size+strlen(sizeC);
		totalSize += addSize;
		arr1d = (char*)realloc(arr1d, totalSize * sizeof(char));
		if(!arr1d)
		{
			printf("malloc failed, try again...\n");
			exit(EXIT_FAILURE);
		}
		copy(sizeC, &(arr1d[totalSize-addSize]), addSize-size);
		copy(valid[i], &(arr1d[totalSize-size]), size);
	}
	arr1d[totalSize] = '\0';
	*return_size = totalSize;
	return arr1d;
}

// copy string from src to dst
void copy(char* src, char* dst, int size)
{
	int i;
	for(i=0; i<size; i++)
	{
		dst[i] = src[i];
	}
}