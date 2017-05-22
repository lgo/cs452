#include <basic.h>
#include <jstring.h>

unsigned int jstrlen(char *c) {
  int len = 0;
  while(c[len] != 0) len++;
  return len;
}

bool jstrcmp(char *str1, char *str2) {
  int i = 0;
  while (true) {
    if (str1[i] == '\0' && str2[i] == '\0') return true;
    else if (str1[i] == str2[i]) i++;
    else return false;
  }
  return false;
}

void jstrappend(char *str1, char *str2, char *buf) {
  // FIXME: assert size doesn't overflow buffer
  if (buf == str1) {
    // optimization if str1 is also a buffer with space
    while (*buf != '\0') buf++;
  } else {
    while (*str1 != '\0') {
      *buf = *str1;
      buf++;
      str1++;
    }
  }
  while (*str2 != '\0') {
    *buf = *str2;
    buf++;
    str2++;
  }
  *buf = '\0';
}

void jstrappendc(char *str1, char c, char *buf) {
  // FIXME: assert size doesn't overflow buffer
  if (buf == str1) {
    // optimization if str1 is also a buffer
    while (*buf != '\0') buf++;
  } else {
    while (*str1 != '\0') {
      *buf = *str1;
      buf++;
      str1++;
    }
  }
  *buf = c;
  buf++;
  *buf = '\0';
}

char **jstrsplit(char *str, char delimiter) {
  // int i;
  // unsigned int len = strlen(str);
  //
  // // count for how many new strings there will be
  // int str_count = str[0] == delimiter ? 0 : 1;
  // bool in_delimiter = str[0] == delimiter;
  // for (i = 1; i < len; i++) {
  //   if (str[i] == delimiter && in_delimiter) continue;
  //   if (str[i] != delimiter && !in_delimiter) continue;
  //   str_count++;
  //   in_delimiter = str[i] == delimiter;
  // }
  //
  // char **strings = jalloc(sizeof(char **) * str_count);
  //
  // // count the length of each string
  // in_delimiter = str[0] == delimiter;
  // int curr_strlen = str[0] == delimiter ? 0 : 1;
  // for (i = 1; i < len; i++) {
  //   if (str[i] == delimiter && in_delimiter) continue;
  //   if (str[i] != delimiter && !in_delimiter) continue;
  //   if (str[i] != delimiter && !in_delimiter) continue;
  //   str_count++;
  //   in_delimiter = str[i] == delimiter;
  // }
  // return strings;
  return NULL;
}

int jstrsplit_count(char *str, char delimiter) {
  int i;
  unsigned int len = jstrlen(str);
  // edge case, empty string
  if (len == 0) return 0;

  // count for how many new strings there will be
  int str_count = str[0] == delimiter ? 0 : 1;
  bool in_delimiter = str[0] == delimiter;
  for (i = 0; i < len; i++) {
    if (str[i] != delimiter && in_delimiter) str_count++;
    in_delimiter = str[i] == delimiter;
  }

  return str_count;
}


int jstrsplit_buf(char *str, char delimiter, char *buffer, int buffer_size) {
  int i;
  unsigned int len = jstrlen(str);
  // FIXME: this will fail if the buffer is > 256, due to char size limits
  // TODO: assert buffer_size is enough
  // by worst case analysis, you need (strlen + 1) * 1.5 + 1 characters
  int str_count = jstrsplit_count(str, delimiter);

  // put fillers into counts
  for (i = 0; i < str_count; i++) {
    buffer[i] = 0x02;
  }
  // put NULL for char ** end
  buffer[str_count] = '\0';
  // edge case, empty string
  if (str_count == 0) return 0;

  // Set the split characters into the buffer, with offsets in the beginning
  char *current_str = buffer + str_count + 1;
  // start it in the delimiter so it always initializes
  bool in_delimiter = true;
  // start buffer back one, so it initializes with an increment correctly
  if (in_delimiter) buffer--;
  int idx = 0;
  for (i = 0; i < len; i++) {
    if (str[i] == delimiter && in_delimiter) continue;
    else if (str[i] != delimiter && in_delimiter) {
      // move out of delimiter
      // shift current char **
      buffer++;
      // set char **
      *buffer = idx + str_count + 1;
      *current_str = str[i];
    } else if (str[i] == delimiter && !in_delimiter) {
      // move into delimiter
      // add zero byte
      *current_str = '\0';
      debugger();
    } else if (str[i] != delimiter && !in_delimiter) {
      // in string
      // set string value
      *current_str = str[i];
    }
    current_str++;
    idx++;
    in_delimiter = str[i] == delimiter;
  }
  // add zero byte for last char, edge case if there is no final delim
  *current_str = '\0';

  return 0;
}