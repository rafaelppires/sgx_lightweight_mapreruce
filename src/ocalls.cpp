#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
void ocall_outputerror( const char *str );
void ocall_print( const char* str );
#ifdef __cplusplus
}
#endif

//------------------------------------------------------------------------------
void ocall_outputerror( const char *str ) {
    printf("Error: %s\n", str);
}

//------------------------------------------------------------------------------
void ocall_print( const char* str ) {
    printf("\033[96m%s\033[0m", str);
}

