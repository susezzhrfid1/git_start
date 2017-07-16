/*
 * Copyright (c) 2001, Swedish Institute of Computer Science.
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 * 3. Neither the name of the Institute nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 * $Id: sys_arch.c,v 1.3 2002/01/02 17:35:42 adam Exp $
 */

//#include "lwip/debug.h"

#define WIN32_LEAN_AND_MEAN

//#include "Const.h"
#include "lwip/stats.h"
#include "lwip/api.h"
//#include "tGeneral.h"
#include "task.h"
#include "sio.h"
#include "debug.h"

#include "FaCfg.h"
#include "Sysarch.h"

struct sys_timeouts timeouts;
sys_sem_t sem_sys_prot;

#if 0     //û�з�ֹ��Ƭ����
/*-----------------------------------------------------------------------------------*/
sys_sem_t sys_sem_new(u8_t count)         //socket ����ʱ�ᴴ���ź���
{
    xSemaphoreHandle  xSemaphore;

	vSemaphoreCreateBinary( xSemaphore );
	
	if( xSemaphore == NULL )
	{
		return SYS_SEM_NULL;	// TODO need assert
	}
	
	if(count == 0)	// Means it can't be taken
	{
		xSemaphoreTake(xSemaphore,1);
	}
		
	return xSemaphore;
}

sys_sem_t sys_sem_new_m(u8_t count, u8_t max)  
{
    return xSemaphoreCreateCounting(max, count);
}
/*
sys_sem_t new_period_sem(u32_t milliseconds)
{
	return VDK::CreateSemaphore(0, 1, milliseconds*TICKPERMS, milliseconds*TICKPERMS);    
}*/

/*-----------------------------------------------------------------------------------*/
void sys_sem_free(sys_sem_t sem)        //�ر�socketʱ�������ź���
{			
	vQueueDelete( sem );
}
#endif
#if 1
//��ֹ��Ƭ����                     SOCKET����ر�ʱ��Ƶ�������ź���
typedef struct TSemItem
{
    sys_sem_t sem;
    bool fUsed;            //�Ƿ���ʹ��    
    struct TSemItem *pTSemNext;   //ָ����һ��������ź���
}TSemItem;

static TSemItem *g_pTSemHead = NULL;

/*-----------------------------------------------------------------------------------*/
sys_sem_t sys_sem_new(u8_t count)         //socket ����ʱ�ᴴ���ź���
{    
    return sys_sem_new_m(count, 1);
}

//volatile static DWORD g_mallocCnt2 = 0;
//volatile static DWORD g_mallocNum2 = 0;

sys_sem_t sys_sem_new_m(u8_t count, u8_t max)
{
    TSemItem *pTSem = g_pTSemHead;  
    
    OsEnterCritical();
    
    //1 �����������ѷ��䵫��û��ʹ�õ��ź���
    if (pTSem != NULL)
    {
        do     //���ѷ�����ź���
        {
            if (!pTSem->fUsed)   //�ź���û��ʹ��
            {        
                //�ź������Ͳ��Ƚ��ˣ���Ϊ�������ź���Ҳ��ͨ��COUNT����������                
                if (GetQueueLength(pTSem->sem) == max)  //�ź������ȱ���һ�²���ʹ��
                {
                    //���³�ʼ������ź���   
                    ReInitQueue(pTSem->sem);
                    SetQueueMessage(pTSem->sem, count);
                }
                    
                pTSem->fUsed = true;
                OsExitCritical();
                return pTSem->sem;                
            }
            if (pTSem->pTSemNext != NULL)
                pTSem = pTSem->pTSemNext; //������һ���ź���
            else
                break;
        }while(1);
    }
    
    //2 �����µ��ź���    
    xSemaphoreHandle  xSemaphore;

    xSemaphore = xSemaphoreCreateCounting(max, count);
	
	if( xSemaphore == NULL )
	{        
        OsExitCritical();
		return SYS_SEM_NULL;	// TODO need assert
	}
	
    TSemItem *pTSemNew = NULL;
    pTSemNew = (TSemItem *)malloc(sizeof(TSemItem));
#ifndef NDEBUG    
    StatMallocSize(sizeof(TSemItem));
#endif
    //g_mallocCnt2++;
    //g_mallocNum2 += sizeof(TSemItem);
    
    if (pTSemNew == NULL)
    {
        vQueueDelete( xSemaphore );
        OsExitCritical();
        DTRACE(DB_CRITICAL, ("LWIP:failed to malloc sem!\r\n"));
        return SYS_SEM_NULL;
    }
    pTSemNew->sem = xSemaphore;
    pTSemNew->fUsed = true;
    pTSemNew->pTSemNext = NULL;
    
    if (g_pTSemHead == NULL)
    {
        g_pTSemHead = pTSemNew;
    }
    else
        pTSem->pTSemNext = pTSemNew;      //�������������
		
    OsExitCritical();
	return xSemaphore;
}

