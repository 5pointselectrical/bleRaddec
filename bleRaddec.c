/*****************************************************************************
 * @file  demo_bleScanner.c
 * @brief Start the BLE discovery and subscribe the BLE event
 *******************************************************************************
 Copyright 2020 GL-iNet. https://www.gl-inet.com/

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 ******************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <json-c/json.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#include "gl_errno.h"
#include "gl_type.h"
#include "gl_bleapi.h"

static void sigal_hander(int sig);
static int ble_gap_cb(gl_ble_gap_event_t event, gl_ble_gap_data_t *data);
static int ble_module_cb(gl_ble_module_event_t event, gl_ble_module_data_t *data);
static int addr2str(BLE_MAC adr, char *str);
void removecolon(char *str);
static bool module_work = false;
static int gl_tools_hexStr2bytes(const char *hexstr, int strlen, unsigned char *bytes);


int main(int argc, char *argv[])
{
	signal(SIGTERM, sigal_hander);
	signal(SIGINT, sigal_hander);
	signal(SIGQUIT, sigal_hander);

	// init msg callback
	gl_ble_cbs ble_cb;
	memset(&ble_cb, 0, sizeof(gl_ble_cbs));

	ble_cb.ble_gap_event = ble_gap_cb;
	ble_cb.ble_gatt_event = NULL;
	ble_cb.ble_module_event = ble_module_cb;

	int phys = 1, interval = 16, window = 16, type = 0, mode = 1;
	// get scanner param
	if ((argc != 1) && (argc != 6))
	{
		printf("param err!");
		return GL_ERR_PARAM;
	}
	if (argc == 6)
	{
		phys = atoi(argv[1]);
		interval = atoi(argv[2]);
		window = atoi(argv[3]);
		type = atoi(argv[4]);
		mode = atoi(argv[5]);
	}

	// init ble module
	GL_RET ret;
	ret = gl_ble_init();
	if (GL_SUCCESS != ret)
	{
		printf("gl_ble_init failed: %d\n", ret);
		exit(-1);
	}

	ret = gl_ble_subscribe(&ble_cb);
	if (GL_SUCCESS != ret)
	{
		printf("gl_ble_subscribe failed: %d\n", ret);
		exit(-1);
	}

	// wait for module reset
	while (!module_work)
	{
		usleep(100000);
	}

	// start scan
	ret = gl_ble_discovery(phys, interval, window, type, mode);
	if (ret != GL_SUCCESS)
	{
		printf("Start ble discovery error!! Err code: %d\n", ret);
		exit(-1);
	}

	while (1)
	{
		sleep(1000);
	}

	return 0;
}

static int addr2str(BLE_MAC adr, char *str)
{
	sprintf(str, "%02x%02x%02x%02x%02x%02x", adr[5], adr[4],
			adr[3], adr[2], adr[1], adr[0]);
	return 0;
}
void removecolon(char *str)
{
	int i, j, len;
	
	len = strlen(str);
	
	for(i = 0; i < len; i++)
	{
		if(str[i] == ':')
		{
			for(j = i; j < len; j++)
			{
				str[j] = str[j + 1];
			}
			len--;
			i--;	
		} 
	}	
}

static int gl_tools_hexStr2bytes(const char *hexstr, int strlen, unsigned char *bytes)
{
	int i;
	int cnt = 0;

	for (i = 0; i < strlen; i++)
	{
		if (!isxdigit((int)hexstr[i]))
		{
			continue;
		}

		if ((hexstr[i] >= '0') && (hexstr[i] <= '9'))
			*bytes = (hexstr[i] - '0') << 4;
		else
			*bytes = (toupper((int)hexstr[i]) - 'A' + 10) << 4;

		i++;

		if ((hexstr[i] >= '0') && (hexstr[i] <= '9'))
			*bytes |= hexstr[i] - '0';
		else
			*bytes |= toupper((int)hexstr[i]) - 'A' + 10;

		bytes++;
		cnt++;
	}
	return cnt;
}
static int ble_gap_cb(gl_ble_gap_event_t event, gl_ble_gap_data_t *data)
{
	char address[BLE_MAC_LEN] = {0};
	char mac[18];
	FILE *file=fopen("/sys/class/net/eth0/address","r");
	fscanf(file,"%s",mac);
	fclose(file);
	removecolon(mac);
	int beacontype = 2;
	int numrec = 1;
    int numdec = 1;
	int rectype = 2;
	int radflag = 100;
	int packflag = 3856;
	int numpack = 1;
	char servip[16];
	FILE *file2=fopen("/root/raddec.conf","r");
	fscanf(file2,"%s",servip);
	fclose(file2);
	switch (event)
	{
	case GAP_BLE_SCAN_RESULT_EVT:
	{
		addr2str(data->scan_rst.address, address);
    	char beacon[15];
    	snprintf(beacon,sizeof(beacon),"%02d%s",beacontype,address);
    	int rssi=data->scan_rst.rssi+127;
    	char rec[25];
    	snprintf(rec,sizeof(rec),"%02d%x%02d%02d%s",numrec,rssi,numdec,rectype,mac);
		char packet[100];
		snprintf(packet,sizeof(packet),"%s",data->scan_rst.ble_adv);
		int packlength = strlen(packet)/2;
		char data[125];
		snprintf(data,sizeof(data),"%x%d%02x%s",packflag,numpack,packlength,packet);
    	int length = (sizeof(radflag)+sizeof(beacon)+sizeof(rec)+strlen(data))/2-1;
    	char base[length*2+1];
    	snprintf(base,sizeof(base),"%d%03x%s%s%s",radflag,length,beacon,rec,data);
    	char raddec[length*2+3];
    	unsigned int checksum = 0;
    	int i;
    	char j[4];
    	int k;
    	for(i=0;i<(length-1);++i)
    	{
      		sprintf(j,"%c%c",base[i*2],base[i*2+1]);
      		k=strtol(j,NULL,16);
      		checksum+=k;
    	}
    	char checkstring[3];
    	snprintf(checkstring,sizeof(checkstring),"%02x",checksum%256);
    	snprintf(raddec,sizeof(raddec),"%s%s",base,checkstring);
		char *tmp;
		tmp = strstr(raddec, "ac233f");
		char radout[strlen(raddec)/2+1];
		gl_tools_hexStr2bytes(raddec,sizeof(raddec),radout);
		extern int errno;
		int sock;
		struct sockaddr_in serv;
		sock=socket(AF_INET,SOCK_DGRAM,0);
		if(sock < 0){
        	perror("Error while creating socket");
        	printf("%d\n",errno);
			}
		serv.sin_family = AF_INET;    
  		serv.sin_addr.s_addr = inet_addr(servip);
  		serv.sin_port = htons(50001);
		memset(serv.sin_zero, '\0', sizeof serv.sin_zero);
    	if (tmp)
			{
			if (sendto(sock, radout, strlen(raddec)/2, 0 , (struct sockaddr *) &serv, sizeof(serv))==-1)
				{
					perror("Error sending data");
					printf("%d\n",errno);
				}

			}
		close(sock);
		break;
	}
	default:
		break;
	}

	return 0;
}

static int ble_module_cb(gl_ble_module_event_t event, gl_ble_module_data_t *data)
{
	switch (event)
	{
	case MODULE_BLE_SYSTEM_BOOT_EVT:
	{
		module_work = true;
		json_object *o = NULL;
		o = json_object_new_object();
		json_object_object_add(o, "type", json_object_new_string("module_start"));
		json_object_object_add(o, "major", json_object_new_int(data->system_boot_data.major));
		json_object_object_add(o, "minor", json_object_new_int(data->system_boot_data.minor));
		json_object_object_add(o, "patch", json_object_new_int(data->system_boot_data.patch));
		json_object_object_add(o, "build", json_object_new_int(data->system_boot_data.build));
		json_object_object_add(o, "bootloader", json_object_new_int(data->system_boot_data.bootloader));
		json_object_object_add(o, "hw", json_object_new_int(data->system_boot_data.hw));
		json_object_object_add(o, "ble_hash", json_object_new_string(data->system_boot_data.ble_hash));
		const char *temp = json_object_to_json_string(o);
		printf("MODULE_CB_MSG >> %s\n", temp);

		json_object_put(o);
		break;
	}
	default:
		break;
	}

	return 0;
}

static void sigal_hander(int sig)
{
	printf("\nbleRaddec exit!\n");

	gl_ble_stop_discovery();
	gl_ble_unsubscribe();
	gl_ble_destroy();

	exit(0);
}
