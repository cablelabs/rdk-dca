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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>           /* Definition of AT_* constants */
#include <unistd.h>

#define LEN 14
#define BUF_LEN 16
#define CMD_LEN 256
#define MAXLEN 512
#define PID_SIZE 10
#define PIDOF_SIZE 50
#define MEM_STRING_SIZE 20
#define PROC_PATH_SIZE 50

#include "dcautils.h"

#define MEM_KEY_PREFIX "mem_"
#define CPU_KEY_PREFIX "cpu_"

typedef struct proc_info {
  int           utime;                    /** user mode jiffies **/
  int           stime;                    /** kernel mode jiffies **/
  int		cutime;                   /** user mode jiffies with childs **/
  int           cstime;                   /** kernel mode jiffies with childs **/
  unsigned int  rss;                      /** Resident Set Size **/
} procinfo;

typedef struct _procMemCpuInfo {
  pid_t *pid;
  char processName[BUF_LEN];
  char cpuUse[BUF_LEN];
  char memUse[BUF_LEN];
  int total_instance;
} procMemCpuInfo;

int getProcInfo(procMemCpuInfo *pInfo);
int getMemInfo(procMemCpuInfo *pmInfo);
int getCPUInfo(procMemCpuInfo *pInfo);

int getProcUsage(char *processName) {
  if (processName != NULL) {
    procMemCpuInfo pInfo;
    char pidofCommand[PIDOF_SIZE];
    FILE *cmdPid;
    char *mem_key = NULL, *cpu_key = NULL;
    int ret = 0;
    int index = 0;
    pid_t *pid = NULL;
    pid_t *temp = NULL;

    memset(&pInfo, '\0', sizeof(procMemCpuInfo));
    memcpy(pInfo.processName, processName, strlen(processName)+1); 

    sprintf(pidofCommand, "pidof %s", processName);
    if (!(cmdPid = popen(pidofCommand, "r")))
    {
      LOG("Failed to execute %s", pidofCommand);
      return 0;
    }

    pid = (int *) malloc (sizeof(pid_t));
    if ( NULL == pid )
    {
        pclose(cmdPid);
        return 0;
    }
    *pid=0;
    while(fscanf(cmdPid,"%d",(pid+index)) == 1)
    {
        if ((*(pid+index)) <= 0)
        {
            continue;
        }
        index++;
        temp = (pid_t *) realloc (pid,((index+1)*sizeof(pid_t)) );
        if ( NULL == temp )
        {
           if(pid)
                free(pid);
           pclose(cmdPid);
           return 0;
        }
        pid=temp;
        temp=NULL;
    }
    // If pidof command output is empty
    if ((*pid) <= 0)
    {
        free(pid);
        pclose(cmdPid);
        return 0;
    }

    pInfo.total_instance=index;
    pInfo.pid=pid;
    pclose(cmdPid);

    if(0 != getProcInfo(&pInfo))
    {
      mem_key = malloc(strlen(processName) + strlen(MEM_KEY_PREFIX) + 1);
      cpu_key = malloc(strlen(processName) + strlen(CPU_KEY_PREFIX) + 1);
      if (NULL != mem_key && NULL != cpu_key)
      {
        strcpy(cpu_key, CPU_KEY_PREFIX);
        strcat(cpu_key, processName);

        strcpy(mem_key, MEM_KEY_PREFIX);
        strcat(mem_key, processName);

        addToSearchResult(mem_key, pInfo.memUse);
        addToSearchResult(cpu_key, pInfo.cpuUse);
        ret = 1;
      }
 
      if (mem_key)
        free(mem_key);

      if (cpu_key)
        free(cpu_key);

      if (pid)
        free(pid);

      return ret;
    }
    if (pid)
      free(pid);
  } 
  return 0;
}

/** @description: To get process information of the process.
 *  @parm pid process id of process.
 *  @param pinfo address of process object of process.
 *  @return 1 on success.
 */

