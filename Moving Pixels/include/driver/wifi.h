#ifndef INCLUDE_DRIVER_WIFI_H_
#define INCLUDE_DRIVER_WIFI_H_

extern void ICACHE_FLASH_ATTR wifi_handle_event_cb(System_Event_t *evt);
extern void ICACHE_FLASH_ATTR scanCB(void *arg, STATUS status);
extern void ICACHE_FLASH_ATTR user_set_AP_configuration(void);
extern void ICACHE_FLASH_ATTR user_set_station_configuration(void);

#endif /* INCLUDE_DRIVER_WIFI_H_ */
