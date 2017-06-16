#ifndef _WORKER_PROTOCOL_H_
#define _WORKER_PROTOCOL_H_

#include <enclaved_common.h>
#include <sgx_utiles.h>
#include <enclave_mapreduce_t.h>

class WorkerProtocol {
public:
    WorkerProtocol() : encrypt_(false) {
        codetype_ = datatype_ = resulttype_ = INVALID_TYPE;
    }

    void initialsub(); 
    void startsession( std::string s );
    void outputpair( const char *dst, const char *key, const char *value ); 
    void actual_output( char *buff, size_t len,
                        const std::string &header, const std::string &payload);

    void setmap( const std::string &id, bool enc ) {
        id_ = id;
        codetype_   = MAP_CODETYPE;
        datatype_   = MAP_DATATYPE;
        resulttype_ = REDUCE_DATATYPE;
        encrypt_ = enc;
    }

    void setreduce( const std::string &id, bool enc ) {
        id_ = id;
        codetype_   = REDUCE_CODETYPE;
        datatype_   = REDUCE_DATATYPE;
        resulttype_ = RESULT_DATATYPE;
        encrypt_ = enc;
    }

private:
    bool encrypt_;
    std::string sid_, id_;
    MsgType codetype_, datatype_, resulttype_;
};

#endif