/*-----------------------------------------------------------------------------------*/
void sys_sem_free(sys_sem_t sem)        //�ر�socketʱ�������ź���
{			
    TSemItem *pTSem = g_pTSemHead;
    
    OsEnterCritical();
    
    //1 �����Ƿ���������
    while(pTSem != NULL)
    {
        if (pTSem->sem == sem)
        {
            pTSem->fUsed = false;
            //vQueueUnregisterQueue( pTSem->sem );
            OsExitCritical();
            return;
        }
        pTSem = pTSem->pTSemNext;
    }    
    
    //2 ������������ɾ���ź���
	vQueueDelete( sem );
    OsExitCritical();
}
#endif


/*-----------------------------------------------------------------------------------*/
void sys_sem_wait(sys_sem_t sem)
{
    xSemaphoreTake( sem, portMAX_DELAY );    
}

err_t sys_sem_wait_t(sys_sem_t sem, u32_t timeout)
{
	WORD wRet = 0;
	if (sem == NULL)
		return SYS_ERR_SYS;
    if (timeout == 0)
        timeout = portMAX_DELAY;    //block indefinitely
    else if (timeout == SYS_GET_SEM_NO_WAIT)
        timeout = 0;                //poll the semaphore
    wRet = xSemaphoreTake( sem, timeout );
    if (wRet == 1)
        return SYS_ERR_OK;
    else 
        return SYS_ERR_TIMEOUT;
}

/*-----------------------------------------------------------------------------------*/
void sys_sem_signal(sys_sem_t sem)
{
	xSemaphoreGive( sem );
}

/*-----------------------------------------------------------------------------------*/
struct sys_timeouts *
sys_arch_timeouts(void)
{
  return &timeouts;
}

typedef void (* pppMainFun)(void *arg);
typedef struct 
{
    void *arg;
    pppMainFun pMain;    
    bool fFstCreate;    //��һ�δ�����־
    xTaskHandle tTaskHandle;
}TPppMain;

//TPppMain *g_tPppMain;

static TPppMain tPppMain;

void PppMainInit(void)
{
    tPppMain.pMain = NULL;
    tPppMain.arg = NULL;
    tPppMain.fFstCreate = 1;
    tPppMain.tTaskHandle = NULL;
}

void MonitorPppMain(void *pvParameters)
{        
    for ( ; ; )
    {        
        if (tPppMain.pMain != NULL)
        {
            tPppMain.pMain(tPppMain.arg);    
            //pppClose();
            tPppMain.pMain = NULL;
        }
        Sleep(1000);
    }
}

/*-----------------------------------------------------------------------------------*/
void sys_init(void)
{
  timeouts.next = NULL;
  timeouts.lasttime = 0;

  sem_sys_prot = sys_sem_new(1);
  
  PppMainInit();
  
  return;
}

u32_t sys_now(void)
{
	return xTaskGetTickCount();
}

/*-----------------------------------------------------------------------------------*/
//void sys_thread_new(int (* function)(void *arg), void *arg, DWORD dwStackSize, int prio)
sys_thread_t sys_thread_new(char *name, void (* thread)(void *arg), void *arg, int stacksize, int prio)
{
	/*TThreadData ThreadData;
	ThreadData.function = function;
	ThreadData.arg = arg;  	*/
    xTaskHandle CreatedTask;
    int result;
        
    if (strcmp(name, PPP_THREAD_NAME)==0) //�߳���ΪPPP_THREAD_NAME�������̶��Ŀռ�
    {           
       if (tPppMain.fFstCreate == 1)  //�״δ���PPPMain�߳�     NOTE��tPppMainû�б�����ֻ��һ���̴߳���PPPMAIN������������
       {
           //NewThread(MonitorPppMain, NULL, 70, THREAD_PRIORITY_IDLE);               
           result = xTaskCreate( MonitorPppMain, ( signed portCHAR * ) name, stacksize, NULL, prio, &CreatedTask );
           if (result == pdPASS)
           {
               tPppMain.fFstCreate = 0;
               tPppMain.tTaskHandle = CreatedTask;
               tPppMain.pMain = thread;
               tPppMain.arg = arg;
           }
       }
       else
       {
           tPppMain.pMain = thread;
           tPppMain.arg = arg;
           return tPppMain.tTaskHandle;
       }
    }
    else
       result = xTaskCreate((pdTASK_CODE)thread, ( signed portCHAR * ) name, (unsigned short)stacksize, arg, prio, &CreatedTask );    
    if(result == pdPASS)
	{
        //DTRACE(1, ("ThreadCreate : success to create %s thread!\r\n", name));
	    return CreatedTask;
	}
	else
	{
        DTRACE(DB_CRITICAL, ("ThreadCreate : failed to create %s thread!\r\n", name));
	    return NULL;
    }
}



