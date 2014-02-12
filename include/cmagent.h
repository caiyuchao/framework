#ifndef __CMAGENT_H__
#define __CMAGENT_H__

#include <time.h>
#include <stdint.h>
#include <stdio.h>

enum agent_states {
    AGENT_CONNECTED  = 0,
    AGENT_CONNECTING,
    AGENT_DISCONNECTED,
    AGENT_UNKNOW,                       //A fetal erorr is occured if we run into this state, for instance, tring to connect while agent is in connected state.
};

enum disconn_reasons {
    SSL_CONNECTION_ERROR = 0,
    TCP_CONNECTION_ERROR,
    CONNECTION_INITIALIZING,
    RELOADING,
    KEEPALIVE_ERROR,
};

typedef struct agent_ctx {
    FILE * ctx_file;
    int conn_state;                     //Current connection state of cmagent.
    int disconn_reason;                 //Why we are in disconnected state.
    time_t retry_tm;                    //We will try to connect to Carrier at retry_tm if we are disconnected.
    uint32_t nr_failure;                //How many times we have been tring to connect to Carrier so far.
} agent_ctx_t;

#define get_agent_ctx() (&agent_ctx)
extern agent_ctx_t agent_ctx;
extern const char *get_agent_state_string(int);
extern const char *get_disconn_reason_string(int);
extern void dump_agent_ctx(void);
#endif
