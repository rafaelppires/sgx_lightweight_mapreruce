#ifndef _SCBRMSG_h_
#define _SCBRMSG_h_

#include <map>
#include <vector>
#include <sstream>

typedef std::vector< std::string > StringVector;
inline void split( const std::string &s, const char* delim,
                   StringVector & v ){
    char * dup = strdup(s.c_str());
    char * token = strtok(dup, delim);
    while(token != NULL){
        v.push_back(std::string(token));
        token = strtok(NULL, delim);
    }
    free(dup);
}

typedef std::map<std::string,std::string> KeyValue;
inline std::string pubheader( const KeyValue &kv ) {
    KeyValue::const_iterator it = kv.begin(), end = kv.end();
    std::stringstream ss;
    ss << "0 0 P";
    for(; it != end; ++it )
        ss << " i " << it->first << " " << it->second;
    return ss.str();
}

inline std::string subheader( const std::string &id, const KeyValue &kv ) {
    KeyValue::const_iterator it = kv.begin(), end = kv.end();
    std::stringstream ss;
    ss << "0 " << id << " S";
    for(; it != end; ++it )
        ss << " i " << it->first << " = " << it->second;
    return ss.str();
}


#endif 

