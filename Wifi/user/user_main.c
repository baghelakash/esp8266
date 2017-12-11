#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "eagle_soc.h"
#include "c_types.h"
#include "../include/user_config.h"
#include "uart.h"
#include "user_interface.h"
#include "espconn.h"

struct espconn conn1;
esp_tcp tcp1;
uint32 SCREEN_CURSOR = 0;

void ICACHE_FLASH_ATTR tcp_server_sent_cb(void *arg) {
	os_printf("tcp sent cb \r\n");
}

void ICACHE_FLASH_ATTR tcp_server_recon_cb(void *arg, sint8 err) {
	os_printf("reconnect callback, error code %d !!! \r\n", err);
}

void ICACHE_FLASH_ATTR tcp_server_recv_cb(void *arg, char *pusrdata,
		unsigned short length) {
	struct espconn *pespconn = arg;
	if (SCREEN_CURSOR <= 0x3C00) {
		SCREEN_CURSOR += (length / 4);
	}
	os_printf("%d\r\n", SCREEN_CURSOR);
	os_printf("tcp recv : %s length : %d\r\n", pusrdata, length);
	//espconn_send(pespconn, pusrdata, length);
}

void ICACHE_FLASH_ATTR tcp_server_discon_cb(void *arg) {
	os_printf("tcp disconnect succeed !!! \r\n");
}

//Callback function for incoming TCP connection
void ICACHE_FLASH_ATTR TCPconnectCB(void *arg) {
	struct espconn *pesp_conn = arg;
	os_printf("tcp_server_listen !!! \r\n");
	espconn_regist_recvcb(pesp_conn, tcp_server_recv_cb);
	espconn_regist_reconcb(pesp_conn, tcp_server_recon_cb);
	espconn_regist_disconcb(pesp_conn, tcp_server_discon_cb);
	espconn_regist_sentcb(pesp_conn, tcp_server_sent_cb);
}

//Wifi event handler function
void ICACHE_FLASH_ATTR wifi_handle_event_cb(System_Event_t *evt) {
	switch (evt->event) {
	case EVENT_STAMODE_CONNECTED:
		break;
	case EVENT_STAMODE_DISCONNECTED:
		break;
	case EVENT_STAMODE_AUTHMODE_CHANGE:
		break;
	case EVENT_STAMODE_GOT_IP:
		tcp1.local_port = 25867;
		conn1.type = ESPCONN_TCP;
		conn1.state = ESPCONN_NONE;
		conn1.proto.tcp = &tcp1;
		espconn_regist_connectcb(&conn1, TCPconnectCB);
		espconn_accept(&conn1);
		break;
	case EVENT_SOFTAPMODE_STACONNECTED:
		break;
	case EVENT_SOFTAPMODE_STADISCONNECTED:
		break;
	default:
		break;
	}
}

//Wifi AP configuration information structure
void ICACHE_FLASH_ATTR user_set_AP_configuration(void) {
	struct softap_config routerparm;
	wifi_softap_get_config(&routerparm);
	os_memset(routerparm.ssid, 0, 32);
	os_memset(routerparm.password, 0, 64);
	os_memcpy(routerparm.ssid, "ESP8266", 7);
	os_memcpy(routerparm.password, "12345678", 8);
	routerparm.authmode = AUTH_WPA_WPA2_PSK;
	routerparm.ssid_len = 0; // Or its actual length
	routerparm.max_connection = 4; // Max no of stations allowed
	wifi_softap_set_config(&routerparm);
}

//Wifi station configuration information structure
void ICACHE_FLASH_ATTR user_set_station_configuration(void) {
	wifi_station_set_auto_connect(0);
	wifi_station_dhcpc_set_maxtry(5);
	wifi_station_ap_number_set(5);
	wifi_station_set_reconnect_policy(1);
	char ssid[32] = "OnePlus3t";
	char password[64] = "zaq1xsw2";
	struct station_config routerparm;
	routerparm.bssid_set = 0;
	os_memcpy(&routerparm.ssid, ssid, 11);
	os_memcpy(&routerparm.password, password, 64);
	wifi_station_set_config(&routerparm);
}

//WIFI scan done callback function
void scanCB(void *arg, STATUS status) {
	struct bss_info *bssInfo;
	bssInfo = (struct bss_info *) arg;
	while (bssInfo != NULL ) {
		os_printf("ssid: %s\n", bssInfo->ssid);
		bssInfo = STAILQ_NEXT(bssInfo, next);
	}
}

//User init done callback
void ICACHE_FLASH_ATTR post_user_init_func() {
//Wifi stuff
	wifi_station_connect();
	wifi_station_scan(NULL, scanCB);
	os_printf("post_user_init_func() done\r\n");
}

//Init function 
void ICACHE_FLASH_ATTR user_init(void) {

	system_update_cpu_freq(160); // Set CPU frequency to 160Mhz

//UART stuff
	uart_init(115200, 115200); // Initialize UART0 to use as debug

//Print RAM info on startup
	system_print_meminfo();

//Wifi stuff
	wifi_set_opmode(STATIONAP_MODE); // Set access point + station mode
	user_set_AP_configuration(); // Configure SoftAP parameters
	user_set_station_configuration(); // configure target network parameters and connect
	wifi_set_event_handler_cb(wifi_handle_event_cb);

//Register a callback function to let user code know that system initialization is complete
	system_init_done_cb(&post_user_init_func);
	os_printf("user_init() done\r\n");
}
