/*
* CSCI3150 Assignment 3 - Implement pthread and openmp program
* Feel free to modify the given code.
*
*/

/* Header Declaration */
#include <stdio.h>
#include <omp.h>
#include <string.h>

/* Function Declaration */

void compare(FILE *fp, FILE *input1, FILE *input2) {
  int *input1size;
  fscanf(input1, "%i", input1size);
  printf("%d integers\n", *input1size);
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

    compare(fp, inputf1, inputf2);

    fclose(fp);
    fclose(inputf1);
    fclose(inputf2);

    return 0;
}
