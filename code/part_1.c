#include <stdio.h>
#include <stdlib.h>

#define SUCCESS 0
#define FAILURE 1

// Function prototypes
void greet_user(void);

int main(void) {
  // Greet the user
  greet_user();

  // Program logic here
  printf("This is an idiomatic C program.\n");

  return SUCCESS;
}

// Function definitions
void greet_user(void) {
  printf("Welcome to the program!\n");
}