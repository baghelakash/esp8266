#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "eagle_soc.h"
#include "c_types.h"
#include "user_interface.h"
#include "uart.h"
#include "slc_register.h"
#include "../include/user_config.h"
#include "../include/driver/i2s.h"
#include "../include/driver/i2s_reg.h"
#include "../include/driver/pin_mux_register.h"

#define Resolution         960 // Number of scan lines (should be divisible by 3)
#define user_TaskPrio        0 // Priority of loop function (0,1,2)
#define user_TaskQueueLen    1 // Message Queue depth
#define SLC_BUF_CNT (Resolution/3) //Number of buffers in the I2S circular buffer
#define SLC_BUF_LEN (12) //Length of one buffer, in 32-bit words.

//Create Framebuffer
uint32 framebuffer[SLC_BUF_CNT * SLC_BUF_LEN]; // Currently single buffered
//Declare loop function parameters
os_event_t user_TaskQueue[user_TaskQueueLen];
//Declare Software timer
static volatile os_timer_t soft_Timer;
//Declare volatile variables that hold timing information flow
volatile uint32 theta = 0, t1 = 0, t2 = 0, t3 = 0, i = 0, j = 0, k = 0;
//Create linked list for DMA
struct link_list_structure {
	uint32_t blocksize :12;
	uint32_t datalen :12;
	uint32_t unused :5;
	uint32_t sub_sof :1;
	uint32_t eof :1;
	uint32_t owner :1;
	uint32_t buf_ptr;
	uint32_t next_link_ptr;
};
static struct link_list_structure link_list_members[SLC_BUF_CNT];

//Callback function for I2S module
void i2s_isr(void) {
	ETS_INTR_LOCK(); //beta
	if (READ_PERI_REG(0x3ff00020) & BIT9) {
		os_printf_plus("1 %d\r\n", READ_PERI_REG(I2SINT_ST));
		WRITE_PERI_REG(I2SINT_CLR, 0xffffffff); //Clear I2S interrupt
		ETS_SPI_INTR_DISABLE(); //beta
	}
	ETS_INTR_UNLOCK(); //beta
}

//End of frame ISR
void i2s_slc_isr(void) {
	ETS_INTR_LOCK(); //beta
	if (READ_PERI_REG(SLC_INT_STATUS) & SLC_RX_EOF_INT_ST) {
		WRITE_PERI_REG(SLC_INT_CLR, 0xffffffff); // Clear SLC interrupt to resume DMA
		os_printf_plus("2\r\n");
		ETS_SPI_INTR_ENABLE(); //beta
	}
	ETS_INTR_UNLOCK(); //beta
}

//Main loop function
static void ICACHE_FLASH_ATTR
loop(os_event_t *events) {
	os_printf("loop() called\r\n");
	//system_os_post(user_TaskPrio, 0, 0 ); // Function reposts itself to OS to create a loop effect
	os_delay_us(10);
}

//GPIO interrupt callback function
void reset(void) {
	uint16 gpio_status = 0;
	gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS); // Get input levels of all GPIO in gpio_status variable
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status); // Clear GPIO interrupt flag
	if (gpio_status & BIT5) { // Check if interrupt was caused by GPIO5 interrupt
		os_printf_plus("reset() called\r\n");
	}
}

//Software timer callback function
void ICACHE_FLASH_ATTR soft_Timerfunc(void *arg) {
	os_printf_plus("soft_timerfunc() called\r\n");
	ETS_SLC_INTR_ENABLE(); // Enable SLC interrupts //beta
}

