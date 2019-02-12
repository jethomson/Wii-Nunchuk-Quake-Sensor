/*
   This program enables a teensy to act as a USB adapter for a Wii Nunchuk.
   It reports itself as an HID joystick and outputs only the accelerometer
   data from the Wii Nunchuk to the host computer. The nunchuk data is filtered
   with a moving average filter. This program is based on the LUFA Joystick demo
   by Dean Camera and uses the TWI library by Peter Fleury. Attributions and
   copyright notices are contained within the respective files.

   All original modifications are copyrighted by Jonathan Thomson.
   The license for the original Joystick.c is applied to all original
   modifications made by Jonathan Thomson. 
*/

/*
             LUFA Library
     Copyright (C) Dean Camera, 2011.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2011  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/* ------ NOTES ------
 * Genuine and fake nunchuks were both tested. The genuine nunchuks use an
 * accelerometer from STMicroelectronics and the fakes use an accelerometer with
 * the markings 6331 over QS***, where the asterisks stand in for a three letter
 * code.
 *
 * The STMicroelectronics based nunchuks work at 100 and 200 kHz and require a
 * delay between reading out data and requesting new data.

 * The 6331 based nunchuks work at 100, 200, and 400 kHz, don't require a delay
 * between reading out data and requesting new data, and can be read from more
 * frequently. However, the accelerometer data in 6331 based nunchuks have some
 * bits that are permanently fixed (i.e. stuck at 0 or 1). Bit 0 of byte 2 is
 * always zero, bit 0 of byte 3 is always one, and bit 0 of byte 4 is always
 * zero. Bits 3, 4, and 5 of byte 5 are also stuck at zero.
 * See the article at:
 * http://jethomson.wordpress.com/2012/04/29/fake-wii-nunchuks-with-a-6331-accelerometer/
 *
 * This code checks the nunchuk's identification bytes and only works with 
 * genuine nunchuks.
 *
 * To prevent aliasing the nunchuk uses an anti-aliasing filter on each of the
 * accelerometer's axes before the nunchuk's internal microcontroller samples
 * them. The anti-aliasing filters have a cut-off frequency of about 60 Hz.
 * Therefore the teensy should take at least 120 readings (samples) a second 
 * from the nunchuk, where each reading contains the data for all three axes
 * of the accelerometer. The USB polling frequency is 125 Hz, so oversampling
 * won't help with visual interpretion the data.
 */

#include "nunchuk_quake_sensor.h"
#include "i2cmaster.h"

#define next_cbi() (cbi == (M-1)) ? 0 : cbi+1

// accelerometer data buffers
static uint16_t buff_x[M];
static uint16_t buff_y[M];
static uint16_t buff_z[M];

/** Buffer to hold the previously generated HID report, for comparison purposes inside the HID class driver. */
static uint8_t PrevJoystickHIDReportBuffer[sizeof(USB_JoystickReport_Data_t)];

/** LUFA HID Class driver interface configuration and state information. This structure is
 *  passed to all HID Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_HID_Device_t Joystick_HID_Interface =
	{
		.Config =
			{
				.InterfaceNumber              = 0,

				.ReportINEndpointNumber       = JOYSTICK_EPNUM,
				.ReportINEndpointSize         = JOYSTICK_EPSIZE,
				.ReportINEndpointDoubleBank   = false,

				.PrevReportINBuffer           = PrevJoystickHIDReportBuffer,
				.PrevReportINBufferSize       = sizeof(PrevJoystickHIDReportBuffer),
			},
	};


ISR(TIMER1_COMPA_vect)
{
	uint8_t i = 0;
	static uint8_t cbi = 0;
	uint8_t nc_data[NUM_BYTES] = { 0 };

	/* read a new sample */
	i2c_rep_start(DevAddr+I2C_READ);

	while(i < (NUM_BYTES-1))
	{
		nc_data[i] = i2c_readAck();    // read one byte from nunchuk
		i++;
	}
	nc_data[i] = i2c_readNak();
	i2c_stop();



	/* increment the circular buffer index. cbi is now the index of the 
	 * oldest sample, which will become the newest sample index as soon as
	 * the oldest sample is overwritten. */
	i = cbi;
	cbi = next_cbi();

	/* Sometimes the data for one or more axes will spike or dip. If this
	 * happens, then discard those samples. A spike is always indicated by
	 * bytes 4 and 5 being equal to 0xFE. */
	if ( !(nc_data[4] == 0xFE && nc_data[5] == 0xFE) )
	{
		/* byte nc_data[5] contains the two lowest bits of accelerometer
		 * data for each axis */
		// ((x >> (p+1-n)) & ~(~0 << n)) gives the bits p:(p-(n-1)) of x.
		// This expression is more portable because it is independent of
		// word length. The C Programming Language, p.45
		buff_x[cbi] = (nc_data[2] << 2) | ((nc_data[5] >> 2) & ~(~0 << 2));
		buff_y[cbi] = (nc_data[3] << 2) | ((nc_data[5] >> 4) & ~(~0 << 2));
		buff_z[cbi] = (nc_data[4] << 2) | ((nc_data[5] >> 6) & ~(~0 << 2));
	}
	else
	{
		/* since the newest sample was bad, reuse the previous sample
		 * and save it in the buffer's slot for the newest sample */
		buff_x[cbi] = buff_x[i];
		buff_y[cbi] = buff_y[i];
		buff_z[cbi] = buff_z[i];
	}


	/* The STMicroelectronics based nunchuk needs a delay of 14 or more
	 * microseconds between reading data and requesting new data. Use a 15
	 * us delay to give a little padding. The buffer store if block
	 * also adds some more cushion (around 5 us). */
	_delay_us(15);

	/* tell nunchuk to prepare a new sample to be read at the next interrupt */
	i2c_start_wait(DevAddr+I2C_WRITE);
	i2c_write(0x00);
	i2c_stop();
}



