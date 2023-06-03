#include "cy_pdl.h"
#include "cybsp.h"
#include "cycfg.h"
#include "cycfg_capsense.h"
#include "cy_gpio.h"

#define CAPSENSE_INTR_PRIORITY    (3u)  // psoc4 priority = 0~3
#define CY_ASSERT_FAILED          (0u)

static void handle_error(void);
static void initialize_capsense(void);
static void capsense_isr(void);

#define MAX_S 13
uint8_t led = 0;
uint8_t touch_status[MAX_S] = { 0 };
uint8_t touch[MAX_S] = { 0 };
uint8_t send_data[MAX_S + 1] = { 0 };



int main(void) {
	cy_rslt_t result;
	cy_en_scb_uart_status_t initstatus;
	cy_stc_scb_uart_context_t CYBSP_UART_context;

	// Initialize Device and Peripherals
	result = cybsp_init();
	if (result != CY_RSLT_SUCCESS) {
		CY_ASSERT(0);
	}

	// Enable Global Interrupts
	__enable_irq();

	// StartUp LED
	for (int i = 0; i < 10; i++) {
		Cy_GPIO_Write(P1_2_PORT, P1_2_NUM, 1);
		Cy_SysLib_Delay(25);
		Cy_GPIO_Write(P1_2_PORT, P1_2_NUM, 0);
		Cy_SysLib_Delay(50);
	}

	// Initialize UART
	initstatus = Cy_SCB_UART_Init(CYBSP_UART_HW, &CYBSP_UART_config, &CYBSP_UART_context);
	if (initstatus != CY_SCB_UART_SUCCESS) {
		handle_error();
	}

	// UART
	Cy_SCB_UART_Enable(CYBSP_UART_HW);

	// CAPSENSE
	initialize_capsense();

	// COMMAND HEADER
	send_data[0] = 'U';


	// CAPSENSE First SCAN
	Cy_CapSense_ScanAllWidgets(&cy_capsense_context);

	// Loop
	for (;;) {
		//
		if (CY_CAPSENSE_NOT_BUSY == Cy_CapSense_IsBusy(&cy_capsense_context)) {

			// CAPSENSE Process all widgets
			Cy_CapSense_ProcessAllWidgets(&cy_capsense_context);

			led = 0;
			for (int i = 0; i < MAX_S; i++) {
				if (0 != Cy_CapSense_IsWidgetActive(i, &cy_capsense_context)) {
					if (touch_status[i] == 0) {
						touch_status[i] = 1;
						touch[i] = 1;
					} else {
						touch[i] = 0;
					}
				} else {
					touch[i] = 0;
					touch_status[i] = 0;
				}
				if (touch[i] == 1) {
					led = 1;
				}
				send_data[i + 1] = touch[i] + '0';
			}
			if (led == 1) {
				Cy_GPIO_Write(P1_2_PORT, P1_2_NUM, 1);
			} else {
				Cy_GPIO_Write(P1_2_PORT, P1_2_NUM, 0);
			}

			// CAPSENSE Start the next scan
			Cy_CapSense_ScanAllWidgets(&cy_capsense_context);

			// UART Send
			Cy_SCB_UART_PutArrayBlocking(CYBSP_UART_HW, &(send_data[0]), sizeof(send_data));
		}
	}

}

// SYSTEM
static void handle_error(void) {
	__disable_irq();
	while (1) {
	}
}


// CAPSENSE
static void initialize_capsense(void) {
	cy_capsense_status_t status = CY_CAPSENSE_STATUS_SUCCESS;
	const cy_stc_sysint_t capsense_interrupt_config = {
			.intrSrc = CYBSP_CSD_IRQ, .intrPriority = CAPSENSE_INTR_PRIORITY, };

	status = Cy_CapSense_Init(&cy_capsense_context);

	if (CY_CAPSENSE_STATUS_SUCCESS == status) {
		Cy_SysInt_Init(&capsense_interrupt_config, capsense_isr);
		NVIC_ClearPendingIRQ(capsense_interrupt_config.intrSrc);
		NVIC_EnableIRQ(capsense_interrupt_config.intrSrc);
		status = Cy_CapSense_Enable(&cy_capsense_context);
	}
}

static void capsense_isr(void) {
	Cy_CapSense_InterruptHandler(CYBSP_CSD_HW, &cy_capsense_context);
}
