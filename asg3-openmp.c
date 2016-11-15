/*
* CSCI3150 Assignment 3 - Implement pthread and openmp program
*
*/

/* Header Declaration */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

/* Function Declaration */
extern int *readdata(char *filename, long *number);

typedef struct list {
  int value;
  int positionCount;
  struct list *prev;
} linkedList;

struct args {
  int* list;
  int leftPos;
  int rightPos;
} args;

int initHashMap[3125000]; //use a huge bit array to hash values - abusing global value initialisation to 0 here 
int compareHashMap[3125000];
linkedList *initMatch;
int matchCount; //Necessary because malloc doesn't track array size
int* sortedArray;

void setBit(int k, int hashMap[])
{
  hashMap[k/32] |= 1 << (k%32);  // With thanks to http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html
}

void clearBit(int k, int hashMap[])                
{
  hashMap[k/32] &= ~(1 << (k%32));
}

int testBit(int k, int hashMap[])
{
  return ( (hashMap[k/32] & (1 << (k%32) )) != 0 ) ;     
}


int* compare(int* list1, int list1size, int* list2, int list2size) { //returns array of matching ints
  int i;
  for (i = 0; i < list1size; i++) {
    setBit(*(list1 + i), initHashMap);
  }


  linkedList* newNode;
  linkedList* prevNode;
  for (i = 0; i < list2size; i++) {
    if (testBit(*(list2 + i), initHashMap) && !testBit(*(list2 + i), compareHashMap)) { //short circuit on the && operator useful here
        setBit(*(list2 + i), compareHashMap);
        if (!initMatch) {
          initMatch = (linkedList*)malloc(sizeof(linkedList));
          initMatch->value = *(list2 + i);
          initMatch->positionCount = 1;
          prevNode = initMatch;
        } else {
          newNode = (linkedList*)malloc(sizeof(linkedList));
          newNode->prev = prevNode;
          newNode->value = *(list2 + i);
          newNode->positionCount = prevNode->positionCount + 1;
          prevNode = newNode;
        }
    }
  }

  matchCount = prevNode->positionCount;
  int* matchesArray = (int*)malloc(matchCount*sizeof(int));
  // printf("array vals:\n");
  for (i = 0; i < matchCount; i++) {
    *(matchesArray + i) = prevNode->value;
    prevNode = prevNode->prev;
    // printf("%i\n", *(matchesArray + i));
  }

  return matchesArray;
}

void merge(int *list, int low, int mid, int high) {
  int l1, l2, i;

  for(l1 = low, l2 = mid + 1, i = low; l1 <= mid && l2 <= high; i++) {
    // printf("comparing: %i and %i\n", *(list + l1), *(list + l2));
    if(*(list + l1) <= *(list + l2)) {
      // printf("chose %i for slot %i\n", *(list + l1), i);
      *(sortedArray + i) = *(list + l1++);
    } else {
      // printf("chose %i for slot %i\n", *(list + l2), i);
      *(sortedArray + i) = *(list + l2++);
    }
  }

  // printf("got here\n");
  // for (int j = low; j <= high; j++) {
  //     printf("pos %i: %i\n", j, *(sortedArray + j));
  // }
   
  while(l1 <= mid) {
    // printf("adding element %i at pos %i\n", *(list + l1), i);
    *(sortedArray + i++) = *(list + l1++);
  }

  while(l2 <= high) {   
    // printf("adding element %i at pos %i\n", *(list + l2), i);
    *(sortedArray + i++) = *(list + l2++);
  }


  // for (int j = low; j < i; j++) {
  //   printf("pos %i:%i\n", j, *(sortedArray + j));
  // }

  for(i = low; i <= high; i++)
    *(list + i) = *(sortedArray + i);
}

void mergeSort(int *list, int low, int high) {
  int mid;

  if (low < high) {
    mid = (low + high) / 2;
    mergeSort(list, low, mid);
    mergeSort(list, mid + 1, high);
    merge(list, low, mid, high);
  } else {
    return;
  }
}

void *threadHandler(void* args) {
  mergeSort(((struct args*) args)->list, ((struct args*) args)->leftPos, ((struct args*) args)->rightPos);
  return NULL;
}

void sort(FILE *outputfile, int* list, int threadCount) {
  sortedArray = (int*)malloc(matchCount*sizeof(int));

  int i;
  for (i = 0; i < matchCount; i++) {
    *(sortedArray + i) = *(list + i);
  }

  omp_set_num_threads(threadCount);

  #pragma omp parallel for
  for (i = 0; i < threadCount; i++) {
    int leftPos = (i * (matchCount - 1)) / threadCount;
    int rightPos = ((i + 1) * (matchCount - 1)) / threadCount;
    if (i > 0)
      leftPos++;

    mergeSort(list, leftPos, rightPos);
    // printf("thread %i sorted pos %i to %i\n", i, leftPos, rightPos);
    // for (int j = 0; j < matchCount; j++) {
    //   printf("%i\n", *(sortedArray + j));
    // }
  }

  for (i = 0; i < threadCount - 1; i++) {
    int leftPos = 0;
    int midPos = ((i + 1) * (matchCount - 1)) / threadCount;
    int rightPos = ((i + 2) * (matchCount - 1)) / threadCount;
    // printf("merging from pos %i to %i with %i to %i\n", leftPos, midPos, midPos + 1, rightPos);
    merge(list, leftPos, midPos, rightPos);
    // for (int i = leftPos; i <= rightPos; i++) {
    //   printf("%i\n", *(list + i));
    // }
  }

  // printf("final result:\n");
  int j;
  for (j = 0; j < matchCount; j++) {
    fprintf(outputfile, "%i\n", *(sortedArray + j));
  }
}

/* Main */

int main(int argc, char *argv[]) {
  if(argc!=5) {
    printf("usage:\n");
    printf("    ./asg3-openmp inputfile1 inputfile2 outputfile ThreadNum\n");

    return -1;
  }

  int *array1, *array2;
  long num1, num2;

  array1 = readdata(argv[1], &num1);
  array2 = readdata(argv[2], &num2);

  /* do your assignment start from here */

  //int* matchList = compare(array1, num1, array2, num2);
  FILE *fp=fopen(argv[3], "w");
  //sort(fp, matchList, atoi(argv[4]));
  fclose(fp);

  return 0;
}
