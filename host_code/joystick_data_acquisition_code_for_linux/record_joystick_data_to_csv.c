/* This code reads the three-axis accelerometer data from up to two joysticks
 * and saves each axis to a separate csv file. To read data from js0 and js1
 * uncomment the line #define JOY_DEV1 "/dev/input/js1"
 *
 * This is a heavily modified version of joytest.c found in the package 
 * JW_Linux_01.zip on the JoyWarrior website. This modified version contains
 * code taken from the QCN forums for joystick correction. The joystick
 * correction code prevents the the joystick driver from mangling the raw
 * joystick data with calibration values.
 *
 * To compile: gcc record_joystick_data_to_csv.c -o record_joystick_data_to_csv
 * To run: ./record_joystick_data_to_csv
 *         ./record_joystick_data_to_csv -n N   (where N is the desired number of samples)
 *
 * joytestv2 author: Jonathan Thomson
 * license: Unknown
 */

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>

#define JOY_DEV0 "/dev/input/js0"
//#define JOY_DEV1 "/dev/input/js1"

#define MAX_AXES 9
#define DEFAULT_NUM_SAMPLES 9000
// when non-blocking 360000 is a good number of loop iterations
//#define DEFAULT_NUM_SAMPLES 360000

typedef struct
{
	int joy_fd;
	int num_of_axis;
	int num_of_buttons;
	char *dev_path_of_joystick;
	char name_of_joystick[80];
	FILE *fpx;
	FILE *fpy;
	FILE *fpz;
} joystick_container_t;


int setup_joystick(joystick_container_t *jsp)
{
	int i = 0;
	int j = 0;
	struct js_corr corr[MAX_AXES];

	jsp->joy_fd = open(jsp->dev_path_of_joystick, O_RDONLY);
	if (jsp->joy_fd == -1)
	{
		fprintf(stderr, "Couldn't open joystick: %s.\n", jsp->dev_path_of_joystick);
		return -1;
	}

	/* Zero correction coefficient structure and set all axes to Raw mode. */
	for (i=0; i<MAX_AXES; i++)
	{
		corr[i].type = JS_CORR_NONE;
		corr[i].prec = 0;
		for (j=0; j<8; j++)
		{
			corr[i].coef[j] = 0;
		}
	}

	if (ioctl(jsp->joy_fd, JSIOCSCORR, &corr))
	{
		fprintf(stderr, "Error setting joystick correction.\n");
		return -1;
	}

	/* The original joytest.c code uses non-blocking mode, but I don't see
	 * why. It makes more sense to me to wait for the data than keep the
	 * loop busy by repeatedly calling read until new data is ready. */
	//fcntl(joy_fd, F_SETFL, O_NONBLOCK);   /* use non-blocking mode */

	ioctl(jsp->joy_fd, JSIOCGAXES, &(jsp->num_of_axis));
	ioctl(jsp->joy_fd, JSIOCGBUTTONS, &(jsp->num_of_buttons));
	ioctl(jsp->joy_fd, JSIOCGNAME(80), &(jsp->name_of_joystick));

	fprintf(stdout, "Joystick detected %s: %s\n\t%d axis\n\t%d buttons\n\n"
	              , jsp->dev_path_of_joystick
	              , jsp->name_of_joystick
	              , jsp->num_of_axis
	              , jsp->num_of_buttons);

	return 0;
}

int get_data(joystick_container_t *jsp)
{
	int num_bytes = 0;

	/* initialize with a non-existant axis number to be sure bad data 
	 * doesn't get through the if statements. */
	struct js_event jse = {0, 0, 0, 255};

	/* read the joystick state */
	num_bytes = read(jsp->joy_fd, &jse, sizeof(struct js_event));
	if (num_bytes < 1)
	{
		return -1;
	}

	switch (jse.type & ~JS_EVENT_INIT)
	{
	case JS_EVENT_AXIS:
		if (jse.number == 0)
		{
			fprintf(jsp->fpx, "%u, %d\n", \
			        jse.time, jse.value);
			fflush(jsp->fpx);
		}
		else if (jse.number == 1)
		{
			fprintf(jsp->fpy, "%u, %d\n", \
			        jse.time, jse.value);
			fflush(jsp->fpy);
		}
		else if (jse.number == 2)
		{
			fprintf(jsp->fpz, "%u, %d\n", \
			        jse.time, jse.value);
			fflush(jsp->fpz);
		}
		break;
	default:
		break;
	}

	return 0;
}

int main(int argc, char* argv[])
{
	int err_code = 0;
	int num_samples = DEFAULT_NUM_SAMPLES;
	FILE * fpx;
	FILE * fpy;
	FILE * fpz;

	fpx = fopen("js0_x-axis.csv", "w");
	fpy = fopen("js0_y-axis.csv", "w");
	fpz = fopen("js0_z-axis.csv", "w");
	joystick_container_t jsc0 = {0, 0, 0, JOY_DEV0, "", fpx, fpy, fpz};
#ifdef JOY_DEV1
	fpx = fopen("js1_x-axis.csv", "w");
	fpy = fopen("js1_y-axis.csv", "w");
	fpz = fopen("js1_z-axis.csv", "w");
	joystick_container_t jsc1 = {0, 0, 0, JOY_DEV1, "", fpx, fpy, fpz};
#endif

	if (argc > 2 && (strncmp("-n", argv[1], 2*sizeof(char)) == 0))
	{
		char *p;
		errno = 0;
		num_samples = strtol(argv[2], &p, 10);
		if (errno != 0 || *p != 0 || p == argv[2])
		{
			fprintf(stderr, "Invalid number of samples requested.\n");
			goto finished;
		}
	}

	err_code = setup_joystick(&jsc0);
#ifdef JOY_DEV1
	err_code = setup_joystick(&jsc1);
#endif

	if (err_code != 0)
	{
		fprintf(stderr, "Error setting up joystick.\n");
		goto finished;
	}


	/* The USB polling rate for the JoyWarrior is one report every 8 ms (125 Hz).
	 * A report contains all the updated data for each axis and button, but
	 * three joystick reads are required to get a sample from each axis.
	 * (8000 us)/3 = 2666.67 us 
	 * So this loop nexts to cycle at least once every 2666.67 us.
	 */
	while(num_samples--)
	{
		err_code = get_data(&jsc0);
#ifdef JOY_DEV1
		err_code = get_data(&jsc1);
#endif

		if (err_code != 0)
		{
			fprintf(stderr, "Error reading from joystick.\n");
			goto finished;
		}
	}

finished:
	fclose(jsc0.fpx);
	fclose(jsc0.fpy);
	fclose(jsc0.fpz);
	close(jsc0.joy_fd);
#ifdef JOY_DEV1
	fclose(jsc1.fpx);
	fclose(jsc1.fpy);
	fclose(jsc1.fpz);
	close(jsc1.joy_fd);
#endif

	return 0;
}
