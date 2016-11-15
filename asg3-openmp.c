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

int initHashMap[3125000]; //use a huge bit array to hash values - abusing global value initialisation to 0 here; bit operation helper functions are below
int compareHashMap[3125000];

void setBit(int k, int hashMap[])
{
  hashMap[k/32] |= 1 << (k%32);
}

void clearBit(int k, int hashMap[])                
{
  hashMap[k/32] &= ~(1 << (k%32));
}

int testBit(int k, int hashMap[])
{
  return ((hashMap[k/32] & (1 << (k%32))) != 0);     
}

void readIntoArray(int* matchesArray, int* compareHash, int left, int right, int ctrStartPos) {
  int i, insertPos;

  for (i = left; i <= right; i++) {
    if (testBit(i, compareHash)) {
      *(matchesArray + ctrStartPos++) = i;
    }
  }
}

void compareAndOutput(int* list1, int list1size, int* list2, int list2size, int threadCount, FILE* output) {
  //In order to compare the 2 files I first mark each distinct number as "seen" in a bit array, then run thru the 2nd list,
  //seeing if that value was "seen" in the first list and marking it in the 2nd bit array. The "Sort" and output operation is as simple as iterating 
  //thru the 2nd bit array and outputting any values that are marked as "seen" (For scaling purposes I have split this across threads). In a sparsely
  //paired list this would be far from optimal, but for denser lists the O(n) time makes a difference.

  int i, j;
  for (i = 0; i < list1size; i++) {
   setBit(*(list1 + i), initHashMap);
  }

  int ctr = 0, highestSeen = 0, lowestSeen = 100000001;

  for (i = 0; i < list2size; i++) {
    if (testBit(*(list2 + i), initHashMap) && !testBit(*(list2 + i), compareHashMap)) { //short circuit on the && operator useful here
      setBit(*(list2 + i), compareHashMap);
      ctr++;
      if (*(list2 + i) > highestSeen) 
        highestSeen = *(list2 + i);
      if (*(list2 + i) < lowestSeen)
        lowestSeen = *(list2 + i);
    }
  }

  //This next section is merely for splitting the load evenly for parallelisation; in an evenly distributed pairs list this would be a waste of time
  //however if all pairs were for example skewed to the lower end this would prevent all the work going to one thread

  int regionMinCount[threadCount], regionMin[threadCount], regionMaxPos[threadCount];
  for (i = 0; i < threadCount; i++) {
    regionMinCount[i] = (ctr * i) / threadCount;
    if (i > 0)
      regionMinCount[i]++;
  }

  int nextRegionToSet = 1, posCtr = 0;
  regionMin[0] = 0;
  for (i = lowestSeen; i < highestSeen; i++) {
    if (testBit(i, compareHashMap)) {
      posCtr++;
      if (posCtr == regionMinCount[nextRegionToSet]) {
        regionMin[nextRegionToSet++] = i;
      }
    }
  }

  //This section will scale out by a constant factor - though it makes up the minority of processing time unfortunately 
  //meaning it will likely fail the scaleout tests given I/O time uncertainty

  int* matchesArray = (int*)malloc(ctr*sizeof(int));
  
  int left, right;
  #pragma omp parallel for
  for (i = 0; i < threadCount; i++) { 
    left = regionMin[i];
    if (i > 0)
      left++;
    if (i < threadCount - 1)
      right = regionMin[i + 1];
    else
      right = highestSeen;
    readIntoArray(matchesArray, compareHashMap, left, right, regionMinCount[i]);
  }

  for (i = 0; i < ctr; i++) {
    fprintf(output, "%i\n", *(matchesArray + i));
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
  compareAndOutput(array1, num1, array2, num2, atoi(argv[4]), fp);
  fclose(fp);

  return 0;
}
