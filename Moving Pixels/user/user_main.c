#include "../include/user_config.h"

//Declare loop function parameters
os_event_t user_TaskQueue[user_TaskQueueLen];
//Declare Software timer
static volatile os_timer_t soft_Timer;
//Declare variables
uint32_t volatile screenMatrix[2][Resolution * 4] = { 0 }; // Framebuffer, double buffered
uint32_t volatile theta = 0, i = 0, j = 0, k = 0, FRAME_BUFFER_CURSOR = 0, t1 =
		0, t2 = 0, t3 = 0, OVERFLOW_BUFFER_DATA_LENGTH = 0, FLASH_CURSOR =
		0x100000; // Variables needed for timing and indexing
uint8 volatile CURRENT_FRAMEBUFFER = 0, OBJECT1 = 0, OBJECT2 = 0, OBJECT3 = 0,
		OVERFLOW_BUFFER[1460 * 5];
extern struct espconn *TCP_CONNECTION_ID;
extern uint32_t volatile CART2POL[357][357];
FLAG FLAGS = { 0, 0, 1, 0 };
uint8 car_obj = 0;

//Hardware timer callback function
void tick(void) {
	ETS_INTR_LOCK();
	RTC_REG_WRITE(FRC1_INT_ADDRESS, FRC1_INT_CLR_MASK); // Clear hardware timer interrupt
//os_printf("tick() called\r\n");
	theta += 1; // Increment theta
	if ((FLAGS.HUD_DATA_READY_FLAG) && ((theta % (Resolution / 3)) == 0)) { // Check if new frame is ready and it is a new frame entry point
		CURRENT_FRAMEBUFFER = !CURRENT_FRAMEBUFFER;
		//FLAGS.BACKGOUND_DATA_READY_FLAG = 0; //beta
		FLAGS.HUD_DATA_READY_FLAG = 0; //beta
		espconn_recv_unhold(TCP_CONNECTION_ID); //beta
	}
	if (theta < Resolution) {
		i = ((theta + Resolution * 2 / 3) % Resolution) * 4; // Cursors to read frame buffer
		j = ((theta + Resolution / 3) % Resolution) * 4;
		k = (theta % Resolution) * 4;
		WRITE_PERI_REG(SPI_W0(1), screenMatrix[CURRENT_FRAMEBUFFER][j + 3]); // Push data to screen via SPI
		WRITE_PERI_REG(SPI_W1(1), screenMatrix[CURRENT_FRAMEBUFFER][j + 2]);
		WRITE_PERI_REG(SPI_W2(1), screenMatrix[CURRENT_FRAMEBUFFER][j + 1]);
		WRITE_PERI_REG(SPI_W3(1), screenMatrix[CURRENT_FRAMEBUFFER][j]);
		WRITE_PERI_REG(SPI_W4(1), screenMatrix[CURRENT_FRAMEBUFFER][i + 3]);
		WRITE_PERI_REG(SPI_W5(1), screenMatrix[CURRENT_FRAMEBUFFER][i + 2]);
		WRITE_PERI_REG(SPI_W6(1), screenMatrix[CURRENT_FRAMEBUFFER][i + 1]);
		WRITE_PERI_REG(SPI_W7(1), screenMatrix[CURRENT_FRAMEBUFFER][i]);
		WRITE_PERI_REG(SPI_W8(1), screenMatrix[CURRENT_FRAMEBUFFER][k + 3]);
		WRITE_PERI_REG(SPI_W9(1), screenMatrix[CURRENT_FRAMEBUFFER][k + 2]);
		WRITE_PERI_REG(SPI_W10(1), screenMatrix[CURRENT_FRAMEBUFFER][k + 1]);
		WRITE_PERI_REG(SPI_W11(1), screenMatrix[CURRENT_FRAMEBUFFER][k]);
		SET_PERI_REG_MASK(SPI_CMD(1), SPI_USR); // Start SPI transfer
	}
	ETS_INTR_UNLOCK();
}

//SPI transfer done callback function
void push(void) {
	ETS_INTR_LOCK();
	if (READ_PERI_REG(SPI_SLAVE(1)) & SPI_TRANS_DONE) {
		CLEAR_PERI_REG_MASK(SPI_SLAVE(1), SPI_TRANS_DONE); // Clear SPI DONE interrupt
		//os_printf("push() called\r\n");
		gpio_output_set(BIT4, 0, BIT4, 0);
		gpio_output_set(0, BIT4, BIT4, 0);
		FLAGS.READ_VIDEO_FROM_FLASH_FLAG = 1; //beta
	}
	ETS_INTR_UNLOCK();
}

