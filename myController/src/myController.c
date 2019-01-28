#include <stdlib.h>
#include <stdio.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#define MY_PULSE_CODE _PULSE_CODE_MINAVAIL

typedef union {
	struct _pulse pulse;
	char msg[255];
} my_message_t;


int main(void) {
	// Attach to device
	name_attach_t *device = name_attach(NULL, "mydevice", 0);
	if(device == NULL) {
		printf("Error: Cannot name_attach");
		return EXIT_FAILURE;
	}

	// Open resource manager
	FILE *rsmgr = fopen("/dev/local/mydevice", "r+");
	if(rsmgr == NULL){
		printf("Error: Cannot open device");
		name_detach(device, NAME_FLAG_DETACH_SAVEDPP);
		return EXIT_FAILURE;
	}

	// Read from resource manager
	char status[8];
	char value[255];
	int result;
	if((result = fscanf(rsmgr, "%s %s", status, value)) != 2){
		printf("Error: Reading file %d\n", result);
		fclose(rsmgr);
		name_detach(device, NAME_FLAG_DETACH_SAVEDPP);
		return EXIT_FAILURE;
	}
	fclose(rsmgr);

	// Check status
	if(strcmp(status, "status") == 0 && strcmp(value, "closed") == 0){
		printf("Closing controller...");
		fclose(rsmgr);
		name_detach(device, NAME_FLAG_DETACH_SAVEDPP);
		return EXIT_SUCCESS;
	}

	my_message_t rmsg;
	int rcvid;
	while(1){
		if((rcvid = MsgReceivePulse(device->chid, &rmsg, sizeof(rmsg), NULL)) < 0){
			printf("Error: Receiving pulse");
			return EXIT_FAILURE;
		}

		// Message received
		if(rcvid == 0 && rmsg.pulse.code == MY_PULSE_CODE){
			printf("Small Integer: %d\n", rmsg.pulse.value.sival_int);

			// Open resource manager
			rsmgr = fopen("/dev/local/mydevice", "r+");
			if(rsmgr == NULL){
				printf("Error: Cannot open device");
				name_detach(device, NAME_FLAG_DETACH_SAVEDPP);
				return EXIT_FAILURE;
			}

			// Read status
			fscanf(rsmgr, "%s %s", status, rmsg.msg);

			// Read status
			if(strcmp(status, "status") == 0){
				if(strcmp(rmsg.msg, "closed") == 0) break;
				else printf("Status: %s\n", rmsg.msg);
			}
			fclose(rsmgr);

		}else{
			MsgReply(rcvid, EOK, 0, 0);
		}
	}
	fclose(rsmgr);
	name_detach(device, NAME_FLAG_DETACH_SAVEDPP);
	return EXIT_SUCCESS;
}

