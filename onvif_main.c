/***************************************************************************************
 *
 *  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
 *
 *  By downloading, copying, installing or using the software you agree to this license.
 *  If you do not agree to this license, do not download, install, 
 *  copy or use the software.
 *
 *  Copyright (C) 2014-2019, Happytimesoft Corporation, all rights reserved.
 *
 *  Redistribution and use in binary forms, with or without modification, are permitted.
 *
 *  Unless required by applicable law or agreed to in writing, software distributed 
 *  under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 *  CONDITIONS OF ANY KIND, either express or implied. See the License for the specific
 *  language governing permissions and limitations under the License.
 *
****************************************************************************************/
#include "sys_inc.h"
#include "set_config.h"
#include "sys_buf.h"
#include "http_parse.h"
#include "onvif_api.h"
#include "onvif.h"
#include "ntp_conf.h"

#define VISCA_COM	"/dev/ttyAMA3"
#define PTZ_COM     "/dev/ttyAMA2"

void init_network()
{
#if __WINDOWS_OS__
    WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif	
}

void onvifStart(int     bdebug)
{
#if 0 //heguohuo
#if __LINUX_OS__
	
#ifndef DEBUG
		daemon_init();
#endif
	
		struct rlimit limit;
		limit.rlim_cur = _CORE_SIZE;
		limit.rlim_max = _CORE_SIZE;
		
		setrlimit(RLIMIT_CORE, &limit);
		
#endif
#endif
    //设备初始化
    int ret = devInit(PTZ_COM, VISCA_COM);
    if (ret == 0)
		printf("++++ dev init successful. +++++\r\n");

	//打开onvif log开关
	if (1 == bdebug) {
    	logOpen();
	}

	sys_buf_init(32);
	http_msg_buf_init(16);

    init_network();
    onvif_init();    ////

	onvif_start(); 
	
	//开启ntp线程
	ntp_update_run();
}

void onvifStop(void)
{
	onvif_stop();
	
	sys_buf_deinit();
	http_msg_buf_deinit();
}