//User init done callback
void post_user_init_func() {
	os_printf("post_user_init_func() called\r\n");

	//SLC module setup
	SET_PERI_REG_MASK(SLC_CONF0, SLC_RXLINK_RST|SLC_TXLINK_RST);// Reset DMA
	CLEAR_PERI_REG_MASK(SLC_CONF0, SLC_RXLINK_RST|SLC_TXLINK_RST);
	CLEAR_PERI_REG_MASK(SLC_CONF0, (SLC_MODE<<SLC_MODE_S)); // Enable and configure DMA
	SET_PERI_REG_MASK(SLC_CONF0, (1<<SLC_MODE_S));
	SET_PERI_REG_MASK(SLC_RX_DSCR_CONF,
			SLC_INFOR_NO_REPLACE|SLC_TOKEN_NO_REPLACE); //|0xfe
	CLEAR_PERI_REG_MASK(SLC_RX_DSCR_CONF,
			SLC_RX_FILL_EN|SLC_RX_EOF_MODE | SLC_RX_FILL_MODE);
	CLEAR_PERI_REG_MASK(SLC_TX_LINK, SLC_TXLINK_DESCADDR_MASK); // Clear link descripter address
	CLEAR_PERI_REG_MASK(SLC_RX_LINK, SLC_RXLINK_DESCADDR_MASK);
	SET_PERI_REG_MASK(SLC_TX_LINK, (uint32 )&link_list_members[1]); // Set link descriptor address
	SET_PERI_REG_MASK(SLC_RX_LINK, (uint32 )&link_list_members[0]);
	ETS_SLC_INTR_ATTACH(i2s_slc_isr, NULL); // Attach i2s_slc_isr() as DMA interrupt
	WRITE_PERI_REG(SLC_INT_ENA, SLC_RX_EOF_INT_ENA|SLC_RX_DONE_INT_ENA); // Enable EOF and transmission done interrupt
	WRITE_PERI_REG(SLC_INT_CLR, 0xffffffff); // Clear all initial SLC interrupts
	//ETS_SLC_INTR_ENABLE(); // Enable SLC interrupts
	SET_PERI_REG_MASK(SLC_RX_LINK, SLC_RXLINK_START);// Start SLC module

	//I2S module setup
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_I2SO_DATA);// CONFIG I2S TX PIN FUNC
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_I2SO_BCK);
	i2c_writeReg_Mask_def(i2c_bbpll, i2c_bbpll_en_audio_clock_out, 1); // Provide running clock to I2S module
	CLEAR_PERI_REG_MASK(I2SCONF, I2S_I2S_RESET_MASK); // Reset I2S module
	SET_PERI_REG_MASK(I2SCONF, I2S_I2S_RESET_MASK);
	CLEAR_PERI_REG_MASK(I2SCONF, I2S_I2S_RESET_MASK);
	SET_PERI_REG_MASK(I2S_FIFO_CONF, I2S_I2S_DSCR_EN); // Set mode to DMA access instead of software access
	SET_PERI_REG_MASK(I2S_FIFO_CONF, 0x00<<I2S_I2S_TX_FIFO_MOD_S); // Set channel mode to 16 bits per channel full data
	SET_PERI_REG_MASK(I2SCONF_CHAN, I2S_TX_CHAN_MOD<<I2S_TX_CHAN_MOD_S); // Set dual channel mode
	WRITE_PERI_REG(I2SRXEOF_NUM, (SLC_BUF_LEN * 4)); // Set the number of bytes to receive to start DMA operation
	CLEAR_PERI_REG_MASK(I2SCONF,
			I2S_TRANS_SLAVE_MOD|(I2S_BITS_MOD<<I2S_BITS_MOD_S)|(I2S_BCK_DIV_NUM <<I2S_BCK_DIV_NUM_S)|(I2S_CLKM_DIV_NUM<<I2S_CLKM_DIV_NUM_S)); // Clear I2S config bits
	SET_PERI_REG_MASK(I2SCONF,
			I2S_RIGHT_FIRST|I2S_MSB_RIGHT|I2S_TRANS_MSB_SHIFT|(( 26&I2S_BCK_DIV_NUM )<<I2S_BCK_DIV_NUM_S)|((4&I2S_CLKM_DIV_NUM)<<I2S_CLKM_DIV_NUM_S)|(8<<I2S_BITS_MOD_S)); // Set relevant bits in config register of I2S
	SET_PERI_REG_MASK(I2SINT_CLR,
			I2S_I2S_TX_REMPTY_INT_CLR|I2S_I2S_TX_WFULL_INT_CLR|I2S_I2S_RX_REMPTY_INT_CLR|I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR); // Clear all I2S interrupts
	CLEAR_PERI_REG_MASK(I2SINT_CLR,
			I2S_I2S_TX_REMPTY_INT_CLR|I2S_I2S_TX_WFULL_INT_CLR|I2S_I2S_RX_REMPTY_INT_CLR|I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR); // Clear all I2S interrupt enable flags
	//SET_PERI_REG_MASK(I2SINT_ENA,
	//	I2S_I2S_TX_REMPTY_INT_ENA|I2S_I2S_TX_WFULL_INT_ENA|I2S_I2S_TX_PUT_DATA_INT_ENA); // Set all interrupts for I2S //beta
	SET_PERI_REG_MASK(I2SINT_ENA, I2S_I2S_TX_REMPTY_INT_ENA);//beta
	ETS_SPI_INTR_ATTACH(i2s_isr, NULL);
	ETS_SPI_INTR_ENABLE();
	SET_PERI_REG_MASK(I2SCONF, I2S_I2S_TX_START);	// Start I2S transmission

	//Software recurring timer stuff
	os_timer_disarm(&soft_Timer); // Disarm timer pre-configuration
	os_timer_setfn(&soft_Timer, (os_timer_func_t *) soft_Timerfunc, NULL); // Set software timer function
	os_timer_arm(&soft_Timer, 1000, 1); // Arm timer post-configuration

	//Post a message in OS to invoke user task of set priority
	system_os_post(user_TaskPrio, 0, 0);
}

