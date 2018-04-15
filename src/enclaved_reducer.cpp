#define ENABLE_SGX
#include <enclaved_reducer.h>
#include <enclave_mapreduce_t.h>
#include <set>
#include <algorithm>

//------------------------------------------------------------------------------
EnclavedReducer::EnclavedReducer() : state_(EXPECT_CODE), mapper_count_(0),
                                     mappers_done_(0), round_(1) { }

//------------------------------------------------------------------------------
std::string EnclavedReducer::orderjson( const std::string &key ) {
    std::string str = key;
    char sep = ',';
    str.erase(std::remove(str.begin(), str.end(), '{'), str.end());
    str.erase(std::remove(str.begin(), str.end(), '}'), str.end());
    std::string::iterator it = str.begin(), end = str.end(), tmp;

    std::multiset<std::string> order;
    size_t n = 0, t;
    while( n != std::string::npos ) {
        t = str.find( sep, n );
        order.insert(str.substr( n,  t - n ));
        n = t == std::string::npos ? t : t + 1;
    }

    std::string ret = "{";
    for( auto &x : order ) ret += (ret == "{" ? "" : ",") + x;
    return ret + '}';
}

//------------------------------------------------------------------------------
void EnclavedReducer::income_data( MsgType type, const char *str, size_t len ) {
    //printf("<%s>\n", str);
    const char *k = strchr( str, pload_sep ), 
               *err = "Protocol Error\n";
    if( k == NULL ) { printf("Invalid format!\n"); 
abort();
return; }

    std::string key = std::string( str, k-str );
    std::string value( k+1 );

    switch( state_ ) {
    case EXPECT_CODE:
    {
        if( type != REDUCE_CODETYPE ) goto proterror;
        size_t n, i = 0;
        if((n = value.find( pload_sep )) != std::string::npos ) {
            mapper_count_ = atoi( std::string(value, i, n).c_str() );
            if( mapper_count_ < 1 || mapper_count_ > 999 ) {
                printf(err);
                return;
            }
        } else goto proterror;
        std::string code(value, n+1);
        process_code( code.c_str(), code.size(), func_ );
        state_ = EXPECT_DATA;
        break;
    }
    case EXPECT_DATA:
        if( type != REDUCE_DATATYPE ) goto proterror; 

        if( key.empty() && value.empty() ) { 
            if( ++mappers_done_ == mapper_count_ ) {
                start_process();
            }
        } else {
            key = orderjson( key );
            data_buff_[ key ].push_back( value );
        }
        break;
    case WAIT_NEXT:
        if( type == REDUCE_DATATYPE ) {
            mappers_done_ = 0;
            data_buff_.clear();

            printf("-> Reduce: Round %d\n", ++round_ );
            state_ = EXPECT_DATA;
            income_data( type, str, len );
        }
        break;
    default:
        printf("REDUCER Unknown state: %d\n", state_);
    };
    return;

proterror:
    printf(err);
}

//------------------------------------------------------------------------------
void EnclavedReducer::start_process() {
    std::string value;
    for( auto const &data : data_buff_ ) {
        for( auto const &v : data.second ) value += v + ",";
        process_data( data.first.c_str(), 
                 ("[" + value.substr(0,value.size()-1) + "]").c_str(), func_ ); 
        value = "";
    }

    outputpair( "-","","" ); 
    state_ = WAIT_NEXT;
}

//------------------------------------------------------------------------------

