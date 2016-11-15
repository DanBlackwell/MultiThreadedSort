/*
* CSCI3150 Assignment 3 - Implement pthread and openmp program
*
*/

/* Header Declaration */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

/* Function Declaration */
extern int *readdata(char *filename, long *number);

typedef struct list {
  int value;
  int positionCount;
  struct list *prev;
} linkedList;

typedef struct args {
  int* list;
  int leftPos;
  int rightPos;
} args;

typedef struct searchArea {
  int* matchesArray;
  int* compareHash;
  int left;
  int right;
  int ctrStartPos;
} searchArea;

int initHashMap[3125000]; //use a huge bit array to hash values - abusing global value initialisation to 0 here 
int compareHashMap[3125000];
linkedList *initMatch;
int matchCount; //Necessary because malloc doesn't track array size
int* sortedArray;
int curPos;

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

void readIntoArray(int* matchesArray, int* compareHash, int left, int right, int ctrStartPos) {
  int i, insertPos;

  printf("Thread starting from pos: %i going to pos: %i, ctrStratPos: %i\n", left, right, ctrStartPos);
  for (i = left; i <= right; i++) {
    if (testBit(i, compareHash)) {
      *(matchesArray + ctrStartPos++) = i;
 //     printf("added %i at pos %i\n", i, ctrStartPos - 1);
    }
  }
}

void *threadHandler(void* args) {
  readIntoArray(((struct searchArea*) args)->matchesArray, ((struct searchArea*) args)->compareHash, ((struct searchArea*) args)->left, ((struct searchArea*) args)->right, ((struct searchArea*) args)->ctrStartPos);
  pthread_exit(NULL);
}

int* compare(int* list1, int list1size, int* list2, int list2size, int threadCount, FILE* output) { //returns array of matching ints
  clock_t start = clock(), diff;

  int i, j;
  for (i = 0; i < list1size; i++) {
   setBit(*(list1 + i), initHashMap);
  }

  diff = clock() - start;
  int msec = diff * 1000 / CLOCKS_PER_SEC;
  printf("setting initial array took: %dms\n", msec);  
  start = clock();

  linkedList* newNode;
  linkedList* prevNode;
  int ctr = 0, highestSeen = 0;

  for (i = 0; i < list2size; i++) {
    if (testBit(*(list2 + i), initHashMap) && !testBit(*(list2 + i), compareHashMap)) { //short circuit on the && operator useful here
      setBit(*(list2 + i), compareHashMap);
      ctr++;
      if (*(list2 + i) > highestSeen) 
        highestSeen = *(list2 + i);
    }
  }

  int regionCount[threadCount];
  int regionLeftPos[threadCount], regionRightPos[threadCount];
  int regionStartPos[threadCount];
  for (i = 0; i < threadCount; i++) {
    regionCount[i] = 0;
    regionStartPos[i] = 0;
  }

  for (i = 0; i < threadCount; i++) {
    regionLeftPos[i] = (i * highestSeen) / threadCount;
    if (i > 0) 
      regionLeftPos[i]++;
    regionRightPos[i] = ((i + 1) * highestSeen) / threadCount;
    printf("Region %i, leftPos: %i, rightPos: %i\n", i, regionLeftPos[i], regionRightPos[i]);
  }

  for (i = 0; i < highestSeen; i++) {
    if (testBit(i, compareHashMap)) {
      for (j = 0; j < threadCount; j++) {
        if (i >= regionLeftPos[j] && i <= regionRightPos[j]) { //can optimise here lumping all in final segment together ie skip loop
          regionCount[j]++;
         // printf("Added %i between %i and %i\n", i, regionLeftPos[j], regionRightPos[j]);
          break;
        }
      }
    }
  }

  regionStartPos[0] = 0;
  for (i = 1; i < threadCount; i++) {
    regionStartPos[i] = regionCount[i - 1] + regionStartPos[i - 1];
    printf("region %i startPos: %i\n", i, regionStartPos[i]);
  }

  diff = clock() - start;
  msec = diff * 1000 / CLOCKS_PER_SEC;
  printf("creating linked list took: %dms\n", msec);
  start = clock();

  matchCount = ctr;//prevNode->positionCount;
  int posCtr = 0;
  int* matchesArray = (int*)malloc(matchCount*sizeof(int));
//  printf("Match count: %i, highestSeen: %i\n", matchCount, highestSeen);
  
  pthread_t thread[threadCount];
  searchArea area[threadCount];
  for (i = 0; i < threadCount; i++) {
    area[i].matchesArray = matchesArray;
    area[i].compareHash = compareHashMap;
    area[i].left = (i * highestSeen) / threadCount;
    area[i].right = ((i + 1) * highestSeen) / threadCount;
    area[i].ctrStartPos = regionStartPos[i];
    if (i > 0)
      area[i].left++;

    pthread_create(&thread[i], NULL, threadHandler, &area[i]);
  }

  for (i = 0; i < threadCount; i++) {
    pthread_join(thread[i], NULL);
  }

//  printf("Match Array:\n");
  for (i = 0; i < matchCount; i++) {
 //   printf("%i: %i\n", i, *(matchesArray + i));
    fprintf(output, "%i\n", *(matchesArray + i));
  }

  // for (i = 0; i <= highestSeen; i++) {
  //   if (testBit(i, compareHashMap)) {
  //     *(matchesArray + posCtr++) = i;
  //   }
  // }

  diff = clock() - start;
  msec = diff * 1000 / CLOCKS_PER_SEC;
  printf("list to array took: %dms\n", msec);

  return matchesArray;
}

