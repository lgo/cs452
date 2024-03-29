#pragma once

#include <ts7200.h>
#include <stdarg.h>

#define ON  1
#define OFF 0

int bwsetfifo( int channel, int state );

int bwsetspeed( int channel, int speed );

int bwputc( int channel, char c );

// Put multiple of a character, useful for padding
int bwputmc( int channel, char c , int n );

int bwgetc( int channel );

int bwputx( int channel, char c );

int bwputstr( int channel, const char *str );

int bwputr( int channel, unsigned int reg );

void bwputw( int channel, int n, char fc, char *bf );

void bwprintf( int channel, char *format, ... ) __attribute__ ((format (printf, 2, 3)));
