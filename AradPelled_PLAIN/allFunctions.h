#pragma once

#define MAX_FILENAME_SIZE 80
#define MAX_LINE_SIZE 256
#define banner_file "banner.txt"
#define RED     "\033[31m"
#define RESET   "\033[0m"

// printing banner
int print_banner();

// read file's content (using it to read encrypted & example files)
char* readFile(char* filename, long* size);

// compare words
static int compare(const void* a, const void* b);

// sort the words
void sort(const char* arr[], int n);

// binary search a word
int search(char* arr[], int n, char* word);

// read example words into a data structure
char** words_to_struct(char* filename, long* size);

// check if a key is correct 
int check_key(char* cipher, long size, int key_size, char* valid[], long valid_size, long i, int last);

// brute forcing keys with omp
void brute_force_omp(char* cipher, long size, int key_size, char* valid[], long valid_size, long partStart, long partStop, int num_threads, int* numWords, long* keyNum);

// get best key and num words he decrypted
void getBestResults(int size, int* numWords, long* keyNum, long* key_val, int* numWords_val);

// decrypt xor cipher with key
char* xor_decrypt(char* cipher, int size, char* key, int key_size, int mode);

// checking if the plain text is valid (according to example file)
int is_english(char* plain, int size, char* valid[], int size_valid, long i);

// printing the right key & plaintext
void print_result(char* plain, char* key);

int compute_on_gpu(char* cipher, long size, int key_size, char* valid, long valid_size, long num_rows, unsigned long partStart, unsigned long partStop);

// 2d char array -> 1d char array - for cuda
char* chageDimension(char* valid[], long valid_size, long* size);

// copy string from src to dst
void copy(char* src, char* dst, int size);