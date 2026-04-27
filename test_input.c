#include <stdio.h>
int main() {
  int x;
  if (scanf("%d", &x) == 1) {
    printf("You entered: %d\n", x);
  } else {
    printf("No input\n");
  }
  return 0;
}
