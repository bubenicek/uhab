
#ifndef __MINING_JSONRPC_H
#define __MINING_JSONRPC_H


#define CFG_MINING_SW_VERSION           40

#ifndef CFG_MINING_POOL_ADDRLEN
#define CFG_MINING_POOL_ADDRLEN         40
#endif

#ifndef CFG_MINIG_MAXNUM_GPU
#define CFG_MINIG_MAXNUM_GPU            6
#endif

#ifndef CFG_MINING_CLAYMORE_PORT
#define CFG_MINING_CLAYMORE_PORT        3333
#endif

#define CFG_MINING_JSON_MAXNUM_TOKENS     32


/** GPU definition */
typedef struct
{
   uint32_t hashrate;
   uint16_t temperature;
   uint16_t fanspeed;

} mining_jsonrpc_gpu_status_t;


/** Mining rig definition */
typedef struct
{
   char version[CFG_MINING_SW_VERSION];
   uint32_t uptime;
   uint32_t total_hashrate;
   uint32_t total_shares;
   uint32_t total_rejected;
   char current_mining_pool[CFG_MINING_POOL_ADDRLEN];

   mining_jsonrpc_gpu_status_t gpu[CFG_MINIG_MAXNUM_GPU];
   int gpu_count;

} mining_jsonrpc_rig_status_t;


/** Control GPU state 0=disable 1=enable */
int mining_jsonrpc_control_gpu(const char *hostname, int gpu, int state);

/** Get miner rig status */
int mining_jsonrpc_get_miner_status(const char *hostname, mining_jsonrpc_rig_status_t *status);


#endif // __MINING_JSONRPC_H
