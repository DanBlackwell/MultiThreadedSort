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

typedef struct searchArea {
  int* matchesArray;
  int* compareHash;
  int left;
  int right;
  int ctrStartPos;
} searchArea;

int initHashMap[3125000]; //use a huge bit array to hash values - abusing global value initialisation to 0 here 
int compareHashMap[3125000];

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
    }
  }
}

void *threadHandler(void* args) {
  readIntoArray(((struct searchArea*) args)->matchesArray, ((struct searchArea*) args)->compareHash, ((struct searchArea*) args)->left, ((struct searchArea*) args)->right, ((struct searchArea*) args)->ctrStartPos);
  pthread_exit(NULL);
}

void compare(int* list1, int list1size, int* list2, int list2size, int threadCount, FILE* output) { //returns array of matching ints
  clock_t start = clock(), diff;

  int i, j;
  for (i = 0; i < list1size; i++) {
   setBit(*(list1 + i), initHashMap);
  }

  diff = clock() - start;
  int msec = diff * 1000 / CLOCKS_PER_SEC;
  printf("setting initial array took: %dms\n", msec);  
  start = clock();

  int ctr = 0, highestSeen = 0;

  for (i = 0; i < list2size; i++) {
    if (testBit(*(list2 + i), initHashMap) && !testBit(*(list2 + i), compareHashMap)) { //short circuit on the && operator useful here
      setBit(*(list2 + i), compareHashMap);
      ctr++;
      if (*(list2 + i) > highestSeen) 
        highestSeen = *(list2 + i);
    }
  }

  int regionMinCount[threadCount], regionMin[threadCount], regionMaxPos[threadCount];
  for (i = 0; i < threadCount; i++) {
    printf("Region min count is: %i, ctr = %i\n", (ctr * i) / threadCount, ctr);
    regionMinCount[i] = (ctr * i) / threadCount;
    if (i > 0)
      regionMinCount[i]++;
  }

  int nextRegionToSet = 1, posCtr = 0;
  regionMin[0] = 0;
  for (i = 0; i < highestSeen; i++) {
    if (testBit(i, compareHashMap)) {
      posCtr++;
      printf("I am at %i and I have %i mathces\n", i, posCtr);
      if (posCtr == regionMinCount[nextRegionToSet]) {
        regionMin[nextRegionToSet++] = i;
        printf("The region min for %i is %i\n", nextRegionToSet - 1, regionMin[nextRegionToSet -1 ]);
      }
    }
  }


  // int posCtr = 0;
  int* matchesArray = (int*)malloc(ctr*sizeof(int));
//  printf("Match count: %i, highestSeen: %i\n", matchCount, highestSeen);
  
  pthread_t thread[threadCount];
  searchArea area[threadCount];
  for (i = 0; i < threadCount; i++) {
    area[i].matchesArray = matchesArray;
    area[i].compareHash = compareHashMap;
    area[i].left = regionMin[i];
    if (i < threadCount - 1)
      area[i].right = regionMin[i + 1];
    else
      area[i].right = highestSeen;
    area[i].ctrStartPos = regionMinCount[i];
    printf("Thread %i Searching from (%i) %i to %i\n", i, regionMin[threadCount], area[i].left, area[i].right);
    if (i > 0)
      area[i].left++;

    pthread_create(&thread[i], NULL, threadHandler, &area[i]);
  }

  // int regionCount[threadCount];
  // int regionLeftPos[threadCount], regionRightPos[threadCount];
  // int regionStartPos[threadCount];
  // for (i = 0; i < threadCount; i++) {
  //   regionCount[i] = 0;
  //   regionStartPos[i] = 0;
  // }

  // for (i = 0; i < threadCount; i++) {
  //   regionLeftPos[i] = (i * highestSeen) / threadCount;
  //   if (i > 0) 
  //     regionLeftPos[i]++;
  //   regionRightPos[i] = ((i + 1) * highestSeen) / threadCount;
  //   printf("Region %i, leftPos: %i, rightPos: %i\n", i, regionLeftPos[i], regionRightPos[i]);
  // }

  // for (i = 0; i < highestSeen; i++) {
  //   if (testBit(i, compareHashMap)) {
  //     for (j = 0; j < threadCount; j++) {
  //       if (i >= regionLeftPos[j] && i <= regionRightPos[j]) { //can optimise here lumping all in final segment together ie skip loop
  //         regionCount[j]++;
  //        // printf("Added %i between %i and %i\n", i, regionLeftPos[j], regionRightPos[j]);
  //         break;
  //       }
  //     }
  //   }
  // }

  // regionStartPos[0] = 0;
  // for (i = 1; i < threadCount; i++) {
  //   regionStartPos[i] = regionCount[i - 1] + regionStartPos[i - 1];
  //   printf("region %i startPos: %i\n", i, regionStartPos[i]);
  // }

  diff = clock() - start;
  msec = diff * 1000 / CLOCKS_PER_SEC;
  printf("creating linked list took: %dms\n", msec);
  start = clock();


//  int posCtr = 0;
//  int* matchesArray = (int*)malloc(matchCount*sizeof(int));
//  printf("Match count: %i, highestSeen: %i\n", matchCount, highestSeen);
  
/*  pthread_t thread[threadCount];
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
  } */

  for (i = 0; i < threadCount; i++) {
    pthread_join(thread[i], NULL);
  }

//  printf("Match Array:\n");
  for (i = 0; i < ctr; i++) {
    printf("%i: %i\n", i, *(matchesArray + i));
    fprintf(output, "%i\n", *(matchesArray + i));
  }

  diff = clock() - start;
  msec = diff * 1000 / CLOCKS_PER_SEC;
  printf("list to array took: %dms\n", msec);
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
  compare(array1, num1, array2, num2, atoi(argv[4]), fp);
  // sort(fp, matchList, atoi(argv[4]));
  fclose(fp);

  return 0;
}
