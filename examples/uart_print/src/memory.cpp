#include "memory.hpp"

extern "C"
{

  void *
  memset(void *s, int c, size_t n)
  {
    unsigned char *p = (unsigned char *)s;
    while (n--)
      *p++ = (unsigned char)c;
    return s;
  }

  void *
  memcpy(void *dest, const void *src, size_t n)
  {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    while (n--)
      *d++ = *s++;

    return dest;
  }

  int
  memcmp(const void *s1, const void *s2, size_t n)
  {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;

    while (n--)
    {
      unsigned char a = *p1++;
      unsigned char b = *p2++;
      if (a != b)
        return (int)a - (int)b;
    }
    return 0;
  }

  void *
  memmove(void *dest, const void *src, size_t n)
  {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    if (d == s || n == 0)
      return dest;

    if (d < s)
    {
      while (n--)
        *d++ = *s++;
    }
    else
    {
      d += n;
      s += n;
      while (n--)
        *--d = *--s;
    }
    return dest;
  }

}; // extern "C"
