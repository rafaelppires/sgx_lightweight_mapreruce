#ifndef _ENCLAVED_REDUCER_H_
#define _ENCLAVED_REDUCER_H_

#include <worker_protocol.h>
#include <mrprotocol.h>
#include <enclave_mapreduce.h>
#include <string>
#include <vector>
#include <map>

class EnclavedReducer : public WorkerProtocol {
public:
    EnclavedReducer();
    void income_data( MsgType, const char *, size_t  );
private:
    void start_process();
    std::string orderjson( const std::string &key );

    State state_;
    int mapper_count_, mappers_done_;
    std::map< std::string, std::vector<std::string> > data_buff_;
    const char *func_ = "reduce";
    size_t round_;
};

#endif

