/**
  * Worker - Subscribe for code and data to be processed
  *          Process both inside an enclave
  *          Publish resulting data
 **/

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <sstream>
#include <enclave_mapreduce_u.h>
#include <sgx_initenclave.h>
#include <libgen.h>
#include <zhelpers.hpp>
#include <fstream>
#include <argp.h>
#include <sgx_cryptoall.h>
#include <communication_zmq.h>
#include <mrprotocol.h>

//------------------------------------------------------------------------------
/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;
//------------------------------------------------------------------------------
Communication< zmq::socket_t > *global_pipe = 0;
std::ofstream luamemstats;
//------------------------------------------------------------------------------
size_t countoutput = 0;
void ocall_outputdata( const char *header, size_t hsz, 
                       const char *payload, size_t psz, size_t luavm ) {
    countoutput += hsz + psz;
    Message m(true, std::string(header,hsz), std::string(payload,psz) );
    if( global_pipe ) global_pipe->send( m );
    if( luamemstats.is_open() ) {
        static int last = 0;
        int now = time(0);
        if( now != last ) {
            last = now;
            luamemstats << last << "\t" << luavm << "\n";
        }
    }
}

//------------------------------------------------------------------------------
void ctrlc_handler( int s ) {
    printf("\033[0m\n(%d) Bye!\n", s);
    luamemstats.close();
    exit(1);
}

//------------------------------------------------------------------------------
void ocall_mapped() {
    // publish_reducible_data();
}

//------------------------------------------------------------------------------
const char *argp_program_version = "SGX Lua MapReduce Worker 0.9";
const char *argp_program_bug_address = "<rafael.pires@unine.ch>";

/* Program documentation. */
static char doc[] =
  "SGX Lua MapReduce Worker - Accepts jobs to operate on data coming from "
  "IPC or a CBR engine";
static char args_doc[] = "";
static struct argp_option options[] = {
    { 0,0,0,0, "To define worker BEHAVIOR" },
    { "mapper",  'm', "Id", 0, "Set it as mapper with provided Id" },
    { "reducer", 'r', "Id", 0, "Set it as reducer with provided Id" },
    { "encrypt", 'e', 0, 0, "Encrypt messages" },
    { 0,0,0,0, "To define DATASOURCE" },
    { "pubsub",  'p', "host:port", 0, "Data comes from the Publish/Subscribe "
                                    "engine in the provided address" },
    { "ipc", 'i', "Address", 0, "Data comes from interprocess communication in "
                                "provided Address"},
    { 0 }
};
//------------------------------------------------------------------------------
struct Arguments {
    Arguments() : encrypt(false) {}
    std::string id, address;
    bool encrypt;
};
//------------------------------------------------------------------------------
/* Parse a single option. */
static error_t parse_opt (int key, char *arg, struct argp_state *state) {
    Arguments *args = (Arguments*)state->input;
    switch(key) {
    case 'm':
        args->id = "M" + std::string(arg); 
        break;
    case 'r':
        args->id = "R" + std::string(arg); 
        break;
    case 'p':
        args->address = std::string("tcp://") + arg;
        break;
    case 'i':
        args->address = std::string("ipc://") + arg;
        break;
    case 'e':
        args->encrypt = true;
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    };
    return 0;
}
static struct argp argp = { options, parse_opt, 0, doc };
//------------------------------------------------------------------------------
int SGX_CDECL main( int argc, char **argv ) {
    Arguments args;
    argp_parse(&argp, argc, argv, 0, 0, &args);

    printf("Id: %s Addr: %s Encryption: %s\n", 
                args.id.c_str(), args.address.c_str(), 
                args.encrypt?"\033[32my\033[0m":"\033[1;31mn\033[0m");
    if( args.id == "" || args.address == "" ) {
        std::cerr << argp_program_version << ": Error: " 
                  << "Either id ('" << args.id << "') " << "and address ('"
                  << args.address << "') must be specified "
                                     "(-? or --help for info)\n"; 
        return -1;
    }

    /* To record stats of mem occupancy inside enclave */
    std::string pid = std::to_string(getpid());
    luamemstats.open( std::string("logs/lmemstats_" + args.id + "_" + pid).c_str() );
    /* ----------------------------------------------- */

#ifndef NONENCLAVE
    /* Changing dir to where the executable is.*/
    char absolutePath [MAX_PATH];
    char *ptr = NULL;

    ptr = realpath(dirname(argv[0]),absolutePath);

    if( chdir(absolutePath) != 0)
            abort();

    /* Initialize the enclave */
    if(initialize_enclave( global_eid,
                        "enclave_mapreduce.signed.so","enclave.mr.token") < 0) {
        return -2;
    }
#endif
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = ctrlc_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT,&sigIntHandler,NULL);

    zmq::context_t context(1);
    Communication< zmq::socket_t > pipe( context, args.address, args.id, false);
    //WorkerProtocol protocol( args.encrypt, args.id, pipe );
    global_pipe = &pipe;

    ecall_init( global_eid, args.id.c_str(), args.encrypt );

    // Get respective publications
    //protocol.init();
    while(1) {
        bool rc;
        do {
            zmq::message_t zmsg;
            if( (rc = pipe.socket().recv( &zmsg, ZMQ_DONTWAIT)) ) {
                // ignore 0 msg
                static size_t countinput = 0;
                std::string payload = s_recv(pipe.socket());

                if( payload.size() ) {
                    ecall_inputdata( global_eid, payload.c_str(), payload.size() );

                    countinput += payload.size();
                    if( payload.size() == 3 ) {
                        std::cout << "in: " << countinput << "\t";
                        std::cout << "out: " << countoutput << "\n";
                        countinput = 0;
                        countoutput = 0;
                    }
                }
            }
        } while( rc );
        usleep(1000);
    }
    ecall_luaclose( global_eid );

#ifndef NONENCLAVE
    sgx_destroy_enclave(global_eid);
#endif
    return 0;
}

