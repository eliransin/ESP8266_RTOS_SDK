// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdlib.h>
#include <string.h>

#include "tcpip_adapter.h"


//#include "netif/etharp.h"
#include "esp_wifi.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"
//#include "dhcpserver/dhcpserver.h"
//#include "dhcpserver/dhcpserver_options.h"
#include "esp_log.h"
#include "internal/esp_wifi_internal.h"

#include "FreeRTOS.h"
#include "timers.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
#include "FreeRTOS_Routing.h"
#include "esp_aio.h"
int ieee80211_output_pbuf(esp_aio_t *aio);

// struct tcpip_adapter_pbuf {
//     struct pbuf_custom  pbuf;

//     void                *eb;
//     void                *buffer;
// };

// struct tcpip_adapter_api_call_data {
//     struct tcpip_api_call_data call;

//     struct netif        *netif;
// };

static const char* get_if_name( tcpip_adapter_if_t if_name) {
    switch (if_name)
    {
        case TCPIP_ADAPTER_IF_STA:     /**< Wi-Fi STA (station) interface */
            return "Wi-Fi STA";
        case TCPIP_ADAPTER_IF_AP:      /**< Wi-Fi soft-AP interface */
            return "Wi-Fi soft-AP";
        case TCPIP_ADAPTER_IF_ETH:         /**< Ethernet interface */
            return "Ethernet interface";
        case TCPIP_ADAPTER_IF_TEST:        /**< tcpip stack test interface */
            return "LoopBack interface";
        case TCPIP_ADAPTER_IF_MAX:
        default:
            return "Invalid Interface";
    }
}

const  uint8_t ethbroadcast[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

enum if_state {
    UP = 0,
    DOWN
};

typedef struct if_data {
    tcpip_adapter_if_t if_name;
    tcpip_adapter_ip_info_t ip_info;
    tcpip_adapter_ip_info_t old_ip_info;
    enum if_state state;
    NetworkEndPoint_t endpoint;
} if_data_t;

static NetworkInterface_t _esp_netif[TCPIP_ADAPTER_IF_MAX];
static struct if_data _if_data[TCPIP_ADAPTER_IF_MAX];
static bool _enabled_interfaces[TCPIP_ADAPTER_IF_MAX] = {true, true, false, false};

#define NET_IF(adapter) (&(_esp_netif[adapter]))
#define IF_DATA(adapter) (&(_if_data[adapter]))
#define IF_DATA_FROM_IF(net_if) ((struct if_data*)&(net_if->pvArgument))
#define IF_NAME_FROM_IF(net_if) (IF_DATA_FROM_IF(net_if)->if_name)
ESP_EVENT_DEFINE_BASE(IP_EVENT);
static const char* TAG = "tcpip_adapter";
#define TCPIP_ADAPTER_LOGI(format_str,...)  ESP_LOGI(TAG," %s : "format_str,__PRETTY_FUNCTION__,__VA_ARGS__)

void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent, 
        struct xNetworkEndPoint *pxEndPoint ) {
    NetworkInterface_t* net_if = pxEndPoint->pxNetworkInterface;
    tcpip_adapter_if_t if_name = IF_NAME_FROM_IF(net_if);
    if (eNetworkEvent = eNetworkUp) {
        // Someone just got an IP so lets adevertise this
        bool ip_changed = false;
        tcpip_adapter_ip_info_t ip_info;
        ip_info.ip.s_addr = pxEndPoint->ulIPAddress;
        ip_info.gw.s_addr = pxEndPoint->ulGatewayAddress;
        ip_info.netmask.s_addr = pxEndPoint->ulNetMask;
        tcpip_adapter_set_ip_info(if_name,&ip_info);
        system_event_t evt;
        bool send_event = false;
        if (if_name == TCPIP_ADAPTER_IF_STA) {
            evt.event_id = SYSTEM_EVENT_STA_GOT_IP;
            send_event = true;
        } else if (if_name == TCPIP_ADAPTER_IF_ETH) {
            evt.event_id = SYSTEM_EVENT_ETH_GOT_IP;
            send_event = true;
        }
        if( send_event) {
            evt.event_info.got_ip.ip_changed = false;
            if (memcmp(&ip_info, &(IF_DATA_FROM_IF(net_if)->old_ip_info), sizeof(tcpip_adapter_ip_info_t))) {
                evt.event_info.got_ip.ip_changed = true;
            }

            evt.event_info.got_ip.if_index = if_name;
            memcpy(&evt.event_info.got_ip.ip_info, &ip_info, sizeof(tcpip_adapter_ip_info_t));
        }
        tcpip_adapter_set_old_ip_info(if_name,&ip_info);
        
        if(send_event) {
            esp_event_send(&evt);
        }            
        TCPIP_ADAPTER_LOGI("%s : Network up event",get_if_name(if_name));
    } else { // eNetworkDown
        TCPIP_ADAPTER_LOGI("%s : Network down event", get_if_name(if_name));
    }
}


    
    //             ESP_LOGD(TAG, "if%d tcpip adapter set static ip: ip changed=%d", tcpip_if, evt.event_info.got_ip.ip_changed);
    //         }
static bool tcpip_inited = false;

// 

// 

// /* Avoid warning. No header file has include these function */
// err_t ethernetif_init(struct netif* netif);
// void system_station_got_ip_set();

// static int dhcp_fail_time = 0;
// static tcpip_adapter_ip_info_t esp_ip[TCPIP_ADAPTER_IF_MAX];
// static TimerHandle_t dhcp_check_timer;

// static void tcpip_adapter_dhcps_cb(uint8_t client_ip[4])
// {
//     ESP_LOGI(TAG,"softAP assign IP to station,IP is: %d.%d.%d.%d",
//                 client_ip[0],client_ip[1],client_ip[2],client_ip[3]);
//     system_event_t evt;
//     evt.event_id = SYSTEM_EVENT_AP_STAIPASSIGNED;
//     memcpy(&evt.event_info.ap_staipassigned.ip, client_ip, 4);
//     esp_event_send(&evt);
// }

// static err_t _dhcp_start(struct tcpip_api_call_data *p)
// {
//     err_t ret;
//     struct tcpip_adapter_api_call_data *call = (struct tcpip_adapter_api_call_data *)p;

//     ret = dhcp_start(call->netif);
//     // TODO: What the hell is this for?    
//     // if (ret == ERR_OK)
//     //     dhcp_set_cb(call->netif, tcpip_adapter_dhcpc_cb);

//     return ret;
// }

// static err_t _dhcp_stop(struct tcpip_api_call_data *p)
// {
//     struct tcpip_adapter_api_call_data *call = (struct tcpip_adapter_api_call_data *)p;

