#include <basic.h>

unsigned int jstrlen(char *c);

bool jstrcmp(char *str1, char *str2);
void jstrappend(char *str1, char *str2, char *buf);
void jstrappendc(char *str1, char c, char *buf);


// Count amount of substrings
// NOTE: internal
int jstrsplit_count(char *str, char delimiter);

char **jstrsplit(char *str, char delimiter);

// Returns status:
// 0 => OK
int jstrsplit_buf(char *str, char delimiter, char *buffer, int buffer_size);