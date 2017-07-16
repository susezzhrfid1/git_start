/*********************************************************************************************************
 * Copyright (c) 2006,���ڿ�½���ӿƼ��ɷ����޹�˾
 * All rights reserved.
 *
 * �ļ����ƣ�Info.cpp
 * ժ    Ҫ�����ļ�ʵ��Ӧ�ò���̼߳���Ϣͨ�ŵĻ���
 * ��ǰ�汾��1.0
 * ��    �ߣ�᯼���
 * ������ڣ�2006��12��
 * ��ע��֪ͨ��Ϣ��ԭ��
 * 		1.�����ڲ�����ϵͳ״̬�����ı�ʱ,֪ͨĳ���߳�����Ӧ����
 �� 		3.�����߶�ΰ���Ϣ�ó�true,�Խ�������˵�൱��ֻ�յ�һ����Ϣ
 �� 		4.��һ����ϢҪ��������߳�ʱ,Ӧ��ÿ���̶߳�����һ����Ϣ�ı�ʶ
 *********************************************************************************************************/
//#include "stdafx.h"
#include "ComConst.h"
#include "FaConst.h"
#include "Info.h"
#include "sysarch.h"
#include <string.h>
//#include "sysapi.h"

static DWORD g_dwInfoClick;
static TSem g_semInfo;
static BYTE g_fInfo[INFO_NUM/8+1];  //ÿλ���Ա�ʾһ����Ϣ
static BYTE g_bInfoDelayFlg[INFO_NUM/8+1];

void InitInfo()
{
	g_dwInfoClick = 0;
	g_semInfo = NewSemaphore(1, 1);
	memset(&g_fInfo, 0, sizeof(g_fInfo));
	memset(&g_bInfoDelayFlg, 0, sizeof(g_bInfoDelayFlg));
}

//����:����֪ͨ��Ϣ
//����:@wID ��Ϣ��ʶ
//	   @fInfo ��Ϣֵ
void SetInfo(WORD wID)
{
	WaitSemaphore(g_semInfo, SYS_TO_INFINITE);
	
	if (wID < INFO_NUM)
		g_fInfo[wID>>3] |= 1<<(wID&7);    
	
	SignalSemaphore(g_semInfo);	
}

//����:���֪ͨ��Ϣ
//����:@wID ��Ϣ��ʶ
//	   @fInfo ��Ϣֵ
void ClrInfo(WORD wID)
{
	WaitSemaphore(g_semInfo, SYS_TO_INFINITE);

	if (wID < INFO_NUM)
        g_fInfo[wID>>3] &= ~(1<<(wID&7));

	SignalSemaphore(g_semInfo);	
}

//����:������ʱ�����֪ͨ��Ϣ,��Ҫ������ݿ���ĵĲ�����ͨ��Э���޸���,
//	   ���޸ĺ������ϾͰ���Ϣ���͸��߳�,������ͨ��Э�鲻���޸Ĳ�����,
//	   �ٰ���Ϣ���͸��߳�
//	   ��ʱ�ַ�Ϊ����ʱ�Ͷ���ʱ
//����:@wID ��Ϣ��ʶ
void SetDelayInfo(WORD wID)
{
	if (wID < INFO_NUM)
	{
		DWORD dwClick = GetClick();

		WaitSemaphore(g_semInfo, SYS_TO_INFINITE);

        g_fInfo[wID>>3] |= 1<<(wID&7);    
		g_dwInfoClick = dwClick;	//��ʹ����ʱ,ʱ�仹�Ǹ���
		if (wID < INFO_NO_DELAY_START)
			g_bInfoDelayFlg[wID>>3] |= 1 << (wID&7);
		
		SignalSemaphore(g_semInfo);
	}
}


//����:ȡ��֪ͨ��Ϣ,�����ϢΪtrue,���Զ�����Ϣ���false
//����:��Ϣֵ
bool GetInfo(WORD wID)
{
	WORD wIndex;
	BYTE bFlg;
	bool fRet = false;
	
	if (wID < INFO_NUM)
	{
		DWORD dwClick = GetClick();
		
		WaitSemaphore(g_semInfo, SYS_TO_INFINITE);
		
		wIndex = wID >> 3;
		bFlg = 1 << (wID&7);
		if (g_bInfoDelayFlg[wIndex] & bFlg) //��������ʱ�����֪ͨ��Ϣ
		{
			if (g_fInfo[wID>>3]&(1<<(wID&7)))
			{	
				if (wID < INFO_SHORT_DELAY_START)
				{	
					if (dwClick-g_dwInfoClick > INFO_LONG_DELAY) //�ȵ�����ʱʱ�䵽�Ű���Ϣ��Ӧ���߳�
						fRet = true;
				}
				else if (wID < INFO_NO_DELAY_START)
				{	
					if (dwClick-g_dwInfoClick > INFO_SHORT_DELAY) //�ȵ�����ʱʱ�䵽�Ű���Ϣ��Ӧ���߳�
						fRet = true;
				}
				else
				{
					fRet = true;
				}
			}
		}
		else if (g_fInfo[wID>>3]&(1<<(wID&7))) //û��������ʱ�����֪ͨ��Ϣ
		{					   //�����ύ	
			fRet = true;
		}
		
		if (fRet)
		{
			g_bInfoDelayFlg[wIndex] &= ~bFlg;
			g_fInfo[wID>>3] &= ~(1<<(wID&7));
		}
		SignalSemaphore(g_semInfo);
	}
	
	return fRet;
}