int main(void)
{
	uint16_t i = 0;

	cli();

	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	DDRD = _BV(PD6);
	PORTD &= ~_BV(PD6); /* Turn yellow LED off */

	i2c_init();

	if (Nunchuk_Init() == 1)
	{
		Timer_Init();
		USB_Init();
		sei();

		for (;;)
		{
			i++;
			if (i == 16000)
			{
				PORTD ^= _BV(PD6); /* Toggle yellow LED */
				i = 0;
			}
			HID_Device_USBTask(&Joystick_HID_Interface);
			USB_USBTask();
		}
	}
	else
	{ // fake nunchuk or bad initialization (try removing power to fix).
	  // LED blinks twice repeatedly to indicate an error.
		for (;;)
		{
			i++;
			if (i == 65535)
			{
				PORTD |= _BV(PD6);
				_delay_ms(100);
				PORTD &= ~_BV(PD6);
				_delay_ms(100);

				PORTD |= _BV(PD6);
				_delay_ms(100);
				PORTD &= ~_BV(PD6);
				_delay_ms(100);

				_delay_ms(1000);

				i = 0;
			}
		}
	}
}

/* 
 * A genuine nunchuk outputs encrypted identification bytes when initialized
 * with the old init method, and unencrypted identifications bytes when
 * initialized with the new init method. A fake (6331 accelerometer) nunchuk
 * always outputs the unencrypted identification bytes whatever the
 * initialization method used. Therefore, to determine if the attached
 * controller is a genuine nunchuk rather than a fake or some other type of
 * Wii controller, the controller is initialized with the old method and the
 * ID bytes are checked to see if they match the encrypted nunchuk ID bytes.
 * encrypted ID bytes:   0xFE 0xFE 0x9A 0x1E 0xFE 0xFE
 * unencrypted ID bytes: 0x00 0x00 0xA4 0x20 0x00 0x00
 *
 * After the ID bytes are checked the new initialization method is used so the
 * nunchuk will output unencrypted data.
 */
uint8_t Nunchuk_Init(void)
{
	uint8_t i = 0;
	uint8_t nc_data[NUM_BYTES] = { 0 };

	/* old init method */
	i2c_start_wait(DevAddr+I2C_WRITE);
	i2c_write(0x40);
	i2c_write(0x00);
	i2c_stop();
	_delay_us(500);

	/* check nunchuk identification bytes */
	i2c_start_wait(DevAddr+I2C_WRITE);
	i2c_write(0xFA);
	i2c_stop();
	_delay_us(500);

	i2c_rep_start(DevAddr+I2C_READ);

	while(i < (NUM_BYTES-1))
	{
		nc_data[i] = i2c_readAck();
		i++;
	}
	nc_data[i] = i2c_readNak();
	i2c_stop();

	if (nc_data[2] == 0x9A && nc_data[3] == 0x1E && nc_data[4] == 0xFE && nc_data[5] == 0xFE)
	{ // genuine
		i = 1;
	}
	else
	{ // fake
		i = 0;
	}
	/* end check nunchuk identification bytes */

	_delay_us(500);

	/* new init method */
	i2c_start_wait(DevAddr+I2C_WRITE);      // set device address and write mode
	i2c_write(0xF0);                        // write address = F0
	i2c_write(0x55);                        // write value 0x55 to nunchuk
	i2c_stop();                             // set stop conditon = release bus
	_delay_us(500);

	i2c_start_wait(DevAddr+I2C_WRITE);      // set device address and write mode
	i2c_write(0xFB);                        // write address = FB
	i2c_write(0x00);                        // write value 0x00 to nunchuk
	i2c_stop();                             // set stop conditon = release bus
	_delay_us(500);

	/* tell nunchuk to prepare a new sample to be read at the next interrupt */
	i2c_start_wait(DevAddr+I2C_WRITE);     // set device address and write mode
	i2c_write(0x00);                       // write address = 00
	i2c_stop();
	_delay_us(500);

	return i;
}

