/*********************************************************************************************************
 * Copyright (c) 2003,深圳科陆电子科技股份有限公司
 * All rights reserved.
 *
 * 文件名称：pppif.cpp
 * 摘    要：本文件主要包含了PPP接口的数据定义和函数实现，遵循lwIP接口层摸板的样式编写而成
 *
 * 当前版本：1.0
 * 作    者：岑坚宇
 * 完成日期：2003年4月25日
 *
 * 取代版本：
 * 原作者  ：
 * 完成日期：
 **********************************************************************************************************/
#ifndef __LWIP_CTRL_H__
#define __LWIP_CTRL_H__

#include "lwip/opt.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"

#include "lwip/ip.h"

#include "lwip/udp.h"
#include "lwip/tcp.h"

#include "lwip/api.h"
#include "ppp.h"

enum ctrl_msg_type 
{
  CTRL_MSG_CLRTIMEOUT,
  CTRL_MSG_MAX
};


struct ctrl_msg
{
  enum ctrl_msg_type type;    //控制命令的类型
  void* arg;                  //控制命令的参数
  sys_mbox_t mbox;            //执行结果返回邮箱
};

void ctrl_msg_input(struct ctrl_msg* msg);
err_t clr_timeout();

bool InitTCPIP(void *arg);

bool ETLinkUp(void);
void SetNetIfDefaultET(void);
void SetNetIfDefaultPPP(void);
struct netif *GetEthIf(void);
struct ip_addr GetEthGW(void);

#endif //__LWIP_CTRL_H__