//     dhcp_stop(call->netif);

//     return 0;
// }

// static err_t _dhcp_release(struct tcpip_api_call_data *p)
// {
//     struct tcpip_adapter_api_call_data *call = (struct tcpip_adapter_api_call_data *)p;

//     dhcp_release(call->netif);
//     dhcp_stop(call->netif);
//     dhcp_cleanup(call->netif);

//     return 0;
// }

// static err_t _dhcp_clean(struct tcpip_api_call_data *p)
// {
//     struct tcpip_adapter_api_call_data *call = (struct tcpip_adapter_api_call_data *)p;

//     dhcp_stop(call->netif);
//     dhcp_cleanup(call->netif);

//     return 0;
// }

// static int tcpip_adapter_start_dhcp(struct netif *netif)
// {
//     struct tcpip_adapter_api_call_data call;

//     call.netif = netif;

//     return tcpip_api_call(_dhcp_start, (struct tcpip_api_call_data *)&call);
// }

// static int tcpip_adapter_stop_dhcp(struct netif *netif)
// {
//     struct tcpip_adapter_api_call_data call;

//     call.netif = netif;

//     return tcpip_api_call(_dhcp_stop, (struct tcpip_api_call_data *)&call);
// }

// static int tcpip_adapter_release_dhcp(struct netif *netif)
// {
//     struct tcpip_adapter_api_call_data call;

//     call.netif = netif;

//     return tcpip_api_call(_dhcp_release, (struct tcpip_api_call_data *)&call);
// }

// static int tcpip_adapter_clean_dhcp(struct netif *netif)
// {
//     struct tcpip_adapter_api_call_data call;

//     call.netif = netif;

//     return tcpip_api_call(_dhcp_clean, (struct tcpip_api_call_data *)&call);
// }


static BaseType_t initialize_network_if(struct xNetworkInterface *net_if) {
    struct if_data* ifdata= (struct if_data*)net_if->pvArgument;
    TCPIP_ADAPTER_LOGI("%s - network state is %s",get_if_name(ifdata->if_name), (ifdata->state == UP)? "UP":"DOWN");    
    return !(ifdata->state == UP);
}

StaticSemaphore_t tx_semaphore_storage;
SemaphoreHandle_t tx_semaphore;

