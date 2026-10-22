#include <immintrin.h>

void test()
{
  __m512i zmm = _mm512_setzero_si512();
}

int main() 
{
  test();
  return 0; 
}