int getProcPidStat(int pid, procinfo * pinfo)
{
  char szFileName [CMD_LEN],szStatStr [2048],*s, *t;
  FILE *fp;
  struct stat st;
  int ppid, pgrp, session, tty, tpgid, counter, priority, starttime, signal, blocked, sigignore, sigcatch;
  char exName [CMD_LEN], state;
  unsigned euid, egid;
  unsigned int flags, minflt, cminflt, majflt, cmajflt, timeout, itrealvalue, vsize, rlim, startcode, endcode, startstack, kstkesp, kstkeip, wchan; 

  if (NULL == pinfo)
  {
    LOG("Invalid input(pinfo=NULL) to get process info");
    return 0;
  }

  sprintf (szFileName, "/proc/%u/stat", (unsigned) pid);
  if (-1 == access (szFileName, R_OK))
  {
    LOG("Unable to access file in get proc info");
    return 0;
  }

  if (-1 != stat (szFileName, &st))
  {
    euid = st.st_uid;
    egid = st.st_gid;
  } 
  else
  {
    euid = egid = -1;
  }

  if ((fp = fopen (szFileName, "r")) == NULL)
  {
    LOG("Failed to open file in get process info");
    return 0;
  }

  if ((s = fgets (szStatStr, 2048, fp)) == NULL)
  {
    fclose (fp);
    return 0;
  }

  /** pid **/
  s = strchr (szStatStr, '(') + 1;
  t = strchr (szStatStr, ')');
  strncpy (exName, s, t - s);
  exName [t - s] = '\0';

  sscanf (t + 2, "%c %d %d %d %d %d %u %u %u %u %u %d %d %d %d %d %d %u %u %d %u %u %u %u %u %u %u %u %d %d %d %d %u",
      /*       1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33*/
      &(state),
      &(ppid),
      &(pgrp),
      &(session),
      &(tty),
      &(tpgid),
      &(flags),
      &(minflt),
      &(cminflt),
      &(majflt),
      &(cmajflt),
      &(pinfo->utime),
      &(pinfo->stime),
      &(pinfo->cutime),
      &(pinfo->cstime),
      &(counter),
      &(priority),
      &(timeout),
      &(itrealvalue),
      &(starttime),
      &(vsize),
      &(pinfo->rss),
      &(rlim),
      &(startcode),
      &(endcode),
      &(startstack),
      &(kstkesp),
      &(kstkeip),
      &(signal),
      &(blocked),
      &(sigignore),
      &(sigcatch),
      &(wchan));

  fclose (fp);

  return 1;
}

/** @description: To get cpu and mem info
 *  @parm pInfo - procMemCpuInfo
 *  @return 1 on success.
 */
int getProcInfo(procMemCpuInfo *pmInfo)
{
  if (0 == getMemInfo(pmInfo))
      return 0;

  if (0 == getCPUInfo(pmInfo))
      return 0;

  return 1;
}


/** @description: To get the reserve memory of a given process.
 *  @parm pInfo - procMemCpuInfo
 *  @return 1 on success.
 */
int getMemInfo(procMemCpuInfo *pmInfo)
{
  static char retMem[MEM_STRING_SIZE];
  int intStr = 0,intValue = 0;
  double residentMemory = 0.0;
  procinfo pinfo;
  long pageSizeInKb = sysconf(_SC_PAGE_SIZE) / 1024; /* x86-64 is configured to use 2MB pages */
  unsigned int total_memory=0;
  int index = 0;

  for(index=0;index<(pmInfo->total_instance);index++)
  {
    memset(&pinfo, 0, sizeof(procinfo));
    if( 0 == getProcPidStat(pmInfo->pid[index], &pinfo))
      return 0;
    total_memory+=pinfo.rss;
  }

  residentMemory = total_memory * pageSizeInKb;
  intStr = (int)residentMemory;
  intValue = intStr;
  if (intValue >= 1024)
    intStr = intStr/1024;
  snprintf(retMem, sizeof(retMem), "%d", intStr);
  if (intValue >= 1024)
    strcat(retMem,"m");
  else
    strcat(retMem,"k");

  strncpy(pmInfo->memUse, retMem, strlen(retMem)+1);
  return 1;
}

#ifndef ENABLE_XCAM_SUPPORT

