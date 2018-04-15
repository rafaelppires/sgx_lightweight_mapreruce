#include <client_protocol.h>
#include <iostream>
#include <sstream>
#include <sgx_cryptoall.h>

static std::string password = "_header_key_";
//------------------------------------------------------------------------------
void ClientProtocol::init() {
    generatessid();

    // Creates session key (signed? athenticated? group key?) and publishes it
    KeyValue kv; // auxiliary to build headers

    std::cout << "M/R Session id: " << sid_ << "\n";
    kv.clear(); kv["type"] = std::to_string(JOB_ADVERTISE); 

    std::string header  = pubheader(kv), 
                payload = std::to_string(sid_);
    if( encrypt_ ) { 
        Crypto::encrypt_aes_inline(password, header);
        Crypto::encrypt_aes_inline(password, payload);
    }
    Message sidpub( true, header, payload );
    pipe_.send( sidpub );

    // Register subscription to receive job requestts 
    kv["type"] = std::to_string(JOB_REQUEST);
    kv["sid"]  = std::to_string(sid_);

    header = subheader(id_,kv);
    if( encrypt_ ) { 
        Crypto::encrypt_aes_inline(password, header);
    }
    Message mjob_req( true, header, "" );
    pipe_.send( mjob_req );

    // Register subscription for result data
    kv["type"] = std::to_string(RESULT_DATATYPE);
    header = subheader(id_,kv);
    if( encrypt_ ) { 
        Crypto::encrypt_aes_inline(password, header);
    }
    Message mresult_req( true, header, "" );
    pipe_.send( mresult_req );
}

//------------------------------------------------------------------------------
void ClientProtocol::generatessid() {
    srand(time(0)); // dummy! TODO: change it
    sid_ = rand();
}

//------------------------------------------------------------------------------
void ClientProtocol::extractwt( const std::string &s, std::string &w, int &t ) {
    // TODO: sanity checks
    StringVector fields;
    split( s, " ", fields );
    w = fields[1];
    for(size_t i = 4; i < fields.size(); i += 4)
        if( fields[i] == "type" ) { t = std::stoi(fields[i+2]); return; }
}


double diagonal_range_ = 0;
//------------------------------------------------------------------------------
bool ClientProtocol::handle_msg( const std::string &message ) {
    std::string msg = encrypt_ ? Crypto::decrypt_aes( password,message ) : message;

    bool ret = false;
    StringVector splited; char sep[] = {pload_sep, 0};
    split( msg, sep, splited );
    if( splited.empty() ) return false;

    switch( std::stoi(splited[0]) ) {
    case JOB_REQUEST:
    {
        for(size_t i = 1; i < splited.size(); ++i) { 
            std::string worker; int type;
            extractwt( splited[i], worker, type );
            if( type == MAP_CODETYPE || type == MAP_DATATYPE ) {
                mappers_.insert( worker );
            } else if( type == REDUCE_CODETYPE || type == REDUCE_DATATYPE ) {
                reducers_.insert( worker );
            } else {
                //std::cout << "duh?" << type << '\n';
            }
            // fwd in behalf of workers their subscriptions for code and data
            if( encrypt_ ) {
                Crypto::encrypt_aes_inline(password,splited[i]); 
            }
            Message m( true, splited[i], "" );
            pipe_.send( m );
            ret = true;
        }
        break;
    }
    case RESULT_DATATYPE:
    {
        //std::cout << msg << "\n"; 
        if( splited.size() == 1 && 
                ++reducers_done_ == reducers_.size() ) {

            if( app_type_ == KMEANS ) {
                double t = round_stats_.end();
                std::cout << "(" <<  t << " sec) ";
                combine_results();
            } else if( app_type_ == WORDCOUNT ) {
                for( auto const &x : last_result_ )
                    std::cout << x.first << " " << x.second << "\n";
                exit(0);
            }

            reducers_done_ = 0;

            if( app_type_ == KMEANS ) { 
                kmeans_output(); // store previous calculated centers
                if ( kerror_ > (threshold_*diagonal_range_) ) {
                    fire_new_round();
                } else {
                    centersfile_.seekp(-2,centersfile_.cur);
                    centersfile_ << "\n]\n";
                    centersfile_.close();

                    std::cout << "k-means Threshold reached: " 
                              << (threshold_*diagonal_range_) << "\n";
                    round_stats_.print_summary( "k-means Round stats" );
                    printf("\n");
                    exit(0); 
                }
            }
        } else if( splited.size() == 3 )
            last_result_.insert( std::make_pair(splited[1],splited[2]) );
        break;
    }
    default:
        std::cout << msg;
    };
    std::cout.flush(); // pasin fflush(stdout);
    return ret;
}

