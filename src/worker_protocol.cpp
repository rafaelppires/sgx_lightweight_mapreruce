#define ENABLE_SGX
#include <worker_protocol.h>

extern "C" {
extern size_t luamem_used;
}

static std::string hkey = "_header_key_", pkey = "_header_key_";
//------------------------------------------------------------------------------
void WorkerProtocol::initialsub() {
    KeyValue kv; // auxiliary to build headers

    // Register subscription of job ads made by clients
    char buff[3];
    snprintf(buff, sizeof(buff), "%d", JOB_ADVERTISE);
    kv["type"] = buff;

    std::string header = subheader(id_,kv);
    if( encrypt_ )
        Crypto::encrypt_aes_inline( hkey, header );

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

    actual_output( header, payload );
}

//------------------------------------------------------------------------------
void WorkerProtocol::outputpair( const char *dst, 
                                 const char *key, const char *value ) {
    KeyValue kv;

    size_t sz = strlen(key) + strlen(value) + 1000;;
    char *buff = new char[ sz ];
    snprintf(buff, sizeof(buff), "%d", resulttype_);
    kv["type"] = buff;
    kv["sid"] = sid_;
    if( resulttype_ != RESULT_DATATYPE )
        kv["dst"] = dst;

    snprintf( buff, sz, "%d%c%s%c%s",
              resulttype_, pload_sep, key, pload_sep, value );

    std::string header  = pubheader(id_,kv),
                payload = buff;
    actual_output( header, payload );
    delete[] buff;
}


//------------------------------------------------------------------------------
void WorkerProtocol::actual_output( std::string &header, std::string &payload) {
    if( encrypt_ ) {
        Crypto::encrypt_aes_inline( hkey, header );
        Crypto::encrypt_aes_inline( pkey, payload );
    }
    ocall_outputdata( header.c_str(), header.size(),
                      payload.c_str(), payload.size(), luamem_used );
}


//------------------------------------------------------------------------------

