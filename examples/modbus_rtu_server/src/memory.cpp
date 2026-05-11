#include "memory.hpp"

extern "C"
{

  void *
  memset(void *s, int c, size_t n)
  {
    unsigned char *p = static_cast<unsigned char *>(s);
    while (n--)
      *p++ = static_cast<unsigned char>(c);
    return s;
  }

  void *
  memcpy(void *dest, const void *src, size_t n)
  {
    unsigned char *d = static_cast<unsigned char *>(dest);
    const unsigned char *s = static_cast<const unsigned char *>(src);
    while (n--)
      *d++ = *s++;
    return dest;
  }

  int
  memcmp(const void *s1, const void *s2, size_t n)
  {
    const unsigned char *p1 = static_cast<const unsigned char *>(s1);
    const unsigned char *p2 = static_cast<const unsigned char *>(s2);
    while (n--)
    {
      if (*p1 != *p2)
        return static_cast<int>(*p1) - static_cast<int>(*p2);
      ++p1;
      ++p2;
    }
    return 0;
  }

  void *
  memmove(void *dest, const void *src, size_t n)
  {
    unsigned char *d = static_cast<unsigned char *>(dest);
    const unsigned char *s = static_cast<const unsigned char *>(src);
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
