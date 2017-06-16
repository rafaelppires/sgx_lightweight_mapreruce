/**
  *  MapReduce client - Subscribe reducced data
  *                     Publish code for Map, Reduce and Data
 **/

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <message.h>
#include <client_protocol.h>
#include <mrprotocol.h>
#include <zhelpers.hpp>
#include <argp.h>

//------------------------------------------------------------------------------
static void fileerror( const std::string &fname ) {
    std::cerr << "Error opening file \"" << fname << "\"\n";
    exit(-2);
}

//------------------------------------------------------------------------------
static void openfiles( std::ifstream files[], std::string fnames[], int n ) {
    for( int i = 0; i < n; ++i ) {
        files[i].open(fnames[i].c_str());
        if( !files[i].good() ) fileerror(fnames[i]);
    }
}

//------------------------------------------------------------------------------
const char *argp_program_version = "SGX Lua MapReduce Client 0.9";
const char *argp_program_bug_address = "<rafael.pires@unine.ch>";

/* Program documentation. */
static char doc[] =
  "SGX Lua MapReduce Client - Fire MR jobs: Wordcount or Kmeans";
static char args_doc[] = "";
static struct argp_option options[] = {
    { 0,0,0,0, "To define MR code" },
    { "mapper",  'm', "Filename", 0, "Set mapper Lua script" },
    { "reducer", 'r', "Filename", 0, "Set reducer Lua script" },
    { 0,0,0,0, "To define input data" },
    { "datafile",  'd', "Filename", 0, "Set input data file" },
    { 0,0,0,0, "To define application behavior" },
    { "kmeans",  'k', "Filename", 0, "Application is Kmeans, centers data file is Filename" },
    { "encrypt", 'e', 0, OPTION_ARG_OPTIONAL, "Encrypt messages" },
    { "pubsub",  'p', "host:port", 0, "Publish/Subscribe engine "
                                    "runs in the provided address" },
    { 0 }
};
//------------------------------------------------------------------------------
struct Arguments {
    Arguments() : encrypt(false), kmeans(false), id("Client01") {}
    std::string id, address, datafile, kmeans_centers, mapscript, reducescript;
    bool encrypt, kmeans;
};
//------------------------------------------------------------------------------
/* Parse a single option. */
static error_t parse_opt (int key, char *arg, struct argp_state *state) {
    Arguments *args = (Arguments*)state->input;
    switch(key) {
    case 'm':
        args->mapscript = std::string(arg);
        break;
    case 'r':
        args->reducescript = std::string(arg);
        break;
    case 'd':
        args->datafile = std::string(arg);
        break;
    case 'p':
        args->address = std::string("tcp://") + arg;
        break;
    case 'e':
        args->encrypt = true;
        break;
    case 'k':
        args->kmeans = true;
        args->kmeans_centers = std::string(arg);
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    };
    return 0;
}

static struct argp argp = { options, parse_opt, 0, doc };
//------------------------------------------------------------------------------
void error( const char *msg, int code ) {
    printf("%s (-? or --help for info)\n", msg);
    exit(code);
}

//------------------------------------------------------------------------------
int main( int argc, char **argv ) {
    Arguments args;
    argp_parse(&argp, argc, argv, 0, 0, &args);

    if( args.address == "" ) error("Missing Pub/sub address", -1);
    if( args.mapscript == "" ) error("Missing Mapper", -2);
    if( args.reducescript == "" ) error("Missing Reducer", -3);
    if( args.datafile == "" ) error("Missing input data file", -4);
    
    printf("M/R encryption is turned %s\033[0m\n",
                args.encrypt ? "\033[32mon" : "\033[1;31moff" );
    zmq::context_t context(1);
    Communication< zmq::socket_t > pipe( context,
                                         args.address, args.id, false);
    ClientProtocol protocol( args.encrypt, args.id, pipe );
    if( args.kmeans ) protocol.setApp( KMEANS );

    protocol.init();

    // Wait for result
    size_t counter = 0;
    while(1) {
        bool rc;
        do {
            zmq::message_t zmsg;
            bool requires_action = false; 
            if( (rc = pipe.socket().recv( &zmsg, ZMQ_DONTWAIT)) ) {
                // ignore 0 msg
                requires_action = protocol.handle_msg( s_recv(pipe.socket()) );
            }
            if( requires_action ) {
                counter = 1000;
                requires_action = false;
            } else if( counter > 0 && --counter == 0 ) {
                std::string fnames[4];
                std::ifstream files[4];
                fnames[0] = args.mapscript; fnames[1] = args.reducescript;
                fnames[2] = args.datafile;  fnames[3] = args.kmeans_centers;
                openfiles( files, fnames, args.kmeans ? 4 : 3 );
                protocol.fire_jobs( files, fnames );
            }
        } while( rc );
        usleep(1000);
    }
}