//GPIO interrupt callback function
void reset(void) {
	ETS_INTR_LOCK();
	uint16 gpio_status = 0;
	gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS); // Get input levels of all GPIO in gpio_status variable
	if (gpio_status & BIT5) { // Check if interrupt was caused by GPIO5 interrupt
		t1 = system_get_time(); // Record proximity event time
		if ((t1 - t3) > 20000ul) { // Check if its the first proximity event
			uint32 dt = t1 - t2; // Previous revolution time
			RTC_SET_REG_MASK(FRC1_CTRL_ADDRESS, 0b010000000); // Set bits to enable timer
			RTC_REG_WRITE(FRC1_LOAD_ADDRESS, 80ul * dt/Resolution); // Reload timer
			theta = ORIGIN; // Reset value of theta to origin
			if (FLAGS.HUD_DATA_READY_FLAG) {
				CURRENT_FRAMEBUFFER = !CURRENT_FRAMEBUFFER;
				//FLAGS.BACKGOUND_DATA_READY_FLAG = 0;
				FLAGS.HUD_DATA_READY_FLAG = 0; //beta
				espconn_recv_unhold(TCP_CONNECTION_ID); //beta
			}
			WRITE_PERI_REG(SPI_W0(1),
					screenMatrix[CURRENT_FRAMEBUFFER][(theta + Resolution / 3) * 4 + 3]); // Push data to screen via SPI
			WRITE_PERI_REG(SPI_W1(1),
					screenMatrix[CURRENT_FRAMEBUFFER][(theta + Resolution / 3) * 4 + 2]);
			WRITE_PERI_REG(SPI_W2(1),
					screenMatrix[CURRENT_FRAMEBUFFER][(theta + Resolution / 3) * 4 + 1]);
			WRITE_PERI_REG(SPI_W3(1),
					screenMatrix[CURRENT_FRAMEBUFFER][(theta + Resolution / 3) * 4]);
			WRITE_PERI_REG(SPI_W4(1),
					screenMatrix[CURRENT_FRAMEBUFFER][(theta + Resolution*2 / 3) * 4 + 3]);
			WRITE_PERI_REG(SPI_W5(1),
					screenMatrix[CURRENT_FRAMEBUFFER][(theta + Resolution *2/ 3) * 4 + 2]);
			WRITE_PERI_REG(SPI_W6(1),
					screenMatrix[CURRENT_FRAMEBUFFER][(theta + Resolution *2/ 3) * 4 + 1]);
			WRITE_PERI_REG(SPI_W7(1),
					screenMatrix[CURRENT_FRAMEBUFFER][(theta + Resolution *2/ 3) * 4]);
			WRITE_PERI_REG(SPI_W8(1),
					screenMatrix[CURRENT_FRAMEBUFFER][(theta) * 4 + 3]);
			WRITE_PERI_REG(SPI_W9(1),
					screenMatrix[CURRENT_FRAMEBUFFER][(theta) * 4 + 2]);
			WRITE_PERI_REG(SPI_W10(1),
					screenMatrix[CURRENT_FRAMEBUFFER][(theta) * 4 + 1]);
			WRITE_PERI_REG(SPI_W11(1),
					screenMatrix[CURRENT_FRAMEBUFFER][(theta) * 4]);
			SET_PERI_REG_MASK(SPI_CMD(1), SPI_USR); // Start SPI transfer

			t2 = t1; // Store first proximity event time
			RTC_REG_WRITE(FRC1_INT_ADDRESS, FRC1_INT_CLR_MASK); // Clear hardware timer interrupt
			CLEAR_PERI_REG_MASK(SPI_SLAVE(1), SPI_TRANS_DONE); // Clear SPI DONE interrupt
		}
		t3 = t1;
		GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status); // Clear GPIO interrupt flag
	}
	ETS_INTR_UNLOCK();
}

//Software timer callback function
void ICACHE_FLASH_ATTR soft_Timerfunc(void *arg) {
//os_printf("soft_timerfunc() called\r\n");
}

