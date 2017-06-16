#ifndef _ENCLAVED_COMMON_H_
#define _ENCLAVED_COMMON_H_

extern "C" {
    void printf(const char *fmt, ...);
}

enum State {
    EXPECT_CODE,
    EXPECT_DATA,
    WAIT_NEXT
};

#include <mrprotocol.h>
#include <string>
#include <stdio.h>
inline std::string pubheader( const std::string &id, const KeyValue &kv ) {
    char buff[1000], tmp[100];

    KeyValue::const_iterator it = kv.begin(), end = kv.end();
    snprintf( buff, sizeof(buff), "0 %s P", id.c_str() );
    for(; it != end; ++it ) {
        snprintf( tmp, sizeof(tmp), " i %s %s",
                                        it->first.c_str(), it->second.c_str() );
        strncat( buff, tmp, sizeof(buff)-1-strlen(buff) );
    }
    return buff;
}

inline std::string subheader( const std::string &id, const KeyValue &kv ) {
    char buff[1000], tmp[100];

    KeyValue::const_iterator it = kv.begin(), end = kv.end();
    snprintf( buff, sizeof(buff), "0 %s S", id.c_str() );
    for(; it != end; ++it ) {
        snprintf( tmp, sizeof(tmp), " i %s = %s", 
                                        it->first.c_str(), it->second.c_str() );
        strncat( buff, tmp, sizeof(buff)-1-strlen(buff) );
    }
    return buff;
}

#endif

