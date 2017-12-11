#ifndef INCLUDE_USER_CONFIG_H_
#define INCLUDE_USER_CONFIG_H_

#include "ets_sys.h"
#include "osapi.h"
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "eagle_soc.h"
#include "c_types.h"
#include "uart.h"
#include "spi_register.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "../include/driver/pin_mux_register.h"
#include "../include/driver/spi.h"
#include "../include/driver/wifi.h"

#define CPU_FREQUENCY					160 // 80 or 160Mhz

#define	Resolution						960	// Number of scan lines (should be divisible by 3)
#define	ORIGIN							0 // Starting angle (0-resolution) w.r.t proximity origin (integer)
#define	FLASH_CURSOR_ROLLBACK 			0x200000 + 0x2F9400 // Length of binary file containing image or video //beta
#define FLASH_VIDEO_ADDRESS				0x200000

#define	user_TaskPrio					0 // Priority of loop function (0,1,2)
#define	user_TaskQueueLen				1 // Message Queue depth
#define SOFT_TIMER_INTERVAL				1000 // Software time click time

#define UART1_BAUD_RATE					115200
#define UART2_BAUD_RATE					115200

#define	WIFI_SOFTAP_SSID				"ESP8266"
#define	WIFI_SOFTAP_PASS				"12345678"
#define	WIFI_STATION_SSID				"OnePlus3t"
#define	WIFI_STATION_PASS				"changeded"
#define WIFI_PHY_MODE					PHY_MODE_11G
#define	wifi_station_auto_connect		1 // Auto connect to previous access point on startup
#define	wifi_station_reconnect_policy	1 // Auto connect to previous access point on disconnection
#define WIFI_MIN_RATE					RATE_11G9M
#define WIFI_MAX_RATE					RATE_11G54M

#define TCP_LOCAL_PORT					25867
#define	TCP_SERVER_TIMEOUT				10

#define MAX_NUMBER_OF_DYNAMIC_POLAR_OBJECTS	20
#define MAX_NUMBER_OF_CHARACTER_OBJECTS	20
#define MAX_NUMBER_OF_DYNAMIC_CARTESIAN_OBJECTS	20
#define MAX_NUMBER_OF_FIXED_CARTESIAN_OBJECTS	20

enum {
	POWER_OFF = 48,
	AUTOPILOT = 49,
	VIDEO_FROM_FLASH = 50,
	VIDEO_UPLOAD_FLASH = 51,
	VIDEO_STREAM_WIFI = 52,
	IMAGE_RAM_FEED = 53,
	SCRATCHPAD = 54,
	PARTICLE_PHYSICS = 55
} MODE;

typedef struct {
	uint32_t volatile BACKGOUND_DATA_READY_FLAG :1;
	uint32_t volatile HUD_DATA_READY_FLAG :1;
	uint32_t volatile OVERFLOW_BUFFER_DATA_READY_FLAG :1;
	uint32_t volatile READ_VIDEO_FROM_FLASH_FLAG :1;
} FLAG;

typedef struct {
	uint32_t HEIGHT :21;
	uint32_t WIDTH :10;
	uint32_t R :21;
	uint32_t THETA :10;
	uint32_t EXIST_FLAG :1;
} DYNAMIC_POLAR_OBJECT; // Can be placed anywhere

typedef struct {
	uint32_t volatile *ADDRESS;
	uint32_t BAND :2;
	uint32_t THETA :10;
	uint32_t EXIST_FLAG :1;
} CHARACTER; // Fixed size with height 16 and band restricted

typedef struct {
	uint32_t HEIGHT :9;
	uint32_t WIDTH :9;
	uint32_t X :16;
	uint32_t Y :16;
	uint32_t EXIST_FLAG :1;
} DYNAMIC_CARTESIAN_OBJECT; // Can be placed anywhere in 357x357 grid

typedef struct {
	uint32_t volatile *ADDRESS;
	uint32_t HEIGHT :6;
	uint32_t WIDTH :6;
	uint32_t X :9;
	uint32_t Y :9;
	uint32_t EXIST_FLAG :1;
} FIXED_CARTESIAN_OBJECT; // Can be placed anywhere in 357x357 grid, Max size 64x64

#endif /* INCLUDE_USER_CONFIG_H_ */
