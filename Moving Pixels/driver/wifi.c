#include "../include/user_config.h"

struct espconn conn1;
struct espconn *TCP_CONNECTION_ID;
esp_tcp tcp1;
extern uint32_t volatile screenMatrix[2][Resolution * 4];
extern uint32_t volatile FRAME_BUFFER_CURSOR, OVERFLOW_BUFFER_DATA_LENGTH;
extern uint8 volatile CURRENT_FRAMEBUFFER, OVERFLOW_BUFFER[1460 * 5];
extern FLAG FLAGS;

//Callback function to indicate TCP data sent
void ICACHE_FLASH_ATTR tcp_server_sent_cb(void *arg) {
	os_printf("tcp sent cb \r\n");
}

//Callback function to indicate TCP data sent
void ICACHE_FLASH_ATTR tcp_server_write_finish_cb(void *arg) {
	os_printf("tcp write finish cb \r\n");
}

//Callback function to handle TCP network errors
void ICACHE_FLASH_ATTR tcp_server_recon_cb(void *arg, sint8 err) {
	os_printf("reconnect callback, error code %d !!! \r\n", err);
}

//Callback function to handle TCP received data
void tcp_server_recv_cb(void *arg, char *pusrdata, unsigned short length) {
	TCP_CONNECTION_ID = arg;
	if (length == 2) {
		char mode[2];
		strcpy(mode, pusrdata);
		if (mode[0] == 'M') {
			switch (mode[1]) {
			case '0':
				MODE = POWER_OFF;
				break;
			case '1':
				MODE = AUTOPILOT;
				break;
			case '2':
				MODE = VIDEO_FROM_FLASH;
				break;
			case '3':
				MODE = VIDEO_UPLOAD_FLASH;
				break;
			case '4':
				MODE = VIDEO_STREAM_WIFI;
				break;
			case '5':
				MODE = IMAGE_RAM_FEED;
				break;
			case '6':
				MODE = SCRATCHPAD;
				break;
			case '7':
				MODE = PARTICLE_PHYSICS;
				break;
			}
			os_printf("%d\r\n", MODE);
		}
	}
	if (MODE == IMAGE_RAM_FEED && length > 2) {
		os_memcpy(
				(uint32 *) (&screenMatrix[!CURRENT_FRAMEBUFFER][0]
						+ FRAME_BUFFER_CURSOR / 4), (uint32 *) pusrdata,
				length);
		FRAME_BUFFER_CURSOR += length;
		if (FRAME_BUFFER_CURSOR >= 15360) {
			FRAME_BUFFER_CURSOR = 0;
			FLAGS.BACKGOUND_DATA_READY_FLAG = 1;
		}
	} else if (MODE == VIDEO_STREAM_WIFI && length > 2) {
		if (FRAME_BUFFER_CURSOR >= 8060) {
			espconn_recv_hold(TCP_CONNECTION_ID);
		}
		if (FLAGS.BACKGOUND_DATA_READY_FLAG
				&& FLAGS.OVERFLOW_BUFFER_DATA_READY_FLAG) {
			os_memcpy(
					(uint32 *) (OVERFLOW_BUFFER + OVERFLOW_BUFFER_DATA_LENGTH),
					(uint32 *) pusrdata, length);
			OVERFLOW_BUFFER_DATA_LENGTH += length;
			FRAME_BUFFER_CURSOR = OVERFLOW_BUFFER_DATA_LENGTH;
		}
		if ((FRAME_BUFFER_CURSOR + length) >= 15360) {
			os_memcpy(
					(uint32 *) (&screenMatrix[!CURRENT_FRAMEBUFFER][0]
							+ FRAME_BUFFER_CURSOR / 4), (uint32 *) pusrdata,
					(15360 - FRAME_BUFFER_CURSOR));
			OVERFLOW_BUFFER_DATA_LENGTH = ((FRAME_BUFFER_CURSOR + length)
					- 15360);
			os_memcpy((uint32 *) OVERFLOW_BUFFER,
					(uint32 *) (pusrdata + 15360 - FRAME_BUFFER_CURSOR),
					OVERFLOW_BUFFER_DATA_LENGTH);
			FRAME_BUFFER_CURSOR = OVERFLOW_BUFFER_DATA_LENGTH;
			FLAGS.BACKGOUND_DATA_READY_FLAG = 1;
			FLAGS.OVERFLOW_BUFFER_DATA_READY_FLAG = 1;
		}
		if (!FLAGS.BACKGOUND_DATA_READY_FLAG) {
			if (FLAGS.OVERFLOW_BUFFER_DATA_READY_FLAG) {
				os_memcpy((uint32*) (&screenMatrix[!CURRENT_FRAMEBUFFER][0]),
						(uint32*) &OVERFLOW_BUFFER,
						OVERFLOW_BUFFER_DATA_LENGTH);
				FLAGS.OVERFLOW_BUFFER_DATA_READY_FLAG = 0;
			}
			os_memcpy(
					(uint32 *) (&screenMatrix[!CURRENT_FRAMEBUFFER][0]
							+ FRAME_BUFFER_CURSOR / 4), (uint32 *) pusrdata,
					length);
			FRAME_BUFFER_CURSOR += length;
		}
	}
	/*os_printf(
	 "counter: %d, length: %d, cursor: %d, OBlength: %d, OBFLAG: %d, DRFLAG: %d, HEAP: %d\r\n",
	 counter, length, FRAME_BUFFER_CURSOR, OVERFLOW_BUFFER_DATA_LENGTH,
	 FLAGS.OVERFLOW_BUFFER_DATA_READY_FLAG,
	 FLAGS.BACKGOUND_DATA_READY_FLAG, system_get_free_heap_size());*/
}

