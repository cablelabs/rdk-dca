/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/



/**
* @defgroup dca
* @{
**/




/**
* @defgroup dca
* @{
* @defgroup src
* @{
**/

//cpu and free memory
#include <stdio.h>

#define MAXLEN 512

int getCpuUsage(char * cpuUtil);


int main()
{
	 char cpuUsage[MAXLEN] = {'\0'};

	 if(getCpuUsage(cpuUsage))
	 {
	     printf("USED_CPU:%s\n", cpuUsage);
	 }
	 else
	 {
	     printf("USED_CPU:NOT AVAILABLE\n");
	 }

	 return 0;
}

/** @description: To get cpu usage of the device.
 *  @param cpuUtil cpuusage of device.
 *  @return 1 on success.
 */
int getCpuUsage(char * cpuUtil)
{
	long double a[10], b[10],usr_cpu,sys_cpu,nic_cpu,idle_cpu, total_time;
	FILE *fp;
	char cpuUtilization[MAXLEN]={'\0'};
	int i=0;

	for(i=0; i<5; i++)
	{
		fp = fopen("/proc/stat","r");
		fscanf(fp,"%*s %Lf %Lf %Lf %Lf %Lf %Lf %Lf %Lf %Lf %Lf",
		&a[0],&a[1],&a[2],&a[3],&a[4],&a[5],&a[6],&a[7],&a[8],&a[9]);
		fclose(fp);
		sleep(1);

		fp = fopen("/proc/stat","r");
		if(!fp)
		{
			return 0;
		}

		fscanf(fp,"%*s %Lf %Lf %Lf %Lf %Lf %Lf %Lf %Lf %Lf %Lf",
		&b[0],&b[1],&b[2],&b[3],&b[4],&b[5],&b[6],&b[7],&b[8],&b[9]);
		fclose(fp);

		total_time=(b[0]+b[1]+b[2]+b[3]+b[4]+b[5]+b[6]+b[7]+b[8]+b[9])
				-
			(a[0]+a[1]+a[2]+a[3]+a[4]+a[5]+a[6]+a[7]+a[8]+a[9]);

		usr_cpu=((b[0]-a[0])/total_time)*100;
		sys_cpu=((b[2]-a[2])/total_time)*100;
		nic_cpu=((b[1]-a[1])/total_time)*100;
		idle_cpu=((b[3]-a[3])/total_time)*100;

		sprintf(cpuUtilization,"%Lf",usr_cpu);
	}

	if(cpuUtil)
	{
		strcpy(cpuUtil,cpuUtilization);
		return 1;
	}
	else
	{
		printf("Exit from getCPUusage due to NULL pointer\n");
		return 0;
	}
}

/** @} */


/** @} */
/** @} */
