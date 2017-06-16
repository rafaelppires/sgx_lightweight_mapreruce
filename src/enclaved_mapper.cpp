#include <enclave_mapreduce.h>
#include <enclave_mapreduce_t.h>
#include <enclaved_mapper.h>
#include <stdio.h>
#include <string>

//------------------------------------------------------------------------------
EnclavedMapper::EnclavedMapper() : state_(EXPECT_CODE), reducer_count_(0), 
                                   round_(1) { }

//------------------------------------------------------------------------------
void EnclavedMapper::income_data( MsgType type, const char *str, size_t len ) {
    //printf("%s\n", str);
    const char *k = strchr( str, pload_sep ), 
               *err = "Protocol Error\n";
    if( k == NULL ) { printf("Invalid format!\n"); return; }

    std::string key( str, k-str );
    std::string value( k+1 );

    switch( state_ ) {
    case EXPECT_CODE:
    {
        if( type != MAP_CODETYPE ) goto proterror;
        size_t n, i = 0;
        if((n = value.find( pload_sep )) != std::string::npos ) {
            reducer_count_ = atoi( std::string(value, i, n).c_str() );
            if( reducer_count_ < 1 || reducer_count_ > 999 ) {
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
        if( type != MAP_DATATYPE ) goto proterror;
        if( key.empty() && value.empty() )
            finish_round();
        else
            process_data( key.c_str(), value.c_str(), func_ );
        break;
    case WAIT_NEXT:
        if( type == MAP_DATATYPE ) {
            printf("-> Map: Round %d\n", ++round_ );
            state_ = EXPECT_DATA;
            income_data( type, str, len );
        } else goto proterror;
        // Can do something else (cleanup?)
        break;
    default:
        printf("MAPPER Unknown state: %d\n", state_);
    };
    return;

proterror:
    printf(err);
}

//------------------------------------------------------------------------------
void EnclavedMapper::finish_round() {
    flush_output();

    char dst[10];
        printf("** ");
    for( int i = 0; i < reducer_count_; ++i ) {
        snprintf( dst, sizeof(dst), "%d", i );
        outputpair( dst, "", "" );
        printf("** %s ", dst);
    }
        printf("**\n");

    state_ = WAIT_NEXT;
}

//------------------------------------------------------------------------------
void EnclavedMapper::store_output( const std::string &k,
                                   const std::string &v ) {
    outbuff_[k].push_back(v);
}
//------------------------------------------------------------------------------
void EnclavedMapper::flush_output() {
    //printf("Found %lu group of keys\n", outbuff_.size());
    for( auto &i : outbuff_ ) {
        std::string aggregate;
        aggregate += '[';
        for( auto &j : i.second ) {
            aggregate += j.c_str();
            aggregate += ',';
        }
        aggregate[ aggregate.size()-1 ] = ']';
        apply_combine( i.first.c_str(), aggregate.c_str() );
    }

    outbuff_.clear();
}

//------------------------------------------------------------------------------
