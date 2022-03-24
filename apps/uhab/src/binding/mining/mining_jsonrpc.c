
#include "uhab.h"
#include "mining_jsonrpc.h"
#include "httpd_socket.h"
#include "jsmn/jsmn.h"

TRACE_TAG(binding_mining_rpc);
#if !ENABLE_TRACE_MINING_JSONRPC
#include "trace_undef.h"
#endif

#define JSONRPC_METHOD_GET_STATUS   "{\"id\":0,\"jsonrpc\":\"2.0\",\"method\":\"miner_getstat1\"}\n"
#define JSONRPC_METHOD_CONTROL_GPU  "{\"id\":0,\"jsonrpc\":\"2.0\",\"method\":\"control_gpu\", \"params\":[\"%d\", \"%d\"]}\n"


// Prototypes:
static int jsonrpc_sendrecv(const char *hostname, const char *request, char *outbuf, int outbufsize);

// Locals:
static char jsonrpc_buffer[512];


/** Control GPU state 0=disable 1=enable */
int mining_jsonrpc_control_gpu(const char *hostname, int gpu, int state)
{
   int len;
   
   snprintf(jsonrpc_buffer, sizeof(jsonrpc_buffer), JSONRPC_METHOD_CONTROL_GPU, gpu, state);
   if ((len = jsonrpc_sendrecv(hostname, jsonrpc_buffer, jsonrpc_buffer, sizeof(jsonrpc_buffer))) < 0)
   {
      TRACE_ERROR("Send request control gpu to %s failed", hostname);
      throw_exception(fail);
   }
   
   return 0;
   
fail:
   return -1;   
}


#define STATUS_RESULT_INDEX                  5
#define STATUS_MINER_VERSION_INDEX           7
#define STATUS_MINER_UPTIME_INDEX            8
#define STATUS_MINER_TOTAL_HASHRATE_INDEX    9
#define STATUS_GPU_HASHRATE_INDEX            10
#define STATUS_GPU_TEMPFAN_INDEX             13
#define STATUS_MINER_POOL_ADDR_INDEX         15
int mining_jsonrpc_get_miner_status(const char *hostname, mining_jsonrpc_rig_status_t *status)
{
   int ix, iy, ig,  len, res, argc;
   char *value;
   jsmn_parser parser;   
   char *argv[CFG_MINING_JSON_MAXNUM_TOKENS];
   jsmntok_t tokens[CFG_MINING_JSON_MAXNUM_TOKENS];
   
   memset(status, 0, sizeof(mining_jsonrpc_rig_status_t));
                  
   if ((len = jsonrpc_sendrecv(hostname, JSONRPC_METHOD_GET_STATUS, jsonrpc_buffer, sizeof(jsonrpc_buffer))) < 0)
   {
      TRACE_ERROR("Send request status to %s failed", hostname);
      throw_exception(fail);
   }
   
   TRACE("JSON response '%s'", jsonrpc_buffer);

   // Parse API string
   jsmn_init(&parser);
   res = jsmn_parse(&parser, jsonrpc_buffer, len, tokens, CFG_MINING_JSON_MAXNUM_TOKENS);
   if (res < 0)
   {
      TRACE_ERROR("Failed to parse JSON: %d", res);
      throw_exception(fail);
   }

   if (tokens[0].type != JSMN_OBJECT)
   {
      TRACE_ERROR("Top level element is not expected object");
      throw_exception(fail);
   }
   
   // Loop over all keys of the root object
   for (ix = 0; ix < res; ix++)
   {
      switch(tokens[ix].type)
      {            
         case JSMN_STRING:
         case JSMN_PRIMITIVE:
         {
            *(jsonrpc_buffer + tokens[ix].start + tokens[ix].end - tokens[ix].start) = '\0';
            value = jsonrpc_buffer + tokens[ix].start;

            //TRACE("   [%d] %s", ix, value);     
            
            // Read value by token index
            switch(ix)
            {
               case STATUS_RESULT_INDEX:
                  if (strcmp(value, "result") != 0)
                  {
                     TRACE_ERROR("Bad format of json response");
                     throw_exception(fail);
                  }
                  break;                  
                  
               case STATUS_MINER_VERSION_INDEX:
                  strlcpy(status->version, value, sizeof(status->version));
                  break;                                    
                  
               case STATUS_MINER_UPTIME_INDEX:
                  status->uptime = atoi(value);
                  break;
                  
               case STATUS_MINER_TOTAL_HASHRATE_INDEX:
               {
                  if (split_line(value, ';', argv, 3) != 3)
                  {
                     TRACE_ERROR("Bad format of hashrate attribute, expected 3 params");
                     throw_exception(fail);
                  }
                  
                  status->total_hashrate = atoi(argv[0]);
                  status->total_shares = atoi(argv[1]);
                  status->total_rejected = atoi(argv[2]);
               }
               break;
                  
               case STATUS_GPU_HASHRATE_INDEX:
               {
                  argc = split_line(value, ';', argv, CFG_MINIG_MAXNUM_GPU);
                  status->gpu_count = argc;                  
                  
                  for (iy = 0; iy < argc; iy++)
                  {
                     if (strcasecmp(argv[iy], "off") == 0)
                        status->gpu[iy].hashrate = 0;
                     else
                        status->gpu[iy].hashrate = atoi(argv[iy]);
                  }
               }
               break;
                  
               case STATUS_GPU_TEMPFAN_INDEX:
               {
                  argc = split_line(value, ';', argv, CFG_MINIG_MAXNUM_GPU * 2);                  
                  for (iy = 0, ig = 0; iy < argc; iy+=2, ig++)
                  {
                     status->gpu[ig].temperature = atoi(argv[iy]);
                     status->gpu[ig].fanspeed = atoi(argv[iy+1]);
                  }
               }
               break;
                  
               case STATUS_MINER_POOL_ADDR_INDEX:
                  strlcpy(status->current_mining_pool, value, sizeof(status->current_mining_pool));
                  break;
                                    
               default:
                  break;
            }
           
         }
         break;

         case JSMN_OBJECT:
         case JSMN_ARRAY:
            break;

         default:
            TRACE_ERROR("Not supported JSON type: %d in the object", tokens[ix].type);
            throw_exception(fail);
      }
   } 

   return 0;
   
fail:
   return -1;   
}

/** Send request and receive response */
static int jsonrpc_sendrecv(const char *hostname, const char *request, char *outbuf, int outbufsize)
{
   int sd = -1;
   int len;
   
   if ((sd = httpd_raw_socket_connect(hostname, CFG_MINING_CLAYMORE_PORT)) < 0)
   {
      TRACE_ERROR("Connect to %s:%d failed", hostname, CFG_MINING_CLAYMORE_PORT);
      throw_exception(fail);
   }

   if ((len = httpd_raw_socket_send(sd, request, strlen(request))) < 0)
   {
      TRACE_ERROR("Send request: %s to %s failed", request, hostname);
      throw_exception(fail);
   }  
   TRACE("Send jsonrpc request: %s to: %s  len: %d", request, hostname, len);
   
   if ((len = httpd_raw_socket_recv(sd, outbuf, outbufsize)) < 0)
   {
      TRACE_ERROR("Receive response from: %s failed", hostname);
      throw_exception(fail);
   }
   
   if (len > 0)
   {  
      TRACE("Recv jsonrpc response from: %s  len: %d", hostname, len);
      jsonrpc_buffer[len] = 0;
   }
   
   httpd_raw_socket_close(sd);
   
   return len;
   
fail:
   if (sd != -1)
      httpd_raw_socket_close(sd);
   
   return -1;
}
