/*
* CSCI3150 Assignment 3 - Implement pthread and openmp program
* Feel free to modify the given code.
*
*/

/* Header Declaration */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

typedef struct list {
  int value;
  int positionCount;
  struct list *prev;
} linkedList;

int initHashMap[3125000]; //use a huge bit array to hash values - abusing global value initialisation to 0 here 
linkedList *initMatch;
int matchCount; //Necessary because malloc doesn't track array size
int* sortedArray;

/* Function Declaration */

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


int* compare(FILE *fp, FILE *input1, FILE *input2) { //returns array of matching ints
  int input1size; 
  fscanf(input1, "%d", &input1size);
  printf("File 1: %d integers\n", input1size);
  char temp;
  char numberIn[10];
  int count;
  int number;
  while(1) {
    temp = fgetc(input1);
    if (temp == EOF) {
      break;
    } else if (temp == ' ') {
      numberIn[count] = '\000';
      number = atoi(numberIn);
      count = 0;
      setBit(number, initHashMap);
    } else {
      numberIn[count] = temp;
      count++;
  	}
  } 
  fclose(input1);

  for (int i = 0; i < input1size; i++) {
    printf("bit %i: %i\n", i, testBit(i, initHashMap));
  }

  int input2size; 
  fscanf(input2, "%d", &input2size);
  printf("\nFile 2: %d integers\n", input2size);
  linkedList* newNode;
  linkedList* prevNode;
  while(1) {
    temp = fgetc(input2);
    if (temp == EOF) {
      break;
    } else if (temp == ' ') {
      numberIn[count] = '\000';
      number = atoi(numberIn);
      count = 0;
      if (testBit(number, initHashMap)) {
        if (!initMatch) {
          initMatch = (linkedList*)malloc(sizeof(linkedList));
          initMatch->value = number;
          initMatch->positionCount = 1;
          prevNode = initMatch;
        } else {
          newNode = (linkedList*)malloc(sizeof(linkedList));
          newNode->prev = prevNode;
          newNode->value = number;
          newNode->positionCount = prevNode->positionCount + 1;
          prevNode = newNode;
        }
        printf("value: %i, positionCount: %i\n", prevNode->value, prevNode->positionCount);
      }
    } else {
      numberIn[count] = temp;
      count++;
    }
  }
  fclose(input2); 

  matchCount = prevNode->positionCount;
  int* matchesArray = (int*)malloc(matchCount*sizeof(int));
  printf("array vals:\n");
  for (int i = 0; i < matchCount; i++) {
    *(matchesArray + i) = prevNode->value;
    prevNode = prevNode->prev;
    printf("%i\n", *(matchesArray + i));
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

void sort(FILE *outputfile, int* list, int threadCount) {
  sortedArray = (int*)malloc(matchCount*sizeof(int));
  for (int i = 0; i < matchCount; i++) {
    *(sortedArray + i) = *(list + i);
  }

  for (int i = 0; i < threadCount; i++) {
    int leftPos = (i * (matchCount - 1)) / threadCount;
    int rightPos = ((i + 1) * (matchCount - 1)) / threadCount;
    if (i > 0)
      leftPos++;
    mergeSort((list + leftPos), 0, rightPos - leftPos);

    printf("thread %i sorted pos %i to %i\n", i, leftPos, rightPos);
    for (int j = 0; j < matchCount; j++) {
      printf("%i\n", *(sortedArray + j));
    }
  }

  for (int i = 0; i < threadCount - 1; i++) {
    int leftPos = 0;
    int midPos = ((i + 1) * (matchCount - 1)) / threadCount;
    int rightPos = ((i + 2) * (matchCount - 1)) / threadCount;
    printf("merging from pos %i to %i with %i to %i\n", leftPos, midPos, midPos + 1, rightPos);
    merge(list, leftPos, midPos, rightPos);
    for (int i = leftPos; i <= rightPos; i++) {
      printf("%i\n", *(list + i));
    }
  }

  printf("final result:\n");
  int prevValue = -1;
    for (int j = 0; j < matchCount; j++) {
      if (!(*(sortedArray + j) == prevValue)) { 
       printf("%i\n", *(sortedArray + j));
      }
      prevValue = *(sortedArray + j);
    }
}

/* Main */

int main(int argc, char *argv[]) {
    if(argc!=5) {
        printf("usage:\n");
        printf("    ./asg3-openmp inputfile1 inputfile2 outputfile ThreadNum\n");

        return -1;
    }

    FILE *fp=fopen(argv[3], "w");
    FILE *inputf1=fopen(argv[1], "r");
    FILE *inputf2=fopen(argv[2], "r");

    int* result = compare(fp, inputf1, inputf2);
    sort(fp, result, atoi(argv[4]));

    fclose(fp);
    fclose(inputf2);

    return 0;
}