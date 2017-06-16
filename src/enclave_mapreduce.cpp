#include <enclave_mapreduce.h>
#include <enclave_mapreduce_t.h>
#include <lauxlib.h>
#include <lualib.h>
#include <lua_parser.h>
#include <file_mock.h>
#include <mrprotocol.h>
#include <string>
#include <enclaved_mapper.h>
#include <enclaved_reducer.h>
#include <sgx_utiles.h>

//------------------------------------------------------------------------------
/* 
 * printf: 
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
//------------------------------------------------------------------------------
void printf(const char *fmt, ...) {
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print(buf);
}

//==============================================================================
EnclavedMapper mapper;
EnclavedReducer reducer;
WorkerProtocol *broker = 0;
extern "C" { static int l_push( lua_State *L ); } 
lua_State *L = 0;

//------------------------------------------------------------------------------
extern "C" { extern int luaopen_cjson(lua_State *l); }
void ecall_init( const char *id, char enc ) {
    /* LUA stuff */
    L = luaL_newstate();  /* create state */
    if (L == NULL) {
        ocall_print("cannot create state: not enough memory");
        return;
    }
    luaL_requiref(L, "cjson", luaopen_cjson, 1);
    luaL_openlibs(L);

    lua_createtable(L, 0, 2);
    lua_pushstring(L, "_lua_interpreter_"); lua_rawseti(L,-2,-1);
    lua_pushstring(L, "_lua_script_");      lua_rawseti(L,-2, 0);
    lua_setglobal(L,"arg");

    lua_pushcfunction(L, l_push);
    lua_setglobal(L, "push");

    lua_settop(L,0);

    /* MR stuff */
    if( id[0] == 'M' ) {
        mapper.setmap( id, enc );
        broker = &mapper;
    } else if( id[0] == 'R' ) {
        reducer.setreduce( id, enc );
        broker = &reducer;
    } else {
        ocall_outputerror("Invalid role: neither mapper or reducer");
    }
    if( broker ) broker->initialsub();
}

void ecall_luaclose() {
    lua_close(L);
}

void ecall_add_execution( const char *fname, const char *e, size_t len ) {
    file_mock( e, len, fname );
}

//------------------------------------------------------------------------------
void lua_error() {
    ocall_outputerror(lua_tostring(L, -1));
}

struct Config {
    bool writethrough;
} config;

//------------------------------------------------------------------------------
static char * getdestination( const char *key ) {
    static char dst[] = { '?',0 };
    if( mapper.state() ) {
        lua_getglobal(L,"hash");
        lua_pushstring(L,key);
        lua_pushinteger(L,mapper.reducers_count());
        if( lua_pcall(L,2,1,0) ) { lua_error(); return 0; }

        /* retrieve result */
        if (!lua_isnumber(L, -1)) { lua_error(); return 0; }

        dst[0] = lua_tonumber(L, -1) + '0';
        lua_pop(L, 1);  /* pop returned value */
    }
    return dst;
}

//------------------------------------------------------------------------------
void apply_combine( const char *key, const char *value) {
    lua_getglobal(L,"combine");
    if( lua_isnil(L,-1) ) {
        printf("Ooops: No combine function\n");
        lua_pop(L,1);
    } else {
        config.writethrough = true;
        lua_pushstring(L,key);
        lua_pushstring(L,value);
        if( lua_pcall(L,2,0,0) ) { lua_error(); return; } 
    }
}

//------------------------------------------------------------------------------
static void buffer_output( const char *key, const char *value ) {

    if( config.writethrough ) {
        const char *dst = getdestination(key);
        if( broker ) broker->outputpair( dst, key, value );
        return;
    } else if( mapper.state() ) {
        mapper.store_output(key,value);
    }
}

//------------------------------------------------------------------------------
static int l_push( lua_State *L ) {
    luaL_checkstring( L, 1 );
    luaL_checkstring( L, 2 );
    const char *key =   lua_tostring( L, 1 ),
               *value = lua_tostring( L, 2 );
    buffer_output( key, value );
    return 0;
}
//------------------------------------------------------------------------------
void process_data( const char *k, const char *v, const char *funcname ) {
    //printf("Func [%s]\n", funcname); 
    lua_getglobal(L,funcname); // -1
    lua_pushstring(L,k);       // -2
    lua_pushstring(L,v);       // -3
    if( lua_pcall(L,2,0,0) ) { lua_error(); return; }
}

//------------------------------------------------------------------------------
void process_code( const char *buff, size_t len, const char *fname ) {
    printf("File [%s]\n",fname);
    file_mock( buff, len, fname );
    if( luaL_loadfile(L,fname) ) { lua_error(); return; }
    if( lua_pcall(L,0,0,0) ) { lua_error(); return; } 
}

//------------------------------------------------------------------------------
void ecall_inputcode( const char *buff, const char *fname ) {
    file_mock( buff, strlen(buff), fname );
}

//------------------------------------------------------------------------------
void ecall_inputdata( const char *buff, size_t len ) {
    char *recovered = (char*)malloc(len+1);
    if( !recovered ) {
        ocall_outputerror("Unable to allocate memory");
        return;
    }

    const char *data = recovered;
    recovered[len] = 0;
    if( is_cipher(buff,len) ) {
        decrypt(buff,recovered,len);
        data = recovered;
    } else {
        memcpy(recovered,buff,len);
    }

    static bool once = true;
    if( once ) {
        printf("Session id is %s\n", data);
        if( broker ) broker->startsession( data );
        once = false;
        goto getout;
    }

    if( len < 2 || data[0] == 0 || data[1] == 0 ) {
        char buff[50];
        snprintf( buff, sizeof(buff), 
                  "Err msg format %lu %d %d", len, data[0], data[1]);
        ocall_outputerror(buff);
        goto getout;
    } else if( data == recovered && is_cipher(data,len) ) {
        ocall_outputerror("Decryption still gives cipher");
        goto getout;
    }

    {
    MsgType t = MsgType( data[0] - '0' );
    if( t == MAP_CODETYPE || t == MAP_DATATYPE ) {
        static bool once = true;
        if( once && t == MAP_DATATYPE ) {
            lua_getglobal(L,"combine"); 
            if( lua_isnil(L,-1) ) {
                printf("There is no combine function\n");
                config.writethrough = true;
            } else {
                config.writethrough = false;
            }
            lua_pop(L,1);
            once = false;
        }
        mapper.income_data( t, data+2, len-2 );
    } else if( t == REDUCE_CODETYPE || t == REDUCE_DATATYPE ) {
        config.writethrough = true;
        reducer.income_data( t, data+2, len-2 );
    } else {
        ocall_outputerror("Unidentified message");
    }
    }
getout:
    free(recovered);
}

//------------------------------------------------------------------------------