/*-----------------------------------------------------------------------------------*/
sys_mbox_t sys_mbox_new(void)
{
  struct sys_mbox *mbox;

  mbox = (struct sys_mbox* )mem_malloc(sizeof(struct sys_mbox));
  mbox->first = mbox->last = 0;
  mbox->mail = (sys_sem )sys_sem_new_m(0, SYS_MBOX_SIZE);
  mbox->space = (sys_sem )sys_sem_new_m(SYS_MBOX_SIZE, 
                                                SYS_MBOX_SIZE);
  mbox->mutex = (sys_sem )sys_sem_new(1);
    
  return mbox;
}

/*-----------------------------------------------------------------------------------*/
void
sys_mbox_free(struct sys_mbox *mbox)
{
  if(mbox != SYS_MBOX_NULL) 
  {
    sys_sem_wait(mbox->mutex);
    
    sys_sem_free(mbox->mail);
    sys_sem_free(mbox->space);
    sys_sem_free(mbox->mutex);
    mbox->mail = mbox->mutex = SYS_SEM_NULL;
    /*  DEBUGF("sys_mbox_free: mbox 0x%lx\r", mbox);*/
    mem_free(mbox);
  }
}



/*-----------------------------------------------------------------------------------*/
//����������ȡ�ö������ռ�ã�Ȼ�����Ϣ������Ϣ���У�����ͷŶ������ռ��
//     ����Ϣ������Ϣ���У�ʹ������֪����û������Ϣ����ͨ��������ź��������ǵ��������ͷָ��ʹ��
//     ������βָ�롣������ź���ֻ�Ǳ�֤����������Ļ��⡣
//     �μ�sys_arch_mbox_fetch()
void sys_mbox_post(struct sys_mbox *mbox, void *msg)
{
  sys_sem_wait(mbox->space);      //�ȵȴ��������пռ���ʼ�
  sys_sem_wait(mbox->mutex);      //ȡ�ö������ռ��

  /*DEBUGF(SYS_DEBUG, ("sys_mbox_post: mbox %p msg %p first %d last %d\r", 
                     mbox, msg, mbox->first, mbox->last));*/

  mbox->msgs[mbox->last] = msg;   //������Ϣ���е�ͷ
                                  //���msgΪNULL�������һ������Ϣ
  
  mbox->last++;                   //��Ϣ���е�ͷָ��++
  if(mbox->last == SYS_MBOX_SIZE) //Խ��ѭ�����еı߽�
  {
    mbox->last = 0;
  }

  sys_sem_signal(mbox->mail);     //��һ���µ���Ϣ�ˣ�
  
  sys_sem_signal(mbox->mutex);    //�ͷŶ������ռ��
}


/*-----------------------------------------------------------------------------------*/
//������������mbox����û��Ϣ���������ŵ�msg�������NULL�ŵ�msg
//      ������mbox->mail��������Եȴ������ȵ�mail�ź������Ѿ������ѣ������
//������@mboxΪ����,
//      @msg��������ָ��ȡ�õ���Ϣ��ָ�룬������Ϊmsg����Ϊ��
//���أ���
void sys_mbox_get(sys_mbox_t mbox, void **msg)
{
  sys_sem_wait(mbox->mutex);         //ȡ�ö������ռ��

  /*DEBUGF(SYS_DEBUG, ("sys_mbox_get: mbox %p msg %p first %d last %d\r", 
                      mbox, mbox->msgs[mbox->first], mbox->first, mbox->last));*/
  
  if(msg != NULL) 
  {
    *msg = mbox->msgs[mbox->first];   //������Ϣ
  }
  
  mbox->first++;                      //βָ��++����ʹ����msg==NULL��++��ʾ����
  if(mbox->first == SYS_MBOX_SIZE)    //Խ��ѭ�����еı߽�
  {
    mbox->first = 0;
  }    
  
  sys_sem_signal(mbox->space);        //���µĿռ��ˣ�

  sys_sem_signal(mbox->mutex);        //�ͷŶ������ռ��
}

/*-----------------------------------------------------------------------------------*/
//������������mbox��ȡһ����Ϣ��������msg��
//     ������Ϣ�ı�׼�������ͷָ�벻����βָ�롣ȡ�ö�������ź�����ռ�ò�������������Ϣ����ֻ�Ǳ�֤
//     ����������Ļ��⡣
//     �μ�sys_mbox_post()
//������@mboxΪ����,@msg��������ָ��ȡ�õ���Ϣ��ָ�룬
//     @timeout�ȴ���Ϣ��ʱ�䣬=0����ȴ���!=0�ȴ���ʱ��
//���أ���
void sys_mbox_fetch(sys_mbox_t mbox, void **msg)
{
  sys_sem_wait(mbox->mail);          //�ȵȴ����������ʼ�
  sys_mbox_get(mbox, msg);
}

