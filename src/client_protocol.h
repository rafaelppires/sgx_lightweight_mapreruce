#ifndef _CLIENT_PROTOCOL_H_
#define _CLIENT_PROTOCOL_H_

#include <communication_zmq.h>
#include <map>
#include <set>
#include <fstream>
#include <mrprotocol.h>
#include <scbrmsg.h>
#include <utils.h> // for stats

enum ApplicationType {
    WORDCOUNT,
    KMEANS
};

//typedef std::map<std::string,std::string> KeyValue;
typedef std::multimap<std::string,std::string> MMKeyValue;
typedef std::vector< std::pair<double,double> > VecDoublePair;
class ClientProtocol {
public:
    ClientProtocol( bool enc, std::string id,
                    Communication< zmq::socket_t > &p ) : encrypt_(enc),
                       id_(id), pipe_(p), threshold_(1e-3), reducers_done_(0), 
                       app_type_(WORDCOUNT), round_(0),  
                       round_stats_(Stats::SECONDS) {}

    void init();
    bool handle_msg( const std::string &msg );
    void fire_jobs( std::ifstream files[], std::string fnames[] );
    void setApp( ApplicationType t ) { app_type_ = t; }

private:
    void extractwt( const std::string &s, std::string &w, int &t );
    void generatessid();
    void push_data( MsgType t, const std::string &fname, std::ifstream &file );
    void wcount_preprocess( StringVector &content );
    void load_file( std::ifstream &file, std::string &content );
    void senddata_mappers( const StringVector &lines, KeyValue &psheader, 
                                             const std::string &plheader );
    StringVector mappers_data_;
    std::set<std::string> mappers_, reducers_;
    std::string id_;
    int sid_;
    ApplicationType app_type_;
    Communication< zmq::socket_t > &pipe_;
    size_t reducers_done_;
    bool encrypt_;

    // KMEANS
    size_t round_;
    std::ofstream pointsfile_, centersfile_;
    void kmeans_preprocess( StringVector &content );
    void fire_new_round();
    double kmeans_error();
    void kmeans_output();
    void combine_results();
    std::string kmeans_clist();
    double kerror_;;
    Stats round_stats_;

    double threshold_;
    StringVector kmeans_centers_;

    // To compute error between iterations
    MMKeyValue previous_result_, last_result_;
};

#endif

