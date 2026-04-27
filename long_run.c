#include <stdio.h>
int main() {
  volatile unsigned long i = 0;
  while (1) {
    i++;
    if ((i % 100000000) == 0) printf("tick %lu\n", i);
  }
  return 0;
}
