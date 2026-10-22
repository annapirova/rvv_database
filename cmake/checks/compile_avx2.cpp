#include <immintrin.h>

void test()
{
  __m256i ymm = _mm256_setzero_si256();
}

int main() 
{
  test();
  return 0; 
}