err_t sys_mbox_fetchT(sys_mbox_t mbox, void **msg, u32_t timeout)
{
  //err_t err = sys_sem_wait_t(mbox->mail, timeout*TICKPERMS);       //�ȵȴ����������ʼ�
  err_t err = sys_sem_wait_t(mbox->mail, timeout);       //�ȵȴ����������ʼ�
  if (err == ERR_OK)
  {
    sys_mbox_get(mbox, msg);
    return ERR_OK;
  }
  else
  {
    return err;
  }
}

void sys_mbox_drain(sys_mbox_t mbox, u8_t type)
{
  void* msg;

  /* Drain the recvmbox. */
  if(mbox != SYS_MBOX_NULL) 
  {
    sys_sem_wait(mbox->mutex);          //ȡ�ö������ռ��
    while(mbox->first != mbox->last)    //û����Ϣ
    {
      msg = mbox->msgs[mbox->first++];  //������Ϣ,βָ��++
      if(mbox->first == SYS_MBOX_SIZE)  //Խ��ѭ�����еı߽�
      {
        mbox->first = 0;
      }    

      if (type == MBOX_DRAINTYPE_NETCONN)
      {
        netconn_delete((struct netconn *)msg);
      }
      else if(type == MBOX_DRAINTYPE_PBUF)
      {
        pbuf_free((struct pbuf *)msg);
      } 
      else   //MBOX_DRAINTYPE_NETBUF
      {
      	netbuf_delete((struct netbuf *)msg);
      }
    }
    sys_sem_signal(mbox->mutex);    //�ͷŶ������ռ�ã�
    sys_mbox_free(mbox);
  }
}

u16_t sys_mbox_msgnum(sys_mbox_t mbox)
{
  u16_t num;
  sys_sem_wait(mbox->mutex);         //ȡ�ö������ռ��
  
  num = sys_sem_getval(mbox->mail);
 
  sys_sem_signal(mbox->mutex);       //�ͷŶ������ռ��

  return num;
}

u32_t sys_wait_multiple(u8_t nsema, sys_sem sema[], int fwaitall, u32_t timeout)
{
	u8_t i;
	WORD wRet = 0;
	i=0;
    static u8_t j=0;
	while (1)
	{
        wRet = xSemaphoreTake( sema[i], 20 );//25
        if (wRet == 1)                         //�ȵ����ź���
            return i;      
        j++;
        if (j==10)     //250ms��
        {
            j=0;
            return 1;  //sem[1],250MSʱ�䵽�ˡ�
        }
		i++;
		if (i==nsema) i=0;
	}
}

u32_t sio_write(sio_fd_t fd, u8_t* buf, u32_t len)
{
    /*SYSTEMTIME SystemTime;
    GetSystemTime(&SystemTime);
	DEBUGF(1, ("sio_write: start at %d:%d:%d:%d\r\n",
                         SystemTime.wHour, SystemTime.wMinute,
						 SystemTime.wSecond, SystemTime.wMilliseconds));
	TraceBuf(1, "tx<---", buf, len);*/

	//u32_t ret = fd->Write(buf, len);

    /*GetSystemTime(&SystemTime);
	DEBUGF(1, ("sio_write: finish at %d:%d:%d:%d\r\n",
                         SystemTime.wHour, SystemTime.wMinute,
						 SystemTime.wSecond, SystemTime.wMilliseconds));*/
        
    return CommWrite(COMM_GPRS, buf, len, 1000);
    /*int txlen = CommWrite(COMM_GPRS, buf, len, 1000);
    if (txlen > 0)
        DTRACE(1, ("gprs: tx %d\r\n", len));*/
}

int sio_read(sio_fd_t fd, u8_t* buf, u32_t len)
{
    //int rxlen = fd->Read(buf, len);

	/*if (rxlen > 0)
	{
		SYSTEMTIME SystemTime;
		GetSystemTime(&SystemTime);
		DEBUGF(1, ("sio_read: %d:%d:%d:%d\r\n",
							SystemTime.wHour, SystemTime.wMinute,
							SystemTime.wSecond, SystemTime.wMilliseconds));

		TraceBuf(1, "rx--->", buf, rxlen);
	}*/       

	return CommRead(COMM_GPRS, buf, len, 20);
    
    /*int rxlen = CommRead(COMM_GPRS, buf, len, 200);
    if (rxlen > 0)
        DTRACE(1, ("gprs: rx %d\r\n", len));    */
}

int sio_read_abort(sio_fd_t fd)
{
	return 1;
}