void Timer_Init(void)
{
	TIMSK1 &= ~_BV(OCIE1A);  // disable timer compare interrupt
	TIFR1 = _BV(OCF1A);      // clear interrupt flag
	TCNT1 = 0;

	TCCR1B |= _BV(WGM12);    // CTC mode

#if M == 8
	/* at 16 MHz with an I2C clock of 100 kHz, 18384 is the fastest the 
         * STMicro based nunchuk can be sampled. */
	// M = 8, Ts = OCR1A/F_CPU, fc = 0.443/(Ts*M)
	// Ts = 18384/(16*10^6) --> fc = 48.19 Hz
	OCR1A = 18384; // 18384 ticks @ 16 MHz = 1149 us, 870 samples/s
#elif M == 16
	/* at 16 MHz with an I2C clock of 200 kHz, 12000 is the fastest the 
         * STMicro based nunchuk can be sampled. */
	// M = 16, Ts = OCR1A/F_CPU, fc = 0.443/(Ts*M)
	// Ts = 12000/(16*10^6) --> fc = 36.92 Hz
	OCR1A = 12000; // 12000 ticks @ 16 MHz = 750 us, 1333 samples/s
#endif

	TIMSK1 |= _BV(OCIE1A);  // enable timer compare interrupt
	TCCR1B |= _BV(CS10);    // start timer (no prescaling)
}

void EVENT_USB_Device_Connect(void)
{
}

void EVENT_USB_Device_Disconnect(void)
{
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= HID_Device_ConfigureEndpoints(&Joystick_HID_Interface);

	USB_Device_EnableSOFEvents();
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	HID_Device_ProcessControlRequest(&Joystick_HID_Interface);
}

/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void)
{
	HID_Device_MillisecondElapsed(&Joystick_HID_Interface);
}

/** HID class driver callback function for the creation of HID reports to the host.
 *
 *  \param[in]     HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in,out] ReportID    Report ID requested by the host if non-zero, otherwise callback should set to the generated report ID
 *  \param[in]     ReportType  Type of the report to create, either HID_REPORT_ITEM_In or HID_REPORT_ITEM_Feature
 *  \param[out]    ReportData  Pointer to a buffer where the created report should be stored
 *  \param[out]    ReportSize  Number of bytes written in the report (or zero if no report is to be sent
 *
 *  \return Boolean true to force the sending of the report, false to let the library determine if it needs to be sent
 */
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                         uint8_t* const ReportID,
                                         const uint8_t ReportType,
                                         void* ReportData,
                                         uint16_t* const ReportSize)
{
	USB_JoystickReport_Data_t* JoystickReport = (USB_JoystickReport_Data_t*)ReportData;

	uint8_t i;

	uint16_t sum_x = 0;
	uint16_t sum_y = 0;
	uint16_t sum_z = 0;

	cli();
	for (i=0; i<M; i++)
	{
		sum_x = sum_x + buff_x[i];
		sum_y = sum_y + buff_y[i];
		sum_z = sum_z + buff_z[i];
	}
	sei();

	JoystickReport->ax = ((sum_x+_BV((LOG2F(M)-1))) >> LOG2F(M));
	JoystickReport->ay = ((sum_y+_BV((LOG2F(M)-1))) >> LOG2F(M));
	JoystickReport->az = ((sum_z+_BV((LOG2F(M)-1))) >> LOG2F(M));

	// output fake buttons to imitate joywarrior
	JoystickReport->buttons = 0;

	*ReportSize = sizeof(USB_JoystickReport_Data_t);
	return true;
}

/** HID class driver callback function for the processing of HID reports from the host.
 *
 *  \param[in] HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in] ReportID    Report ID of the received report from the host
 *  \param[in] ReportType  The type of report that the host has sent, either HID_REPORT_ITEM_Out or HID_REPORT_ITEM_Feature
 *  \param[in] ReportData  Pointer to a buffer where the created report has been stored
 *  \param[in] ReportSize  Size in bytes of the received HID report
 */
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                          const uint8_t ReportID,
                                          const uint8_t ReportType,
                                          const void* ReportData,
                                          const uint16_t ReportSize)
{
	// Unused (but mandatory for the HID class driver) in this demo, since there are no Host->Device reports
}

