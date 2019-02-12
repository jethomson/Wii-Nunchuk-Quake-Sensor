/* This is a modified version of joytest.c found in the package JW_Linux_01.zip
 * on the JoyWarrior website. This modified version contains code taken from the
 * QCN forums for joystick correction. The joystick correction code prevents the
 * the joystick driver from mangling the raw joystick data with calibration
 * values. I've also modified the code to take an input arguement for the number
 * of samples to print and added three different print functions to improve the
 * the data is output.
 *
 * To compile: gcc joytestv2.c -o joytestv2
 * To run: ./joytestv2
 *         ./joytestv2 -n N   (where N is the desired number of samples)
 *
 * joytestv2 author: Jonathan Thomson
 * license: Unknown
 */

/* This is the linux 2.2.x way of handling joysticks. It allows an arbitrary
 * number of axis and buttons. It's event driven, and has full signed int
 * ranges of the axis (-32768 to 32767). It also lets you pull the joysticks
 * name. The only place this works of that I know of is in the linux 1.x
 * joystick driver, which is included in the linux 2.2.x kernels
 */

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>

#define JOY_DEV "/dev/input/js0"
#define MAX_AXES 9
#define DEFAULT_NUM_SAMPLES 7500

#define print_data() accelerometer_print()
//#define print_data() nunchuk_print()
//#define print_data() generic_joystick_print()

int *axis = NULL;
char *button = NULL;
int num_of_axis = 0;
int num_of_buttons = 0;

void accelerometer_print(void)
{
	//printf("AX: %4d AY: %4d AZ: %4d\r\n", axis[0], axis[1], axis[2]);
	printf("%4d\t%4d\t%4d\r\n", axis[0], axis[1], axis[2]);
}

/* This function only works correctly if the the axis data format is:
 * axis[0] = stick-x, axis[1] = stick-y, axis[2] = accelerometer-x,
 * axis[3] = accelerometer-y, axis[4] = accelerometer-z,
 * This function doesn't check the number of axes so it will print junk data
 * if it is called when it shouldn't be */
void nunchuk_print(void)
{
	int i = 0;

	printf("SX: %4d SY: %4d AX: %4d AY: %4d AZ: %4d ", \
	       axis[0], axis[1], axis[2], axis[3], axis[4]);

	for(i = 0 ; i < num_of_buttons ; i++)
	{
		printf("B%d: %d ", i, button[i]);
	}
	printf("\r\n");
}

void generic_joystick_print(void)
{
	int i = 0;

	for(i = 0 ; i < num_of_axis ; i++)
	{
		printf("A%d: %4d ", i, axis[i]);
	}

	for(i = 0 ; i < num_of_buttons ; i++)
	{
		printf("B%d: %d ", i, button[i]);
	}
	printf("\r\n");
}

int main(int argc, char* argv[])
{
	int err_code = 0;
	int num_samples = DEFAULT_NUM_SAMPLES;
	int joy_fd = -1;
	char name_of_joystick[80];
	int i = 0;
	int j = 0;
	int num_bytes = 0;
	struct js_event js = {0, 0, 0, 255};
	struct js_corr corr[MAX_AXES];


	/* debug */
	//printf("num_args: %d, argv[0]: %s, argv[1]: %s, argv[2]: %s\n", \
	//       argc, argv[0], argv[1], argv[2]);

	if (argc > 1 && (strncmp("-h", argv[1], 2*sizeof(char)) == 0))
	{
		return -1;
	}

	if (argc > 2 && (strncmp("-n", argv[1], 2*sizeof(char)) == 0))
	{
		char *p;
		errno = 0;
		num_samples = strtol(argv[2], &p, 10);
		if (errno != 0 || *p != 0 || p == argv[2])
		{
			printf("Invalid number of samples requested.\n");
			return -1;
		}
	}


	if( (joy_fd = open(JOY_DEV, O_RDONLY)) == -1 )
	{
		printf("Couldn't open joystick: %s\n", JOY_DEV);
		return -1;
	}

	/* Zero correction coefficient structure and set all axes to Raw mode. */
	for (i = 0; i < MAX_AXES; i++)
	{
		corr[i].type = JS_CORR_NONE;
		corr[i].prec = 0;
		for (j = 0; j < 8; j++)
		{
			corr[i].coef[j] = 0;
		}
	}

	if (ioctl(joy_fd, JSIOCSCORR, &corr))
	{
		printf("Error setting joystick correction.\n");
		err_code = -1;
		goto finished;
	}

	/* The original joytest.c code uses non-blocking mode, but I don't see
	 * why. It makes more sense to me to wait for the data than keep the
	 * loop busy by repeatedly calling read until new data is ready. */
	//fcntl( joy_fd, F_SETFL, O_NONBLOCK );   /* use non-blocking mode */

	ioctl(joy_fd, JSIOCGAXES, &num_of_axis);
	ioctl(joy_fd, JSIOCGBUTTONS, &num_of_buttons);
	ioctl(joy_fd, JSIOCGNAME(80), &name_of_joystick);

	axis = (int *) calloc(num_of_axis, sizeof(int));
	button = (char *) calloc(num_of_buttons, sizeof(char));

	printf("Joystick detected: %s\n\t%d axis\n\t%d buttons\n\n"
	        , name_of_joystick
	        , num_of_axis
	        , num_of_buttons);

	while (num_samples--)
	{
		js.time = 0;
		js.value = 0;
		js.type = 0;
		js.number = 0;

		/* read the joystick state */
		num_bytes = read(joy_fd, &js, sizeof(struct js_event));
		if (num_bytes < 1)
		{
			err_code = -1;
			goto finished;
		}

		/* see what to do with the event */
		switch (js.type & ~JS_EVENT_INIT)
		{
		case JS_EVENT_AXIS:
			axis[js.number] = js.value;
			break;
		case JS_EVENT_BUTTON:
			button[js.number] = js.value;
			break;
		}

		print_data();

		fflush(stdout);
	}

finished:
	close(joy_fd);
	return err_code;
}