NetworkBufferDescriptor_t * const current_buff = NULL;
BaseType_t current_release_after_send = pdFALSE;
int tx_cb(struct esp_aio *aio) {
#if ipconfigZERO_COPY_TX_DRIVER != 0
    if(aio->arg) {
        vReleaseNetworkBufferAndDescriptor(aio->arg);
    } else {
#else
    {
#endif
        free((void*)aio->pbuf);
    }
    xSemaphoreGive(tx_semaphore);
    return 0;
}

static BaseType_t NetIFOut(struct xNetworkInterface *net_if,
	    NetworkBufferDescriptor_t * const buff,
	    BaseType_t release_after_send) {
    xSemaphoreTake(tx_semaphore,portMAX_DELAY);            
    if (IF_DATA_FROM_IF(net_if)->state == UP) {
        esp_aio_t async_tx_cmd;
        async_tx_cmd.fd = (wifi_interface_t)IF_NAME_FROM_IF(net_if);
        async_tx_cmd.cb = tx_cb;
        async_tx_cmd.len = buff->xDataLength;
#if ipconfigZERO_COPY_TX_DRIVER != 0
        // Zero copy
        if (release_after_send) {            
            async_tx_cmd.pbuf = buff->pucEthernetBuffer;            
            async_tx_cmd.arg = buff;            
        } else {
#else
        {
#endif
            async_tx_cmd.arg = NULL;
            char * new_buff = malloc(async_tx_cmd.len);
            memcpy(new_buff,buff->pucEthernetBuffer,async_tx_cmd.len);
            async_tx_cmd.pbuf = new_buff;
            
        }
        ieee80211_output_pbuf(&async_tx_cmd);        
    } else {
        xSemaphoreGive(tx_semaphore);
        return -1;
    }
    
    return 0;
}



void tcpip_adapter_init(void)
{
    if (tcpip_inited == false) {
        tcpip_inited = true;
        // initialize the tx semaphore
        tx_semaphore = xSemaphoreCreateBinaryStatic(&tx_semaphore_storage);        
        xSemaphoreGive(tx_semaphore);
        //tcpip_init(NULL, NULL);        
        memset(_esp_netif, 0, sizeof(_esp_netif));
        memset(_if_data, 0, sizeof(_if_data));
        for (int i = 0 ; i< TCPIP_ADAPTER_IF_MAX; i++) {
            if(!_enabled_interfaces[i]) {
                continue;
            }
            _if_data[i].if_name = i;
            _if_data[i].state = DOWN; 
            _esp_netif[i].pvArgument = &(_if_data[i]);
            _esp_netif[i].pfInitialise = &initialize_network_if;
            _esp_netif[i].pfOutput = &NetIFOut;            
            FreeRTOS_AddNetworkInterface(&(_esp_netif[i]));
        }
        
        // Access point has a static IP address
        // TODO: make it changable...
        if (_enabled_interfaces[TCPIP_ADAPTER_IF_AP]) {
            _if_data[TCPIP_ADAPTER_IF_AP].ip_info.ip.s_addr = FreeRTOS_inet_addr("192.168.4.1");
            _if_data[TCPIP_ADAPTER_IF_AP].ip_info.gw.s_addr = _if_data[TCPIP_ADAPTER_IF_AP].ip_info.ip.s_addr;
            _if_data[TCPIP_ADAPTER_IF_AP].ip_info.ip.s_addr = FreeRTOS_inet_addr("255.255.255.0");
            FreeRTOS_AddEndPoint(NET_IF(TCPIP_ADAPTER_IF_AP),&(_if_data[TCPIP_ADAPTER_IF_AP].endpoint));
        }
            
        // The loopback interface has everything preconfigured
        if (_enabled_interfaces[TCPIP_ADAPTER_IF_TEST]) {
            _if_data[TCPIP_ADAPTER_IF_TEST].ip_info.ip.s_addr = FreeRTOS_inet_addr("127.0.0.1");
            _if_data[TCPIP_ADAPTER_IF_TEST].ip_info.gw.s_addr = _if_data[TCPIP_ADAPTER_IF_AP].ip_info.ip.s_addr;
            _if_data[TCPIP_ADAPTER_IF_TEST].ip_info.ip.s_addr = FreeRTOS_inet_addr("255.255.255.255");
            FreeRTOS_AddEndPoint(NET_IF(TCPIP_ADAPTER_IF_TEST),&(_if_data[TCPIP_ADAPTER_IF_TEST].endpoint));
        }
        
        // Station uses dhcp by default
        if (_enabled_interfaces[TCPIP_ADAPTER_IF_STA]) {
            memset(&(_if_data[TCPIP_ADAPTER_IF_STA].ip_info),0,sizeof(tcpip_adapter_ip6_info_t));
            _if_data[TCPIP_ADAPTER_IF_STA].endpoint.bits.bWantDHCP = true;
            FreeRTOS_AddEndPoint(NET_IF(TCPIP_ADAPTER_IF_STA),&(_if_data[TCPIP_ADAPTER_IF_STA].endpoint));
        }

        // Ethernet uses dhcp by default
        if (_enabled_interfaces[TCPIP_ADAPTER_IF_ETH]) {
            memset(&(_if_data[TCPIP_ADAPTER_IF_ETH].ip_info),0,sizeof(tcpip_adapter_ip6_info_t));
            _if_data[TCPIP_ADAPTER_IF_ETH].endpoint.bits.bWantDHCP = true;
            FreeRTOS_AddEndPoint(NET_IF(TCPIP_ADAPTER_IF_ETH),&(_if_data[TCPIP_ADAPTER_IF_ETH].endpoint));
        }        
        // Finally - start the whole thing
        FreeRTOS_IPStart();
    }
}

esp_err_t tcpip_adapter_start(tcpip_adapter_if_t tcpip_if, uint8_t *mac, tcpip_adapter_ip_info_t *ip_info)
{
    esp_err_t ret = -1;
    if (tcpip_if >= TCPIP_ADAPTER_IF_MAX || mac == NULL || ip_info == NULL) {
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }
    
    NetworkInterface_t* net_if = NET_IF(tcpip_if);
    NetworkEndPoint_t* endpoint =  FreeRTOS_FirstEndPoint(net_if);
    
    if (!endpoint) {
        endpoint = malloc(sizeof(NetworkEndPoint_t));
        memcpy(&(endpoint->ulDefaultIPAddress),&(ip_info->ip.s_addr), ipIP_ADDRESS_LENGTH_BYTES);
        FreeRTOS_AddEndPoint(net_if, endpoint);
    }    
    memcpy(&(endpoint->xMACAddress), mac, ipMAC_ADDRESS_LENGTH_BYTES);    
    memcpy(&(endpoint->ulIPAddress),&(ip_info->ip.s_addr), ipIP_ADDRESS_LENGTH_BYTES);
    memcpy(&(endpoint->ulNetMask),&(ip_info->netmask.s_addr), ipIP_ADDRESS_LENGTH_BYTES);
    memcpy(&(endpoint->ulGatewayAddress),&(ip_info->gw.s_addr), ipIP_ADDRESS_LENGTH_BYTES);
    if (endpoint->ulIPAddress == 0) {
        endpoint->bits.bWantDHCP = pdTRUE_UNSIGNED;
    }
    return ESP_OK;
}

esp_err_t tcpip_adapter_stop(tcpip_adapter_if_t tcpip_if)
{
    if (tcpip_if >= TCPIP_ADAPTER_IF_MAX) {
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    // first stop sending new eth packets to this interface
    esp_wifi_internal_reg_rxcb((wifi_interface_t)tcpip_if, NULL);
    // mark the interface as down
    IF_DATA(tcpip_if)->state = DOWN;
    // notify the tcp stack
    FreeRTOS_NetworkDown(NET_IF(tcpip_if));
        
    if (tcpip_if == TCPIP_ADAPTER_IF_AP) {
        //TODO: stop the dhcp server which we have started here.
    } else if (tcpip_if == TCPIP_ADAPTER_IF_STA || tcpip_if == TCPIP_ADAPTER_IF_ETH) {
        memset(&(IF_DATA(TCPIP_ADAPTER_IF_STA)->ip_info),0,sizeof(IF_DATA(TCPIP_ADAPTER_IF_STA)->ip_info));
    }
    
    return ESP_OK;
}

esp_err_t tcpip_adapter_up(tcpip_adapter_if_t tcpip_if)
{
    IF_DATA(tcpip_if)->state = UP;
    return ESP_OK;
}

// static esp_err_t tcpip_adapter_reset_ip_info(tcpip_adapter_if_t tcpip_if)
// {
//     ip4_addr_set_zero(&esp_ip[tcpip_if].ip);
//     ip4_addr_set_zero(&esp_ip[tcpip_if].gw);
//     ip4_addr_set_zero(&esp_ip[tcpip_if].netmask);
//     return ESP_OK;
// }

// esp_err_t tcpip_adapter_down(tcpip_adapter_if_t tcpip_if)
// {
//     if (tcpip_if == TCPIP_ADAPTER_IF_STA) {
//         if (esp_netif[tcpip_if] == NULL) {
//             return ESP_ERR_TCPIP_ADAPTER_IF_NOT_READY;
//         }

//         if (dhcpc_status[tcpip_if] == TCPIP_ADAPTER_DHCP_STARTED) {
//             tcpip_adapter_stop_dhcp(esp_netif[tcpip_if]);

//             dhcpc_status[tcpip_if] = TCPIP_ADAPTER_DHCP_INIT;

//             tcpip_adapter_reset_ip_info(tcpip_if);
//         }

// #if TCPIP_ADAPTER_IPV6
//         for(int8_t i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
//             netif_ip6_addr_set(esp_netif[tcpip_if], i, IP6_ADDR_ANY6);
//             netif_ip6_addr_set_state(esp_netif[tcpip_if], i, IP6_ADDR_INVALID);
//         }
// #endif
//         netif_set_addr(esp_netif[tcpip_if], IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
//         netif_set_down(esp_netif[tcpip_if]);
//         tcpip_adapter_start_ip_lost_timer(tcpip_if);
//     }

//     tcpip_adapter_update_default_netif();

//     return ESP_OK;
// }

esp_err_t tcpip_adapter_set_old_ip_info(tcpip_adapter_if_t tcpip_if, tcpip_adapter_ip_info_t *ip_info)
{
    if (tcpip_if >= TCPIP_ADAPTER_IF_MAX || ip_info == NULL) {
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    memcpy(&_if_data[tcpip_if].old_ip_info, ip_info, sizeof(tcpip_adapter_ip_info_t));

    return ESP_OK;
}

esp_err_t tcpip_adapter_get_old_ip_info(tcpip_adapter_if_t tcpip_if, tcpip_adapter_ip_info_t *ip_info)
{
    if (tcpip_if >= TCPIP_ADAPTER_IF_MAX || ip_info == NULL) {
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    memcpy(ip_info, &_if_data[tcpip_if].old_ip_info, sizeof(tcpip_adapter_ip_info_t));

    return ESP_OK;
}

esp_err_t tcpip_adapter_get_ip_info(tcpip_adapter_if_t tcpip_if, tcpip_adapter_ip_info_t *ip_info)
{
    if (tcpip_if >= TCPIP_ADAPTER_IF_MAX || ip_info == NULL) {
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    memcpy(ip_info, &_if_data[tcpip_if].ip_info, sizeof(tcpip_adapter_ip_info_t));

    return ESP_OK;
}

//     p_netif = esp_netif[tcpip_if];

//     if (p_netif != NULL && netif_is_up(p_netif)) {
//         ip4_addr_set(&ip_info->ip, ip_2_ip4(&p_netif->ip_addr));
//         ip4_addr_set(&ip_info->netmask, ip_2_ip4(&p_netif->netmask));
//         ip4_addr_set(&ip_info->gw, ip_2_ip4(&p_netif->gw));

//         return ESP_OK;
//     }

//     ip4_addr_copy(ip_info->ip, esp_ip[tcpip_if].ip);
//     ip4_addr_copy(ip_info->gw, esp_ip[tcpip_if].gw);
//     ip4_addr_copy(ip_info->netmask, esp_ip[tcpip_if].netmask);

//     return ESP_OK;
// }

esp_err_t tcpip_adapter_set_ip_info(tcpip_adapter_if_t tcpip_if, tcpip_adapter_ip_info_t *ip_info)
{    
    // if (tcpip_if >= TCPIP_ADAPTER_IF_MAX || ip_info == NULL) {
    //     return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    // }

    // if (tcpip_if == TCPIP_ADAPTER_IF_AP) {
    //     //tcpip_adapter_dhcps_get_status(tcpip_if, &status);

    //     // if (status != TCPIP_ADAPTER_DHCP_STOPPED) {
    //     //     return ESP_ERR_TCPIP_ADAPTER_DHCP_NOT_STOPPED;
    //     // }
    // } else if (tcpip_if == TCPIP_ADAPTER_IF_STA) {                
    //     tcpip_adapter_dhcpc_get_status(tcpip_if, &status);
        
    //     if (status != TCPIP_ADAPTER_DHCP_STOPPED) {
    //         return ESP_ERR_TCPIP_ADAPTER_DHCP_NOT_STOPPED;
    //     }
    // }
    // NetworkEndPoint_t* endpoint = FreeRTOS_FirstEndPoint(NET_IF(tcpip_if));
    
    // ip4_addr_copy(esp_ip[tcpip_if].ip, ip_info->ip);
    // ip4_addr_copy(esp_ip[tcpip_if].gw, ip_info->gw);
    // ip4_addr_copy(esp_ip[tcpip_if].netmask, ip_info->netmask);

    // p_netif = esp_netif[tcpip_if];

    // if (p_netif != NULL && netif_is_up(p_netif)) {
    //     netif_set_addr(p_netif, &ip_info->ip, &ip_info->netmask, &ip_info->gw);
    //     if (tcpip_if == TCPIP_ADAPTER_IF_STA) {
    //         if (!(ip4_addr_isany_val(ip_info->ip) || ip4_addr_isany_val(ip_info->netmask) || ip4_addr_isany_val(ip_info->gw))) {
    //             system_event_t evt;
    //             if (tcpip_if == TCPIP_ADAPTER_IF_STA) {
    //                 evt.event_id = SYSTEM_EVENT_STA_GOT_IP;
    //             } else if (tcpip_if == TCPIP_ADAPTER_IF_ETH) {
    //                 evt.event_id = SYSTEM_EVENT_ETH_GOT_IP;
    //             }
    //             evt.event_info.got_ip.ip_changed = false;

    //             if (memcmp(ip_info, &esp_ip_old[tcpip_if], sizeof(tcpip_adapter_ip_info_t))) {
    //                 evt.event_info.got_ip.ip_changed = true;
    //             }

    //             evt.event_info.got_ip.if_index = TCPIP_ADAPTER_IF_STA;
    //             memcpy(&evt.event_info.got_ip.ip_info, ip_info, sizeof(tcpip_adapter_ip_info_t));
    //             memcpy(&esp_ip_old[tcpip_if], ip_info, sizeof(tcpip_adapter_ip_info_t));
    //             esp_event_send(&evt);
    //             ESP_LOGD(TAG, "if%d tcpip adapter set static ip: ip changed=%d", tcpip_if, evt.event_info.got_ip.ip_changed);
    //         }
    //     }
    // }

    return ESP_OK;
}

// #if TCPIP_ADAPTER_IPV6
// static void tcpip_adapter_nd6_cb(struct netif *p_netif, uint8_t ip_idex)
// {
//     tcpip_adapter_ip6_info_t *ip6_info;

//     system_event_t evt;
//     //notify event

//     evt.event_id = SYSTEM_EVENT_GOT_IP6;

//     if (!p_netif) {
//         ESP_LOGD(TAG, "null p_netif=%p", p_netif);
//         return;
//     }

//     if (p_netif == esp_netif[TCPIP_ADAPTER_IF_STA]) {
//         ip6_info = &esp_ip6[TCPIP_ADAPTER_IF_STA];
//         evt.event_info.got_ip6.if_index = TCPIP_ADAPTER_IF_STA;
//     } else if (p_netif == esp_netif[TCPIP_ADAPTER_IF_AP]) {
//         ip6_info = &esp_ip6[TCPIP_ADAPTER_IF_AP];
//         evt.event_info.got_ip6.if_index = TCPIP_ADAPTER_IF_AP;
//     } else if (p_netif == esp_netif[TCPIP_ADAPTER_IF_ETH]) {
//         ip6_info = &esp_ip6[TCPIP_ADAPTER_IF_ETH];
//         evt.event_info.got_ip6.if_index = TCPIP_ADAPTER_IF_ETH;
//     } else {
//         return;
//     }

//     ip6_addr_set(&ip6_info->ip, ip_2_ip6(&p_netif->ip6_addr[ip_idex]));

//     memcpy(&evt.event_info.got_ip6.ip6_info, ip6_info, sizeof(tcpip_adapter_ip6_info_t));
//     esp_event_send(&evt);
// }
// #endif

// #if TCPIP_ADAPTER_IPV6
// esp_err_t tcpip_adapter_create_ip6_linklocal(tcpip_adapter_if_t tcpip_if)
// {
//     struct netif *p_netif;

//     if (tcpip_if >= TCPIP_ADAPTER_IF_MAX) {
//         return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
//     }

//     p_netif = esp_netif[tcpip_if];
//     if (p_netif != NULL && netif_is_up(p_netif)) {
//         netif_create_ip6_linklocal_address(p_netif, 1);
//         /*TODO need add ipv6 address cb*/
//         nd6_set_cb(p_netif, tcpip_adapter_nd6_cb);

//         return ESP_OK;
//     } else {
//         return ESP_FAIL;
//     }
// }

// esp_err_t tcpip_adapter_get_ip6_linklocal(tcpip_adapter_if_t tcpip_if, ip6_addr_t *if_ip6)
// {
//     struct netif *p_netif;

//     if (tcpip_if >= TCPIP_ADAPTER_IF_MAX || if_ip6 == NULL) {
//         return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
//     }

//     p_netif = esp_netif[tcpip_if];
//     if (p_netif != NULL && netif_is_up(p_netif) && ip6_addr_ispreferred(netif_ip6_addr_state(p_netif, 0))) {
//         memcpy(if_ip6, &p_netif->ip6_addr[0], sizeof(ip6_addr_t));
//     } else {
//         return ESP_FAIL;
//     }
//     return ESP_OK;
// }

// esp_err_t tcpip_adapter_get_ip6_global(tcpip_adapter_if_t tcpip_if, ip6_addr_t *if_ip6)
// {
//     struct netif *p_netif;
//     int i;

//     if (tcpip_if >= TCPIP_ADAPTER_IF_MAX || if_ip6 == NULL) {
//         return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
//     }

//     p_netif = esp_netif[tcpip_if];
//     if (p_netif != NULL && netif_is_up(p_netif)) {
//         for (i = 1; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
//             if (ip6_addr_ispreferred(netif_ip6_addr_state(p_netif, i))) {
//                 memcpy(if_ip6, &p_netif->ip6_addr[i], sizeof(ip6_addr_t));
//                 return ESP_OK;
//             }
//         }
//     }

//     return ESP_FAIL;
// }
// #endif

esp_err_t tcpip_adapter_dhcps_option(tcpip_adapter_option_mode_t opt_op, tcpip_adapter_option_id_t opt_id, void *opt_val, uint32_t opt_len)
{
//     void *opt_info = dhcps_option_info(opt_id, opt_len);

//     if (opt_info == NULL || opt_val == NULL) {
//         return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
//     }

//     if (opt_op == TCPIP_ADAPTER_OP_GET) {
//         if (dhcps_status == TCPIP_ADAPTER_DHCP_STOPPED) {
//             return ESP_ERR_TCPIP_ADAPTER_DHCP_ALREADY_STOPPED;
//         }

//         switch (opt_id) {
//         case IP_ADDRESS_LEASE_TIME: {
//             *(uint32_t *)opt_val = *(uint32_t *)opt_info;
//             break;
//         }
//         case SUBNET_MASK:
//         case REQUESTED_IP_ADDRESS: {
//             memcpy(opt_val, opt_info, opt_len);
//             break;
//         }
//         case ROUTER_SOLICITATION_ADDRESS: {
//             if ((*(uint8_t *)opt_info) & OFFER_ROUTER) {
//                 *(uint8_t *)opt_val = 1;
//             } else {
//                 *(uint8_t *)opt_val = 0;
//             }
//             break;
//         }
//         case DOMAIN_NAME_SERVER: {
//             if ((*(uint8_t *)opt_info) & OFFER_DNS) {
//                 *(uint8_t *)opt_val = 1;
//             } else {
//                 *(uint8_t *)opt_val = 0;
//             }
//             break;
//         }
//         default:
//             break;
//         }
//     } else if (opt_op == TCPIP_ADAPTER_OP_SET) {
//         if (dhcps_status == TCPIP_ADAPTER_DHCP_STARTED) {
//             return ESP_ERR_TCPIP_ADAPTER_DHCP_ALREADY_STARTED;
//         }

//         switch (opt_id) {
//         case IP_ADDRESS_LEASE_TIME: {
//             if (*(uint32_t *)opt_val != 0) {
//                 *(uint32_t *)opt_info = *(uint32_t *)opt_val;
//             } else {
//                 *(uint32_t *)opt_info = DHCPS_LEASE_TIME_DEF;
//             }
//             break;
//         }
//         case SUBNET_MASK: {
//             memcpy(opt_info, opt_val, opt_len);
//             break;
//         }
//         case REQUESTED_IP_ADDRESS: {
//             tcpip_adapter_ip_info_t info;
//             uint32_t softap_ip = 0;
//             uint32_t start_ip = 0;
//             uint32_t end_ip = 0;
//             dhcps_lease_t *poll = opt_val;

//             if (poll->enable) {
//                 memset(&info, 0x00, sizeof(tcpip_adapter_ip_info_t));
//                 tcpip_adapter_get_ip_info((tcpip_adapter_if_t)ESP_IF_WIFI_AP, &info);
//                 softap_ip = htonl(info.ip.addr);
//                 start_ip = htonl(poll->start_ip.addr);
//                 end_ip = htonl(poll->end_ip.addr);

//                 /*config ip information can't contain local ip*/
//                 if ((start_ip <= softap_ip) && (softap_ip <= end_ip)) {
//                     return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
//                 }

//                 /*config ip information must be in the same segment as the local ip*/
//                 softap_ip >>= 8;
//                 if ((start_ip >> 8 != softap_ip)
//                         || (end_ip >> 8 != softap_ip)) {
//                     return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
//                 }

//                 if (end_ip - start_ip > DHCPS_MAX_LEASE) {
//                     return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
//                 }
//             }

//             memcpy(opt_info, opt_val, opt_len);
//             break;
//         }
//         case ROUTER_SOLICITATION_ADDRESS: {
//             if (*(uint8_t *)opt_val) {
//                 *(uint8_t *)opt_info |= OFFER_ROUTER;
//             } else {
//                 *(uint8_t *)opt_info &= ((~OFFER_ROUTER)&0xFF);
//             }
//             break;
//         }
//         case DOMAIN_NAME_SERVER: {
//             if (*(uint8_t *)opt_val) {
//                 *(uint8_t *)opt_info |= OFFER_DNS;
//             } else {
//                 *(uint8_t *)opt_info &= ((~OFFER_DNS)&0xFF);
//             }
//             break;
//         }
       
//         default:
//             break;
//         }
//         dhcps_set_option_info(opt_id, opt_info,opt_len);
//     } else {
//         return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
//     }

    return ESP_OK;
}

esp_err_t tcpip_adapter_set_dns_info(tcpip_adapter_if_t tcpip_if, tcpip_adapter_dns_type_t type, tcpip_adapter_dns_info_t *dns)
{
    // if (tcpip_if >= TCPIP_ADAPTER_IF_MAX) {
    //     ESP_LOGD(TAG, "set dns invalid if=%d", tcpip_if);
    //     return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    // }
 
    // if (!dns) {
    //     ESP_LOGD(TAG, "set dns null dns");
    //     return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    // }

    // if (type >= TCPIP_ADAPTER_DNS_MAX) {
    //     ESP_LOGD(TAG, "set dns invalid type=%d", type);
    //     return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    // }

    // if (ip4_addr_isany_val(*ip_2_ip4(&(dns->ip)))) {
    //     ESP_LOGD(TAG, "set dns invalid dns");
    //     return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    // }

    // ESP_LOGD(TAG, "set dns if=%d type=%d dns=%x", tcpip_if, type, ip_2_ip4(&(dns->ip))->addr);
    // IP_SET_TYPE_VAL(dns->ip, IPADDR_TYPE_V4);

    // if (tcpip_if == TCPIP_ADAPTER_IF_STA || tcpip_if == TCPIP_ADAPTER_IF_ETH) {
    //     dns_setserver(type, &(dns->ip));
    // } else {
    //     if (type != TCPIP_ADAPTER_DNS_MAIN) {
    //         ESP_LOGD(TAG, "set dns invalid type");
    //         return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    //     } else {
    //         dhcps_dns_setserver(&(dns->ip));
    //     }
    // }

    return ESP_OK;
}

esp_err_t tcpip_adapter_get_dns_info(tcpip_adapter_if_t tcpip_if, tcpip_adapter_dns_type_t type, tcpip_adapter_dns_info_t *dns)
{ 
    // const ip_addr_t *ns;

    // if (!dns) {
    //     ESP_LOGD(TAG, "get dns null dns");
    //     return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    // }

    // if (type >= TCPIP_ADAPTER_DNS_MAX) {
    //     ESP_LOGD(TAG, "get dns invalid type=%d", type);
    //     return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    // }
    
    // if (tcpip_if >= TCPIP_ADAPTER_IF_MAX) {
    //     ESP_LOGD(TAG, "get dns invalid tcpip_if=%d",tcpip_if);
    //     return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    // }

    // if (tcpip_if == TCPIP_ADAPTER_IF_STA || tcpip_if == TCPIP_ADAPTER_IF_ETH) {
    //     ns = dns_getserver(type);
    //     dns->ip = *ns;
    // } else {
    //     *ip_2_ip4(&(dns->ip)) = dhcps_dns_getserver();
    // }

    return ESP_OK;
}

esp_err_t tcpip_adapter_dhcps_get_status(tcpip_adapter_if_t tcpip_if, tcpip_adapter_dhcp_status_t *status)
{
    //*status = dhcps_status;

    return ESP_OK;
}

esp_err_t tcpip_adapter_dhcps_start(tcpip_adapter_if_t tcpip_if)
{
//     /* only support ap now */
//     if (tcpip_if != TCPIP_ADAPTER_IF_AP || tcpip_if >= TCPIP_ADAPTER_IF_MAX) {
//         ESP_LOGD(TAG, "dhcp server invalid if=%d", tcpip_if);
//         return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
//     }

//     if (dhcps_status != TCPIP_ADAPTER_DHCP_STARTED) {
//         struct netif *p_netif = esp_netif[tcpip_if];

//         if (p_netif != NULL && netif_is_up(p_netif)) {
//             tcpip_adapter_ip_info_t default_ip;
//             tcpip_adapter_get_ip_info((tcpip_adapter_if_t)ESP_IF_WIFI_AP, &default_ip);
//             dhcps_start(p_netif, default_ip.ip);
//             dhcps_status = TCPIP_ADAPTER_DHCP_STARTED;
//             ESP_LOGD(TAG, "dhcp server start successfully");
//             return ESP_OK;
//         } else {
//             ESP_LOGD(TAG, "dhcp server re init");
//             dhcps_status = TCPIP_ADAPTER_DHCP_INIT;
//             return ESP_OK;
//         }
//     }

//     ESP_LOGD(TAG, "dhcp server already start");
// return ESP_ERR_TCPIP_ADAPTER_DHCP_ALREADY_STARTED;
    return ESP_OK;
}

esp_err_t tcpip_adapter_dhcps_stop(tcpip_adapter_if_t tcpip_if)
{
//     /* only support ap now */
//     if (tcpip_if != TCPIP_ADAPTER_IF_AP || tcpip_if >= TCPIP_ADAPTER_IF_MAX) {
//         ESP_LOGD(TAG, "dhcp server invalid if=%d", tcpip_if);
//         return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
//     }

//     if (dhcps_status == TCPIP_ADAPTER_DHCP_STARTED) {
//         struct netif *p_netif = esp_netif[tcpip_if];

//         if (p_netif != NULL) {
//             dhcps_stop(p_netif);
//         } else {
//             ESP_LOGD(TAG, "dhcp server if not ready");
//             return ESP_ERR_TCPIP_ADAPTER_IF_NOT_READY;
//         }
//     } else if (dhcps_status == TCPIP_ADAPTER_DHCP_STOPPED) {
//         ESP_LOGD(TAG, "dhcp server already stoped");
//         return ESP_ERR_TCPIP_ADAPTER_DHCP_ALREADY_STOPPED;
//     }

//     ESP_LOGD(TAG, "dhcp server stop successfully");
//     dhcps_status = TCPIP_ADAPTER_DHCP_STOPPED;
    return ESP_OK;
}

esp_err_t tcpip_adapter_dhcpc_option(tcpip_adapter_option_mode_t opt_op, tcpip_adapter_option_id_t opt_id, void *opt_val, uint32_t opt_len)
{
//     // TODO: when dhcp request timeout,change the retry count
    return ESP_ERR_NOT_SUPPORTED;
}

// static void tcpip_adapter_dhcpc_cb(struct netif *netif)
// {
//     tcpip_adapter_ip_info_t *ip_info_old = NULL;
//     tcpip_adapter_ip_info_t *ip_info = NULL;
//     tcpip_adapter_if_t tcpip_if;

//     if (!netif) {
//         ESP_LOGD(TAG, "null netif=%p", netif);
//         return;
//     }

//     if( netif == esp_netif[TCPIP_ADAPTER_IF_STA] ) {
//         tcpip_if = TCPIP_ADAPTER_IF_STA;
//     } else { 
//         ESP_LOGD(TAG, "err netif=%p", netif);
//         return;
//     }

//     ESP_LOGD(TAG, "if%d dhcpc cb", tcpip_if);
//     ip_info = &esp_ip[tcpip_if];
//     ip_info_old = &esp_ip_old[tcpip_if];

//     if ( !ip_addr_isany_val(netif->ip_addr) ) {
        
//         //check whether IP is changed
//         if ( !ip4_addr_cmp(ip_2_ip4(&netif->ip_addr), &ip_info->ip) ||
//                 !ip4_addr_cmp(ip_2_ip4(&netif->netmask), &ip_info->netmask) ||
//                 !ip4_addr_cmp(ip_2_ip4(&netif->gw), &ip_info->gw) ) {
//             system_event_t evt;

//             ip4_addr_set(&ip_info->ip, ip_2_ip4(&netif->ip_addr));
//             ip4_addr_set(&ip_info->netmask, ip_2_ip4(&netif->netmask));
//             ip4_addr_set(&ip_info->gw, ip_2_ip4(&netif->gw));

//             //notify event
//             evt.event_id = SYSTEM_EVENT_STA_GOT_IP;
//             evt.event_info.got_ip.ip_changed = false;

//             if (memcmp(ip_info, ip_info_old, sizeof(tcpip_adapter_ip_info_t))) {
//                 evt.event_info.got_ip.ip_changed = true;
//             }

//             memcpy(&evt.event_info.got_ip.ip_info, ip_info, sizeof(tcpip_adapter_ip_info_t));
//             memcpy(ip_info_old, ip_info, sizeof(tcpip_adapter_ip_info_t));
//             ESP_LOGD(TAG, "if%d ip changed=%d", tcpip_if, evt.event_info.got_ip.ip_changed);
//             esp_event_send(&evt);
//         } else {
//             ESP_LOGD(TAG, "if%d ip unchanged", CONFIG_IP_LOST_TIMER_INTERVAL);
//         }
//     } else {
//         if (!ip4_addr_isany_val(ip_info->ip)) {
//             tcpip_adapter_start_ip_lost_timer(tcpip_if);
//         }
//     }

//     return;
// }

// static esp_err_t tcpip_adapter_start_ip_lost_timer(tcpip_adapter_if_t tcpip_if)
// {
//     tcpip_adapter_ip_info_t *ip_info_old = &esp_ip_old[tcpip_if];
//     struct netif *netif = esp_netif[tcpip_if];

//     ESP_LOGD(TAG, "if%d start ip lost tmr: enter", tcpip_if);
//     if (tcpip_if != TCPIP_ADAPTER_IF_STA) {
//         ESP_LOGD(TAG, "if%d start ip lost tmr: only sta support ip lost timer", tcpip_if);
//         return ESP_OK;
//     }

//     if (esp_ip_lost_timer[tcpip_if].timer_running) {
//         ESP_LOGD(TAG, "if%d start ip lost tmr: already started", tcpip_if);
//         return ESP_OK;
//     }

//     if ( netif && (CONFIG_IP_LOST_TIMER_INTERVAL > 0) && !ip4_addr_isany_val(ip_info_old->ip)) {
//         esp_ip_lost_timer[tcpip_if].timer_running = true;
//         sys_timeout(CONFIG_IP_LOST_TIMER_INTERVAL*1000, tcpip_adapter_ip_lost_timer, (void*)tcpip_if);
//         ESP_LOGD(TAG, "if%d start ip lost tmr: interval=%d", tcpip_if, CONFIG_IP_LOST_TIMER_INTERVAL);
//         return ESP_OK;
//     }

//     ESP_LOGD(TAG, "if%d start ip lost tmr: no need start because netif=%p interval=%d ip=%x", 
//                   tcpip_if, netif, CONFIG_IP_LOST_TIMER_INTERVAL, ip_info_old->ip.addr);

//     return ESP_OK;
// }

// static void tcpip_adapter_ip_lost_timer(void *arg)
// {
//     tcpip_adapter_if_t tcpip_if = (tcpip_adapter_if_t)arg;

//     ESP_LOGD(TAG, "if%d ip lost tmr: enter", tcpip_if);

//     if (esp_ip_lost_timer[tcpip_if].timer_running == false) {
//         ESP_LOGD(TAG, "ip lost time is not running");
//         return;
//     }

//     esp_ip_lost_timer[tcpip_if].timer_running = false;

//     if (tcpip_if == TCPIP_ADAPTER_IF_STA) {
//         struct netif *netif = esp_netif[tcpip_if];

//         if ( (!netif) || (netif && ip_addr_isany_val(netif->ip_addr))){
//             system_event_t evt;

//             ESP_LOGD(TAG, "if%d ip lost tmr: raise ip lost event", tcpip_if);
//             memset(&esp_ip_old[tcpip_if], 0, sizeof(tcpip_adapter_ip_info_t));
//             evt.event_id = SYSTEM_EVENT_STA_LOST_IP;
//             esp_event_send(&evt);
//         } else {
//             ESP_LOGD(TAG, "if%d ip lost tmr: no need raise ip lost event", tcpip_if);
//         }
//     } else {
//         ESP_LOGD(TAG, "if%d ip lost tmr: not station", tcpip_if);
//     }
// }

esp_err_t tcpip_adapter_dhcpc_get_status(tcpip_adapter_if_t tcpip_if, tcpip_adapter_dhcp_status_t *status)
{
//     *status = dhcpc_status[tcpip_if];

    return ESP_OK;
}

esp_err_t tcpip_adapter_dhcpc_start(tcpip_adapter_if_t tcpip_if)
{
//     if ((tcpip_if != TCPIP_ADAPTER_IF_STA)  || tcpip_if >= TCPIP_ADAPTER_IF_MAX) {
//         ESP_LOGD(TAG, "dhcp client invalid if=%d", tcpip_if);
//         return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
//     }

//     if (dhcpc_status[tcpip_if] != TCPIP_ADAPTER_DHCP_STARTED) {
//         struct netif *p_netif = esp_netif[tcpip_if];

//         tcpip_adapter_reset_ip_info(tcpip_if);
// #if LWIP_DNS
//         //dns_clear_servers(true);
// #endif

//         if (p_netif != NULL) {
//             if (netif_is_up(p_netif)) {
//                 ESP_LOGD(TAG, "dhcp client init ip/mask/gw to all-0");
//                 ip_addr_set_zero(&p_netif->ip_addr);
//                 ip_addr_set_zero(&p_netif->netmask);
//                 ip_addr_set_zero(&p_netif->gw);
//                 tcpip_adapter_start_ip_lost_timer(tcpip_if);
//             } else {
//                 ESP_LOGD(TAG, "dhcp client re init");
//                 dhcpc_status[tcpip_if] = TCPIP_ADAPTER_DHCP_INIT;
//                 return ESP_OK;
//             }

//             if (tcpip_adapter_start_dhcp(p_netif) != ERR_OK) {
//                 ESP_LOGD(TAG, "dhcp client start failed");
//                 return ESP_ERR_TCPIP_ADAPTER_DHCPC_START_FAILED;
//             }

//             dhcp_fail_time = 0;
//             xTimerReset(dhcp_check_timer, 0);
//             ESP_LOGD(TAG, "dhcp client start successfully");
//             dhcpc_status[tcpip_if] = TCPIP_ADAPTER_DHCP_STARTED;
//             return ESP_OK;
//         } else {
//             ESP_LOGD(TAG, "dhcp client re init");
//             dhcpc_status[tcpip_if] = TCPIP_ADAPTER_DHCP_INIT;
//             return ESP_OK;
//         }
//     }

//     ESP_LOGD(TAG, "dhcp client already started");
//     return ESP_ERR_TCPIP_ADAPTER_DHCP_ALREADY_STARTED;
    return ESP_OK;
}

esp_err_t tcpip_adapter_dhcpc_stop(tcpip_adapter_if_t tcpip_if)
{
//     if (tcpip_if != TCPIP_ADAPTER_IF_STA || tcpip_if >= TCPIP_ADAPTER_IF_MAX) {
//         ESP_LOGD(TAG, "dhcp client invalid if=%d", tcpip_if);
//         return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
//     }

//     esp_ip_lost_timer[tcpip_if].timer_running = false;

//     if (dhcpc_status[tcpip_if] == TCPIP_ADAPTER_DHCP_STARTED) {
//         struct netif *p_netif = esp_netif[tcpip_if];

//         if (p_netif != NULL) {
//             tcpip_adapter_stop_dhcp(p_netif);
//             tcpip_adapter_reset_ip_info(tcpip_if);
//         } else {
//             ESP_LOGD(TAG, "dhcp client if not ready");
//             return ESP_ERR_TCPIP_ADAPTER_IF_NOT_READY;
//         }
//     } else if (dhcpc_status[tcpip_if] == TCPIP_ADAPTER_DHCP_STOPPED) {
//         ESP_LOGD(TAG, "dhcp client already stoped");
//         return ESP_ERR_TCPIP_ADAPTER_DHCP_ALREADY_STOPPED;
//     }

//     ESP_LOGD(TAG, "dhcp client stop successfully");
//     dhcpc_status[tcpip_if] = TCPIP_ADAPTER_DHCP_STOPPED;
    return ESP_OK;
}

esp_interface_t tcpip_adapter_get_esp_if(void *dev)
{
    NetworkInterface_t* net_if = (NetworkInterface_t *)dev;
    return (esp_interface_t)IF_NAME_FROM_IF(net_if);   
}

esp_err_t tcpip_adapter_get_sta_list(wifi_sta_list_t *wifi_sta_list, tcpip_adapter_sta_list_t *tcpip_sta_list)
{
//     int i;

//     if ((wifi_sta_list == NULL) || (tcpip_sta_list == NULL)) {
//         return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
//     }

//     memset(tcpip_sta_list, 0, sizeof(tcpip_adapter_sta_list_t));
//     tcpip_sta_list->num = wifi_sta_list->num;
//     for (i = 0; i < wifi_sta_list->num; i++) {
//         memcpy(tcpip_sta_list->sta[i].mac, wifi_sta_list->sta[i].mac, 6);
//         dhcp_search_ip_on_mac(tcpip_sta_list->sta[i].mac, &tcpip_sta_list->sta[i].ip);
//     }

    return ESP_OK;
}

esp_err_t tcpip_adapter_set_hostname(tcpip_adapter_if_t tcpip_if, const char *hostname)
{
// #if LWIP_NETIF_HOSTNAME
//     struct netif *p_netif;
//     static char hostinfo[TCPIP_ADAPTER_IF_MAX][TCPIP_HOSTNAME_MAX_SIZE + 1];

//     if (tcpip_if >= TCPIP_ADAPTER_IF_MAX || hostname == NULL) {
//         return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
//     }

//     if (strlen(hostname) > TCPIP_HOSTNAME_MAX_SIZE) {
//         return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
//     }

//     p_netif = esp_netif[tcpip_if];
//     if (p_netif != NULL) {
//         memset(hostinfo[tcpip_if], 0, sizeof(hostinfo[tcpip_if]));
//         strlcpy(hostinfo[tcpip_if], hostname, sizeof(hostinfo[tcpip_if]));
//         p_netif->hostname = hostinfo[tcpip_if];
//         return ESP_OK;
//     } else {
//         return ESP_ERR_TCPIP_ADAPTER_IF_NOT_READY;
//     }
// #else
//     return ESP_ERR_TCPIP_ADAPTER_IF_NOT_READY;
// #endif
    return ESP_OK;
}

esp_err_t tcpip_adapter_get_hostname(tcpip_adapter_if_t tcpip_if, const char **hostname)
{
// #if LWIP_NETIF_HOSTNAME
//     struct netif *p_netif = NULL;
//     if (tcpip_if >= TCPIP_ADAPTER_IF_MAX || hostname == NULL) {
//         return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
//     }

//     p_netif = esp_netif[tcpip_if];
//     if (p_netif != NULL && p_netif->hostname != NULL) {
//         *hostname = p_netif->hostname;
//         return ESP_OK;
//     } else {
//         return ESP_ERR_TCPIP_ADAPTER_IF_NOT_READY;
//     }
// #else
//     return ESP_ERR_TCPIP_ADAPTER_IF_NOT_READY;
// #endif
    return ESP_OK;
}

esp_err_t tcpip_adapter_get_netif(tcpip_adapter_if_t tcpip_if, void ** netif)
{
    if (tcpip_if >= TCPIP_ADAPTER_IF_MAX) {
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    // *netif = esp_netif[tcpip_if];

    // if (*netif == NULL) {
    //     return ESP_ERR_TCPIP_ADAPTER_IF_NOT_READY;
    // }
    return ESP_OK;
}

bool tcpip_adapter_is_netif_up(tcpip_adapter_if_t tcpip_if)
{
//     if (esp_netif[tcpip_if] != NULL && netif_is_up(esp_netif[tcpip_if])) {
//         return true;
//     } else {
//         return false;
//     }
    return false;
}

int tcpip_adapter_get_netif_index(tcpip_adapter_if_t tcpip_if)
{
//     if (tcpip_if >= TCPIP_ADAPTER_IF_MAX || esp_netif[tcpip_if] == NULL) {
//         return -1;
//     }
//     return netif_get_index(esp_netif[tcpip_if]);
    return 0;
}