//Init function
void ICACHE_FLASH_ATTR user_init() {
	//UART stuff
	//system_uart_swap();
	uart_init(115200, 115200); // Initialize UART0 to use as debug
	//UART_SetPrintPort(1); // Shift printing to GPIO2
	os_printf("user_init() called\r\n");

	int x = 0;
	//Initialize Framebuffer
	for (x = 0; x < (SLC_BUF_CNT * SLC_BUF_LEN); x++) {
		//framebuffer[x] = 0x0000 | (*(volatile uint32_t *)0x3FF20E44) >>16; //Hardware random number generator
		framebuffer[x] = 0x12345678;
	}
	//Initialize link list
	for (x = 0; x < SLC_BUF_CNT; x++) {
		link_list_members[x].owner = 1;
		link_list_members[x].eof = 1;
		link_list_members[x].sub_sof = 0;
		link_list_members[x].unused = 0;
		link_list_members[x].datalen = SLC_BUF_LEN * 4;
		link_list_members[x].blocksize = SLC_BUF_LEN * 4;
		link_list_members[x].buf_ptr = (uint32_t) &framebuffer[x * SLC_BUF_LEN];
		link_list_members[x].next_link_ptr = (uint32_t) (
				(x < (SLC_BUF_CNT - 1)) ?
						(&link_list_members[x + 1]) : (&link_list_members[0]));
	}

	//register a callback function to let user code know that system initialization is complete
	system_init_done_cb(&post_user_init_func);

	//GPIO stuff
	gpio_init();
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4); // Set GPIO4 as GPIO
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4); // Set GPIO5 as GPIO
	gpio_output_set(0, BIT4, BIT4, 0); // Set GPIO4 as output and Low level
	gpio_output_set(0, 0, 0, BIT5);  // Set GPIO5 as input
	ETS_GPIO_INTR_DISABLE(); // Disable all IO interrupts before setting interrupt callback function
	ETS_GPIO_INTR_ATTACH(reset, NULL); // Set reset() as interrupt callback function
	gpio_pin_intr_state_set(GPIO_ID_PIN(5), GPIO_PIN_INTR_POSEDGE); // Attach rising edge interrupt to GPIO5
	ETS_GPIO_INTR_ENABLE(); // Enable all IO interrupts post-configuration

	//Register a system OS task of set priority
	system_os_task(loop, user_TaskPrio, user_TaskQueue, user_TaskQueueLen);
}
