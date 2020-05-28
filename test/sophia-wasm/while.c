#include <stdio.h>

int main(int argc, char ** argv){
  int a = 1;
  int b = 3;
  int d = a + b;
  while (d > 0) {
    if (a >= b) {
      a = a-1;
    } else {
      while (a<b) {
        a = a+1;
        b = b-1;
      }
    }
  }
  printf("%d", d);
}