void merge(int *list, int low, int mid, int high) {
  int l1, l2, i;

  for(l1 = low, l2 = mid + 1, i = low; l1 <= mid && l2 <= high; i++) {
    //printf("comparing: %i and %i\n", *(list + l1), *(list + l2));
    if(*(list + l1) <= *(list + l2)) {
      //printf("chose %i for slot %i\n", *(list + l1), i);
      *(sortedArray + i) = *(list + l1++);
    } else {
      //printf("chose %i for slot %i\n", *(list + l2), i);
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


void finalMerge(int *list, int low, int mid, int high) {
  int l1, l2, i;

  for(l1 = low, l2 = mid + 1, i = low; l1 <= mid && l2 <= high; i++) {
    printf("comparing: %i and %i\n", *(list + l1), *(list + l2));
    if(*(list + l1) <= *(list + l2)) {
      printf("chose %i for slot %i\n", *(list + l1), i);
      *(sortedArray + i) = *(list + l1++);
    } else {
      printf("chose %i for slot %i\n", *(list + l2), i);
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

// void *threadHandler(void* args) {
//   mergeSort(((struct args*) args)->list, ((struct args*) args)->leftPos, ((struct args*) args)->rightPos);
//   pthread_exit(NULL);
// }

void sort(FILE *outputfile, int* list, int threadCount) {
  sortedArray = (int*)malloc(matchCount*sizeof(int));
  pthread_t thread[4];
  args arguments[4];

  int i;
  // for (i = 0; i < matchCount; i++) {
  //   *(sortedArray + i) = *(list + i);
  // }

  memcpy(sortedArray, list, matchCount*sizeof(int));

  for (i = 0; i < threadCount; i++) {
    int leftPos = (i * (matchCount - 1)) / threadCount;
    int rightPos = ((i + 1) * (matchCount - 1)) / threadCount;
    if (i > 0)
      leftPos++;
    
    
    arguments[i].list = list;
    arguments[i].leftPos = leftPos;
    arguments[i].rightPos = rightPos;

    pthread_create(&thread[i], NULL, threadHandler, &arguments[i]);
  }

  for (i = 0; i < threadCount; i++) { //rejoin threads
    pthread_join(thread[i], NULL);
  }

  for (i = 0; i < threadCount - 1; i++) {
    int leftPos = 0;
    int midPos = ((i + 1) * (matchCount - 1)) / threadCount;
    int rightPos = ((i + 2) * (matchCount - 1)) / threadCount;
    //printf("merging from pos %i to %i with %i to %i\n", leftPos, midPos, midPos + 1, rightPos);
    merge(list, leftPos, midPos, rightPos);
    // for (int i = leftPos; i <= rightPos; i++) {
    //   printf("%i\n", *(list + i));
    // }
  }

  int j;
  for (j = 0; j < matchCount; j++) {
    fprintf(outputfile, "%i\n", *(sortedArray + j));
  }
}

/* Main */


int main(int argc, char *argv[]) {
  if(argc!=5) {
    printf("usage:\n");
    printf("    ./asg3-pthread inputfile1 inputfile2 outputfile ThreadNum\n");

    return -1;
  }

  int *array1, *array2;
  long num1, num2;

  array1 = readdata(argv[1], &num1);
  array2 = readdata(argv[2], &num2);

  /* do your assignment start from here */

  FILE *fp=fopen(argv[3], "w");
  int* matchList = compare(array1, num1, array2, num2, atoi(argv[4]), fp);
  // sort(fp, matchList, atoi(argv[4]));
  fclose(fp);

  return 0;
}