//Main loop function
void loop(os_event_t *events) {
//ETS_INTR_LOCK();
//os_printf("loop() called\r\n");
	if (FLAGS.BACKGOUND_DATA_READY_FLAG) {
		rasterize(); //beta
		FLAGS.BACKGOUND_DATA_READY_FLAG = 0; //beta
	}
	if (FLAGS.READ_VIDEO_FROM_FLASH_FLAG && (!(FLAGS.BACKGOUND_DATA_READY_FLAG))
			&& MODE == VIDEO_FROM_FLASH) {
		//Read data from flash and store in framebuffer
		spi_flash_read(FLASH_CURSOR,
				(uint32 *) (&screenMatrix[!CURRENT_FRAMEBUFFER][0]
						+ FRAME_BUFFER_CURSOR / 4), 0x400); // Pull 1KB data from flash into screenmatrix
		FRAME_BUFFER_CURSOR += 0x400; // Increment FRAME_BUFFER_CURSOR
		FLASH_CURSOR += 0x400; // Increment Flash cursor
		FLAGS.READ_VIDEO_FROM_FLASH_FLAG = 0; // Indicate completion of 1kb data transfer
		if ((FLASH_CURSOR - FLASH_VIDEO_ADDRESS) % 0x3C00 == 0) { // If 15 KB has been transferred, set data ready flag
			FLAGS.BACKGOUND_DATA_READY_FLAG = 1;
			FRAME_BUFFER_CURSOR = 0;
			if (FLASH_CURSOR == FLASH_CURSOR_ROLLBACK) // Reset flash cursor to flash start
				FLASH_CURSOR = 0x100000;
		}
	}
	system_os_post(user_TaskPrio, 0, 0); // Function reposts itself to OS to create a loop effect
//ETS_INTR_UNLOCK();
}

//User init done callback
void ICACHE_FLASH_ATTR post_user_init_func() {

//Wifi stuff
//wifi_station_connect();
//wifi_station_scan(NULL, scanCB);

//Software recurring timer stuff
	os_timer_disarm(&soft_Timer); // Disarm timer pre-configuration
	os_timer_setfn(&soft_Timer, (os_timer_func_t *) soft_Timerfunc, NULL); // Set software timer function
	os_timer_arm(&soft_Timer, SOFT_TIMER_INTERVAL, 1); // arm timer post-configuration

//Post a message in OS to invoke user task of set priority
	system_os_task(loop, user_TaskPrio, user_TaskQueue, user_TaskQueueLen);
	system_os_post(user_TaskPrio, 0, 0);

//GPIO stuff
	gpio_init();
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4); // Set GPIO4 as GPIO
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5); // Set GPIO5 as GPIO
	gpio_output_set(0, BIT4, BIT4, 0); // Set GPIO4 as output and Low level
	gpio_output_set(0, 0, 0, BIT5);  // Set GPIO5 as input
	ETS_GPIO_INTR_DISABLE(); // Disable all IO interrupts before setting interrupt callback function
	ETS_GPIO_INTR_ATTACH(reset, NULL); // Set reset() as interrupt callback function
	gpio_pin_intr_state_set(GPIO_ID_PIN(5), GPIO_PIN_INTR_POSEDGE); // Attach rising edge interrupt to GPIO5
	ETS_GPIO_INTR_ENABLE(); // Enable all IO interrupts post-configuration
}

//Init function
void ICACHE_FLASH_ATTR user_init() {

	system_update_cpu_freq(CPU_FREQUENCY); // Set CPU frequency to 160Mhz

//UART stuff
//system_uart_swap();
	uart_init(UART1_BAUD_RATE, UART2_BAUD_RATE); // Initialize UART0 to use as debug
//UART_SetPrintPort(1); // Shift printing to GPIO2

//SPI stuff
	spi_init(); // Setup SPI module
	ETS_SPI_INTR_ATTACH(push, NULL); // Attach SPIinterrupt function push()
	ETS_SPI_INTR_ENABLE(); // Enable SPI interrupts

//Hardware timer stuff
	RTC_REG_WRITE(FRC1_LOAD_ADDRESS, 16000); // Configure hardware timer and load initial value of 16000 (200us)
	RTC_CLR_REG_MASK(FRC1_CTRL_ADDRESS, 0b110001101); // Clear unwanted bits
	RTC_SET_REG_MASK(FRC1_CTRL_ADDRESS, 0b001000000); // Set bits to enable time and autoload
	RTC_REG_WRITE(FRC1_INT_ADDRESS, FRC1_INT_CLR_MASK); // Clear hardware timer interrupt
	TM1_EDGE_INT_ENABLE();
	ETS_FRC_TIMER1_INTR_ATTACH(tick, NULL);
	ETS_FRC1_INTR_ENABLE();

//Wifi stuff
	wifi_set_opmode(STATIONAP_MODE); // Set access point + station mode
	wifi_set_phy_mode(WIFI_PHY_MODE);
	wifi_set_user_sup_rate(WIFI_MIN_RATE, WIFI_MAX_RATE);
	wifi_set_event_handler_cb(wifi_handle_event_cb);
	user_set_AP_configuration(); // Configure SoftAP parameters
	user_set_station_configuration(); // configure target network parameters and connect

//Print RAM info
	system_print_meminfo();

//Register a callback function to let user code know that system initialization is complete
	system_init_done_cb(&post_user_init_func);

//os_printf("user_init() done\r\n");
}
