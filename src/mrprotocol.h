#ifndef _PROTOCOL_H_DEFINED_
#define _PROTOCOL_H_DEFINED_

#include <string>
#include <map>
typedef std::map<std::string,std::string> KeyValue;
static const char pload_sep = '|';

enum MsgType {
    INVALID_TYPE,    // 0
    MAP_CODETYPE,    // 1
    REDUCE_CODETYPE, // 2
    MAP_DATATYPE,    // 3
    REDUCE_DATATYPE, // 4
    RESULT_DATATYPE, // 5
    JOB_ADVERTISE,   // 6
    JOB_REQUEST      // 7
};

#endif

