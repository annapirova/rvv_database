#include <riscv-vector.h>

#define ARRAY_LEN 100

void test()
{
  size_t bs_m1 = vsetvl_e32m1(ARRAY_LEN);
  size_t bs_m2 = vsetvl_e32m2(ARRAY_LEN);
  size_t bs_m4 = vsetvl_e32m4(ARRAY_LEN);
  size_t bs_m8 = vsetvl_e32m8(ARRAY_LEN);
}

int main() 
{
  test();
  return 0; 
}