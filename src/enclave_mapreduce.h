#ifndef _ENCLAVE_MAPREDUCE_H_
#define _ENCLAVE_MAPREDUCE_H_

#include <string.h>

void process_code( const char *buff, size_t len, const char *funcname );
void process_data( const char *k, const char *v, const char *funcname );
void apply_combine( const char *key, const char *value);

#endif