int getCPUInfo(procMemCpuInfo *pInfo)
{
  int ret = 0;
  FILE *inFp = NULL;
  char command[CMD_LEN] = {'\0'};
  char var1[BUF_LEN] = {'\0'};
  char var2[BUF_LEN] = {'\0'};
  char var3[BUF_LEN] = {'\0'};           
  char var4[BUF_LEN] = {'\0'};
  char var5[BUF_LEN] = {'\0'};
  char var6[BUF_LEN] = {'\0'};           
  char var7[BUF_LEN] = {'\0'};           
  char var8[512]= {'\0'};
  char var9[512]= {'\0'}; 
  char var10[512]= {'\0'};          
  int total_cpu_usage=0;
  char top_op[2048]= {'\0'};

  if (pInfo == NULL) {

    return 0;    
  }

#ifdef INTEL
  /* Format Use:  `top n 1 | grep Receiver` */

  sprintf(command, "top -n 1 | grep -i '%s'", pInfo->processName);
#else 
  /* ps -C Receiver -o %cpu -o %mem */
  //sprintf(command, "ps -C '%s' -o %%cpu -o %%mem | sed 1d", pInfo->processName);
  sprintf(command, "top -b -n 1 | grep -i '%s'", pInfo->processName);

#endif


  if(!(inFp = popen(command, "r"))){
    return 0;
  }


  //  2268 root      20   0  831m  66m  20m S   27 13.1 491:06.82 Receiver
#ifdef INTEL
  while(fgets(top_op,2048,inFp)!=NULL) {
    if(sscanf(top_op,"%s %s %s %s %s %s %s %s", var1, var2, var3, var4, var5, var6, var7, var8) == 8) {
      total_cpu_usage += atoi(var7);
      ret=1;
    }
  }
  //#endif
#else
  while(fgets(top_op,2048,inFp)!=NULL) {
    if(sscanf(top_op,"%s %s %s %s %s %s %s %s %s %s", var1, var2, var3, var4, var5, var6, var7, var8, var9, var10) == 10) {
      total_cpu_usage += atoi(var9);
      ret=1;
    }
  }
#endif

  snprintf(pInfo->cpuUse, sizeof(pInfo->cpuUse), "%d", total_cpu_usage);
  pclose(inFp);
  return ret;

}

#else //ENABLE_XCAM_SUPPORT

/** @description: To get total cpu time of the device.
 *  @param totalTime  total time of device.
 *  @return 1 on success.
 */
int getTotalCpuTimes(int * totalTime)
{
  FILE *fp;
  long double a[10];
  int total;

  fp = fopen("/proc/stat","r");

  if(!fp)
    return 0;

  fscanf(fp,"%*s %Lf %Lf %Lf %Lf %Lf %Lf %Lf %Lf %Lf %Lf",
      &a[0],&a[1],&a[2],&a[3],&a[4],&a[5],&a[6],&a[7],&a[8],&a[9]);
  fclose(fp);
  total = (a[0]+a[1]+a[2]+a[3]+a[4]+a[5]+a[6]+a[7]+a[8]+a[9]);
  *totalTime = total;

  return 1;
}

/** @description: To get process cpu utilization of the process.
 *  @parm pid process id of process.
 *  @param procCpuUtil  process cpu utilization of process.
 *  @return 1 on success.
 */
int getProcessCpuUtilization(int pid, int *procCpuUtil)
{
  procinfo pinfo1;
  int no_cpu;
  float total_time_process[2],time[2];
  int t[2];
  float sub1;
  float time1;
  float util=0;


  no_cpu=sysconf(_SC_NPROCESSORS_ONLN);
  if( 0 == getProcPidStat(pid, &pinfo1))
    return 0;

  total_time_process[0]= pinfo1.utime +
    pinfo1.stime +
    pinfo1.cutime +
    pinfo1.cstime;
  //start=pinfo1.starttime;

  if( !getTotalCpuTimes(&t[0]) )
    return 0;

  time[0] = t[0];
  sleep(2);

  if( 0 == getProcPidStat(pid,&pinfo1))
    return 0;

  total_time_process[1]= pinfo1.utime +
    pinfo1.stime +
    pinfo1.cutime +
    pinfo1.cstime;

  if( 0 == getTotalCpuTimes(&t[1]) )
    return 0;

  time[1] = t[1];
  sub1 = total_time_process[1]-total_time_process[0];
  time1= time[1] - time[0];
  util = (sub1/time1)*100*no_cpu;

  if(procCpuUtil)
    *procCpuUtil = util;
  else
    return 0;

  return 1;
}


int getCPUInfo(procMemCpuInfo *pmInfo) {
  int cpu = 0;
  int total_cpu = 0;
  int index = 0;

  for(index=0;index<(pmInfo->total_instance);index++)
  {
    if (0 == getProcessCpuUtilization(pmInfo->pid[index], &cpu))
    {
      continue;
    }
    total_cpu+=cpu;
  }

  snprintf(pmInfo->cpuUse, sizeof(pmInfo->cpuUse), "%.1f", (float)total_cpu);
  return 1;
}

#endif //ENABLE_XCAM_SUPPORT

  /** @} */


/** @} */
/** @} */
