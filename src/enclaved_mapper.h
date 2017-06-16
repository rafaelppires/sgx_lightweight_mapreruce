#ifndef _ENCLAVED_MAPPER_H_
#define _ENCLAVED_MAPPER_H_

#include <worker_protocol.h>
#include <mrprotocol.h>
#include <enclave_mapreduce.h>
#include <map>
#include <vector>

class EnclavedMapper : public WorkerProtocol {
public:
    EnclavedMapper();
    void income_data( MsgType, const char *, size_t );
    State state() { return state_; }
    int reducers_count() { return reducer_count_; }
    void store_output( const std::string &, const std::string & );
private:
    void finish_round();
    void flush_output();

    State state_;
    std::map< std::string, std::vector<std::string> > outbuff_;
    int reducer_count_;
    const char *func_ = "map";
    size_t round_;
};

#endif

