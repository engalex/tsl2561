#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <stdio.h>
#include <string.h>
#include "TSL2561.h"

#define MSGLEN 128

uint16_t broadband, ir;
uint32_t lux=0;
TSL2561 light1 = TSL2561_INIT(1, TSL2561_ADDR_FLOAT);

int setupSensor() {
	// initialize the sensor
	int rc = TSL2561_OPEN(&light1);
	if(rc != 0) {
		fprintf(stderr, "Error initializing TSL2561 sensor (%s). Check your i2c bus (es. i2cdetect)\n", strerror(light1.lasterr));
		// you don't need to TSL2561_CLOSE() if TSL2561_OPEN() failed, but it's safe doing it.
		TSL2561_CLOSE(&light1);
		return 1;
	}
	return 0;	
}

int getSensor() {
	// set the gain to 1X (it can be TSL2561_GAIN_1X or TSL2561_GAIN_16X)
	// use 16X gain to get more precision in dark ambients, or enable auto gain below
	int rc = TSL2561_SETGAIN(&light1, TSL2561_GAIN_1X);
	
	// set the integration time 
	// (TSL2561_INTEGRATIONTIME_402MS or TSL2561_INTEGRATIONTIME_101MS or TSL2561_INTEGRATIONTIME_13MS)
	// TSL2561_INTEGRATIONTIME_402MS is slower but more precise, TSL2561_INTEGRATIONTIME_13MS is very fast but not so precise
	rc = TSL2561_SETINTEGRATIONTIME(&light1, TSL2561_INTEGRATIONTIME_101MS);
	
	// sense the luminosity from the sensor (lux is the luminosity taken in "lux" measure units)
	// the last parameter can be 1 to enable library auto gain, or 0 to disable it
	rc = TSL2561_SENSELIGHT(&light1, &broadband, &ir, &lux, 1);
	printf("Test. RC: %i(%s), broadband: %i, ir: %i, lux: %i\n", rc, strerror(light1.lasterr), broadband, ir, lux);
}

int closeSensor() {
	TSL2561_CLOSE(&light1);
	return 0;
}

/*******************************************************************************
 * Copyright (c) 2015 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Jeffrey Dare - initial implementation and API implementation
 *******************************************************************************/

#include <stdio.h>
#include <signal.h>
#include "iotfclient.h"

volatile int interrupt = 0;

// Handle signal interrupt
void sigHandler(int signo) {
	printf("SigINT received.\n");
	interrupt = 1;
}

void myCallback (char* commandName, char* format, void* payload)
{
	printf("------------------------------------\n" );
	printf("The command received :: %s\n", commandName);
	printf("format : %s\n", format);
	printf("Payload is : %s\n", (char *)payload);

	printf("------------------------------------\n" );
}

int main(int argc, char const *argv[])
{
	int rc = -1;
	char msg[MSGLEN];

	Iotfclient client;

	//catch interrupt signal
	signal(SIGINT, sigHandler);
	signal(SIGTERM, sigHandler);

	char *configFilePath = "./device.cfg";

	rc = initialize_configfile(&client, configFilePath);

	if(rc != SUCCESS){
		printf("initialize failed and returned rc = %d.\n Quitting..", rc);
		return 0;
	}

	rc = connectiotf(&client);

	if(rc != SUCCESS){
		printf("Connection failed and returned rc = %d.\n Quitting..", rc);
		return 0;
	}

	setCommandHandler(&client, myCallback);

	rc = setupSensor();

	if(rc != SUCCESS) {
		printf("setupSensor() failed and return rc = %d.\n Quitting..", rc);
	}

	while(!interrupt) 
	{
		int i;
		for (i = 0; i < MSGLEN; i++) {
			msg[i] = 0;
		}
		getSensor();
		printf("Publishing the event stat with rc ");
		sprintf(msg, "{\"broadband\":%i,\"ir\":%i,\"lux\":%i}", broadband, ir, lux);
		rc= publishEvent(&client, "status","json", msg, QOS0);
		printf("%s\n", msg);
		printf(" %d\n", rc);
		yield(&client, 1000);
		sleep(2);
	}

	printf("Quitting!!\n");

	closeSensor();

	disconnect(&client);

	return 0;
}

