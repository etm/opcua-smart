#include "strnautocat.h"

char *strnautocat(char *str, char *s2, size_t num) {
  int len = 0;
  char *s;
  if (str == NULL) {
    str = (char *)malloc(sizeof(*str));
    str[0] = '\0';
  } else {
    len = strlen(str);
  }
  len += num + 1 * sizeof(*s2);
  s = realloc(str, len);
  strncat(s, s2, num);
  return s;
}