//------------------------------------------------------------------------------
// In kmeans, there can be some centers unassigned to any data points
// When this happens, we do not get the same amount of centers as of the input
// Therefore, we must merge them
//------------------------------------------------------------------------------
void string_remove( std::string &str, const char* charsToRemove ) {
   for ( size_t i = 0; i < strlen(charsToRemove); ++i ) {
      str.erase( remove(str.begin(), str.end(), charsToRemove[i]), str.end() );
   }
}
//------------------------------------------------------------------------------
void ClientProtocol::combine_results() {
    std::string key, value; const char *rm = "{}\"", *sep = ",";
    std::vector<std::string> recalc;
    size_t n;

    for( const auto &x : last_result_ ) {
        value = x.second; string_remove(value,rm); n = value.find("id");
        recalc.push_back( value.substr( n, value.find(sep,n)-n ) );
    }

    for( const auto &x : previous_result_ ) {
        value = x.second; string_remove(value,rm); n = value.find("id"); 
        std::string id = value.substr( n, value.find(sep,n)-n );
        if( std::find( recalc.begin(), recalc.end(), id ) == recalc.end() )
            last_result_.insert( std::make_pair("-",x.second) ); 
    }

    kerror_ = kmeans_error();
    previous_result_ = last_result_;
    last_result_.clear();
}

//------------------------------------------------------------------------------
// Happens only once
//------------------------------------------------------------------------------
void ClientProtocol::fire_jobs( std::ifstream files[], std::string fnames[] ) {
    std::cout << "M/R Firing jobs to Mappers[ ";
    for( auto const &m : mappers_ ) std::cout << m << " ";
    std::cout << "] and Reducers[ ";
    for( auto const &r : reducers_ ) std::cout << r << " ";
    std::cout << "]\n";

    push_data( MAP_CODETYPE,    fnames[0], files[0] );
    push_data( REDUCE_CODETYPE, fnames[1], files[1] );

    if( app_type_ == KMEANS ) {
        pointsfile_.open("points.json");
        centersfile_.open("steps.json");
        std::string content;
        load_file( files[3], content );
        split( content, "\n", kmeans_centers_ );
    }

    round_stats_.begin();
    push_data( MAP_DATATYPE,    fnames[2], files[2] );
}