//Callback function to handle TCP disconnection
void ICACHE_FLASH_ATTR tcp_server_discon_cb(void *arg) {
	os_printf("tcp disconnect succeed !!! \r\n");
	FLAGS.OVERFLOW_BUFFER_DATA_READY_FLAG = 1;
	OVERFLOW_BUFFER_DATA_LENGTH = 0;
	FRAME_BUFFER_CURSOR = 0;
}

//Callback function for incoming TCP connection
void ICACHE_FLASH_ATTR TCPconnectCB(void *arg) {
	struct espconn *pesp_conn = arg;
	os_printf("tcp_server_listen !!! \r\n");
	espconn_regist_recvcb(pesp_conn, tcp_server_recv_cb);
	espconn_regist_reconcb(pesp_conn, tcp_server_recon_cb);
	espconn_regist_disconcb(pesp_conn, tcp_server_discon_cb);
	espconn_regist_sentcb(pesp_conn, tcp_server_sent_cb);
	espconn_regist_write_finish(pesp_conn, tcp_server_write_finish_cb);
	espconn_set_opt(pesp_conn, ESPCONN_REUSEADDR | ESPCONN_NODELAY);
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
		tcp1.local_port = TCP_LOCAL_PORT;
		conn1.type = ESPCONN_TCP;
		conn1.state = ESPCONN_NONE;
		conn1.proto.tcp = &tcp1;
		espconn_regist_connectcb(&conn1, TCPconnectCB);
		espconn_accept(&conn1);
		espconn_regist_time(&conn1, TCP_SERVER_TIMEOUT, 0);
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
	os_memcpy(routerparm.ssid, WIFI_SOFTAP_SSID, 7);
	os_memcpy(routerparm.password, WIFI_SOFTAP_PASS, 8);
	routerparm.authmode = AUTH_WPA_WPA2_PSK;
	routerparm.ssid_len = 0; // Or its actual length
	routerparm.max_connection = 4; // Max no of stations allowed
	wifi_softap_set_config(&routerparm);
}

//Wifi station configuration information structure
void ICACHE_FLASH_ATTR user_set_station_configuration(void) {
	wifi_station_set_auto_connect(wifi_station_auto_connect);
	wifi_station_dhcpc_set_maxtry(5);
	wifi_station_ap_number_set(5);
	wifi_station_set_reconnect_policy(wifi_station_reconnect_policy);
	char ssid[32] = WIFI_STATION_SSID;
	char password[64] = WIFI_STATION_PASS;
	struct station_config routerparm;
	routerparm.bssid_set = 0;
	os_memcpy(&routerparm.ssid, ssid, 32);
	os_memcpy(&routerparm.password, password, 64);
	wifi_station_set_config(&routerparm);
}

//WIFI scan done callback function
void ICACHE_FLASH_ATTR scanCB(void *arg, STATUS status) {
	struct bss_info *bssInfo;
	bssInfo = (struct bss_info *) arg;
	while (bssInfo != NULL ) {
		os_printf("ssid: %s\n", bssInfo->ssid);
		bssInfo = STAILQ_NEXT(bssInfo, next);
	}
}
