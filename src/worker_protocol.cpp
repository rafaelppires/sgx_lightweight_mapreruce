#include <worker_protocol.h>

extern "C" {
extern size_t luamem_used;
}
//------------------------------------------------------------------------------
void WorkerProtocol::initialsub() {
    char buff[200];
    KeyValue kv; // auxiliary to build headers

    // Register subscription of job ads made by clients
    snprintf(buff, sizeof(buff), "%d", JOB_ADVERTISE);
    kv["type"] = buff;

    std::string header = subheader(id_,kv);
    size_t sz = std::min(header.size(),sizeof(buff));
    if( encrypt_ ) {
        encrypt( header.c_str(), buff, sz );
        ocall_outputdata( buff, sz, "", 0, luamem_used );
    } else
        ocall_outputdata( header.c_str(), header.size(), "", 0, luamem_used );
}

//------------------------------------------------------------------------------
void WorkerProtocol::startsession( std::string s ) {
    char buff[200];
    sid_ = s;
    KeyValue kv;

    snprintf(buff, sizeof(buff), "%d", codetype_);
    kv["type"] = buff;
    kv["sid"] = sid_;
    std::string code_req = subheader(id_,kv);

    snprintf(buff, sizeof(buff), "%d", datatype_);
    kv["type"] = buff;
    kv["dst"] = id_.substr(1);
    std::string data_req = subheader(id_,kv);

    kv.erase("dst");
    snprintf(buff, sizeof(buff), "%d", JOB_REQUEST);
    kv["type"] = buff;

    std::string header  = pubheader(id_,kv),
        payload =
            kv["type"] + pload_sep + code_req + pload_sep + data_req;

    actual_output( buff, sizeof(buff), header, payload );
}

//------------------------------------------------------------------------------
void WorkerProtocol::outputpair( const char *dst, const char *key, 
                                 const char *value ) {
    char buff[10000];
    KeyValue kv;

    snprintf(buff, sizeof(buff), "%d", resulttype_);
    kv["type"] = buff;
    kv["sid"] = sid_;
    if( resulttype_ != RESULT_DATATYPE )
        kv["dst"] = dst;

    snprintf( buff, sizeof(buff), "%d%c%s%c%s",
            resulttype_, pload_sep, key, pload_sep, value );

    std::string header  = pubheader(id_,kv),
        payload = buff;
    actual_output( buff, sizeof(buff), header, payload );
}


//------------------------------------------------------------------------------
void WorkerProtocol::actual_output( char *buff, size_t len,
                        const std::string &header, const std::string &payload) {
    size_t hfsz = len/2,
    hdsz = std::min(hfsz,header.size()),
    plsz = std::min(hfsz,payload.size());
    if( encrypt_ ) {
        //printf("<%s><%s>\n", header.c_str(), payload.c_str());
        encrypt( header.c_str(),  buff,      hdsz );
        encrypt( payload.c_str(), buff+hfsz, plsz );
        ocall_outputdata( buff, hdsz, buff+hfsz, plsz, luamem_used );
    } else {
        ocall_outputdata( header.c_str(), header.size(),
                payload.c_str(), payload.size(), luamem_used );
    }
}


//------------------------------------------------------------------------------