//------------------------------------------------------------------------------
void ClientProtocol::load_file( std::ifstream &file, std::string &content ) {
    file.seekg(0, std::ios::end); 
    content.reserve(file.tellg());
    file.seekg(0, std::ios::beg);
    content.assign((std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>());
}

//------------------------------------------------------------------------------
void ClientProtocol::push_data( MsgType type, const std::string &fname, 
                                std::ifstream &file ) {
    KeyValue kv;
    kv["sid"] = std::to_string(sid_);
    kv["type"] = std::to_string(type);

    std::stringstream plheader;
    plheader << type << pload_sep << fname << pload_sep;

    std::string content;
    load_file( file, content );

    switch( type ) {
    // Along with map code, we send the number of reducers
    // So they can shuffle the keys according to some common hash function
    case MAP_CODETYPE:
        plheader << reducers_.size() << pload_sep;
        break;
    // Along with reduce code, we send the number of mappers
    // So they know how many anwers they have to wait before start processing
    case REDUCE_CODETYPE:
        plheader << mappers_.size() << pload_sep;
        break;
    // split
    case MAP_DATATYPE:
    {
        split( content, "\n", mappers_data_ );

        if( app_type_ == KMEANS ) {
            kmeans_preprocess( mappers_data_ );
            kmeans_output(); // store beginning state
            plheader.clear(); plheader.str("");
            plheader << type << pload_sep << kmeans_clist() << pload_sep;
        } else {
            wcount_preprocess( mappers_data_ );
        }

        std::cout << "k-means " << time(0) << " Round 1 ";
        senddata_mappers( mappers_data_, kv, plheader.str() );
        return;
    }
    default:
        return;
    };

    std::string header  = pubheader(kv), 
                payload = plheader.str() + content;
    if( encrypt_ ) { 
        Crypto::encrypt_aes_inline(password,header);
        Crypto::encrypt_aes_inline(password,payload);
    }
    Message m( true, header, payload );
    pipe_.send( m );
}

//------------------------------------------------------------------------------
// psheader: Prefilled Pub/Sub header
// plheader: Payload header
// Is called every iteration of k-means
//------------------------------------------------------------------------------
void ClientProtocol::senddata_mappers( const StringVector &lines,
                                       KeyValue &psheader,
                                       const std::string &plheader ) {
    StringVector mappers;
    mappers.assign(mappers_.begin(),mappers_.end());
    std::string header, payload;

    size_t sz = 0;
    for( size_t i = 0; i < lines.size(); ++i ) {
        if( lines[i].empty() ) continue;
        psheader["dst"] = mappers[ i % mappers.size() ].substr(1);

        header = pubheader(psheader);
        payload = plheader + lines[i];
        if( encrypt_ ) { 
            Crypto::encrypt_aes_inline(password,header);
            Crypto::encrypt_aes_inline(password,payload);
        }
        Message m( true, header, payload );
        sz += header.size() + payload.size();
        pipe_.send( m );
    }
    //std::cout << " Sent " << sz << " bytes ";

    // Signalise Mappers we are done for now (until next round)
    std::stringstream empty; 
    empty << MAP_DATATYPE << pload_sep << pload_sep;
    for( auto const &m : mappers_ ) {
        psheader["dst"] = m.substr(1);

        header  = pubheader(psheader);
        payload = empty.str();
        if( encrypt_ ) {
            Crypto::encrypt_aes_inline(password,header);
            Crypto::encrypt_aes_inline(password,payload);
        }
        Message msg( true, header, payload );
        pipe_.send(msg);
    }
}

//------------------------------------------------------------------------------
void ClientProtocol::wcount_preprocess( StringVector &content ) {
}

//------------------------------------------------------------------------------
inline std::string jsonpair( const std::string &c, int id = -1 ) {
    static int xmin = INT_MAX, xmax = INT_MIN, ymin = INT_MAX, ymax = INT_MIN;
    std::stringstream ss, is;
    StringVector pair;
    split( c, " ", pair );
    if( id >= 0 ) is << ",\"id\":" << id; 
    if( pair.size() == 2 )
        ss << "{\"x\":" << pair[0] << ",\"y\":" << pair[1] << is.str() << "}";
    int x = std::stoi(pair[0]), y = std::stoi(pair[1]);
    if( x < xmin ) xmin = x;
    if( x > xmax ) xmax = x;
    if( y < ymin ) ymin = y;
    if( y > ymax ) ymax = y;
    diagonal_range_ = sqrt(pow(xmax-xmin,2)+pow(ymax-ymin,2));
    return ss.str().substr(0, ss.str().size());
}

//------------------------------------------------------------------------------
// Called to store last round of centers
//------------------------------------------------------------------------------
void ClientProtocol::kmeans_output() {
    if( kmeans_centers_.size() != previous_result_.size() ) {
        printf("k-means Oops... We had %lu centers and now there is just %lu\n", 
                            kmeans_centers_.size(), previous_result_.size() );
        exit(-10);
    }
    if( round_ == 0 ) centersfile_ << "[\n";
    centersfile_ << "{\"step\":" << round_ << ",\"centers\":[";
    for( auto const &x : previous_result_ )
        centersfile_ << x.second << ",";
    centersfile_.seekp(-1,centersfile_.cur);
    centersfile_ << "]},\n";
    if( round_ == 0 ) ++round_;
}

//------------------------------------------------------------------------------
// Called before sending a new round of centers to mappers
//------------------------------------------------------------------------------
std::string ClientProtocol::kmeans_clist() {
    std::stringstream clist;
    clist << "[";
    for( auto const &c : previous_result_ )
        clist << c.second << ",";
    clist.seekp( -1, clist.cur );
    clist << "]";
    return clist.str();
}

//------------------------------------------------------------------------------
// Prepare data before sending it to mappers
// Is executed only once. Data_points is used at every iteration of kmeans
// Data split happens here
//------------------------------------------------------------------------------
void ClientProtocol::kmeans_preprocess( StringVector &data_points ) {
    printf("k-means We have %lu centers and %lu data points\n", kmeans_centers_.size(),
                                                        data_points.size() );
    int i = 0; // center index
    for( auto const &c : kmeans_centers_ )
        previous_result_.insert( std::make_pair("-", jsonpair(c,i++) ) );
    //kmeans_centers_.clear();

    StringVector partitions;
    // Partition size
    int pz = 1 + data_points.size()/mappers_.size(); 
    i = pz;

    pointsfile_ << "[";
    for( auto &p : data_points ) {
        std::string point = jsonpair(p);
        //p = "[" + point + "]";

        if( i-- == pz ) { // first point of the partition
            partitions.push_back( "[" + point );
        } else { // not first point
            partitions.back() += "," + point;
        }

        if( i == 0 ) { // partition is full
            partitions.back() += "]";
            i = pz;
        }

        pointsfile_ << point << ",";
    }

    if( partitions.back().back() != ']' ) { // if it was not, close last one
        partitions.back() += ']';
    }
    data_points = partitions;

    /*** DEBUG ONLY ****/
    /*
    std::cout << ">>> " << pz << " " << mappers_.size() << "\n";
    for( auto &l : data_points ) {
        std::cout << l << "\n";
    }
    std::cout << "<<<\n";
    */
    /*******************/

    pointsfile_.seekp(-1, pointsfile_.cur);
    pointsfile_ << "]";
    pointsfile_.close();
}

//------------------------------------------------------------------------------
void ClientProtocol::fire_new_round() {
    ++round_;
    std::cout << "k-means " << time(0) << " Round " << round_ << " ";
    round_stats_.begin();
    KeyValue psheader;
    psheader["sid"] = std::to_string(sid_);
    psheader["type"] = std::to_string(MAP_DATATYPE);

    std::stringstream plheader;
    plheader << psheader["type"] << pload_sep << kmeans_clist() << pload_sep;    
    senddata_mappers( mappers_data_, psheader, plheader.str() );
}

//------------------------------------------------------------------------------
int value_index( const StringVector &v, const std::string &field ) {
    size_t ret = -1;
    StringVector::const_iterator i = std::find( v.begin(), v.end(), field );
    if( i != v.end() ) {
        int x = std::distance( v.begin(), i );
        if( x < v.size() - 1 ) ret = x + 1;
    }
    return ret;
}

//------------------------------------------------------------------------------
void extract_points( const MMKeyValue &source, VecDoublePair &dst ) {
    size_t n = source.size();
    dst.resize( n );
    for( auto const &c : source ) {
        StringVector v;
        if( c.second.size() > 2 ) {
            split( c.second.substr(1,c.second.size()-2), ":,", v );

            int i = value_index(v,"\"id\""), 
                x = value_index(v,"\"x\""), y = value_index(v,"\"y\"");
            if( i < 0 || x < 0 || y < 0 ) { dst.clear(); return; }

            if( v.size() == 6 ) {
                size_t idx = std::stoi(v[i]) % n;
                dst[ idx ].first  = std::stod(v[x]);
                dst[ idx ].second = std::stod(v[y]);
            }
        }
    }
}
//------------------------------------------------------------------------------
double ClientProtocol::kmeans_error() {
    double sum = 0;
    VecDoublePair ccur, cprev;

    extract_points( previous_result_, cprev );
    if( cprev.empty() ) goto parseerror;
    
    extract_points( last_result_, ccur );
    if( ccur.empty() ) goto parseerror;

    if( cprev.size() != ccur.size() ) goto parseerror;
    for( int i = 0; i < cprev.size(); ++i )
        sum += sqrt( pow( cprev[i].first - ccur[i].first, 2) + 
                     pow( cprev[i].second - ccur[i].second, 2)  );
    sum /= cprev.size();
    std::cout << "delta: " << sum << "\n";
    return sum;

parseerror:
    std::cerr << "Parse error! >(\n";
    return std::numeric_limits<double>::max();
}

//------------------------------------------------------------------------------

