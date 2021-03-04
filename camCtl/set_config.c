#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <error.h>
#include <math.h>
#include <net/route.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "set_config.h"
#include "visca_api.h"
#include "ptz.h" 
#include "cfg_file.h"
#include "onvif.h"
#include "utils_log.h"
#include "ir.h"
#include "gptmessage.h"
#include "gptmessagedef.h"
#include "onvif_ptz.h"


extern ONVIF_CFG g_onvif_cfg;
static pthread_mutex_t m_get_ir_param_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t m_fusion_param_lock = PTHREAD_MUTEX_INITIALIZER;

int onvif_get_devinfo(CONFIG_Information * p_devInfo)
{
	strncpy(p_devInfo->manufacturer,"Huaxiaxin",sizeof(p_devInfo->manufacturer)-1);
	strncpy(p_devInfo->model, PRODUCT_NAME, strlen(PRODUCT_NAME));
    strncpy(p_devInfo->firmware_version, FIRMWARE_VERSION, strlen(FIRMWARE_VERSION));
	strncpy(p_devInfo->serial_number,g_onvif_cfg.EndpointReference, strlen(g_onvif_cfg.EndpointReference));
	strncpy(p_devInfo->hardware_id,"1.0",sizeof(p_devInfo->hardware_id )-1);

	if (p_devInfo->manufacturer[0] != '\0' &&
		 p_devInfo->model[0] != '\0' &&
	 	 p_devInfo->firmware_version[0] != '\0' &&
		 p_devInfo->serial_number[0] != '\0' &&
	 	 p_devInfo->hardware_id[0] != '\0')
	{
		return 0;
	}
	UTIL_INFO("p_devInfo->firmware_version==%s", p_devInfo->firmware_version);
}

#define USERSFILE ("/user/cfg_files/User.dat")
int readUsers(onvif_User *p_users, int cnt)
{
	if (read_cfg_from_file(USERSFILE, (char *)p_users, sizeof(onvif_User)*cnt) != 0) {  // read from USERSFILE file
		return -1;
	}

	return 0;	
}
int writeUsers(onvif_User *p_users, int cnt)
{
	if (save_cfg_to_file(USERSFILE, (char*)p_users, sizeof(onvif_User)*cnt) != 0) {
		return -1;
	}

	return 0;
}


int devInit(char *ptzDevID, const char *cameraDEVID)
{
	int ret;

	//visca 设备初始化 
	for(int i=0;i<10;i++)
	{
		ret = visca_init(cameraDEVID);	
		if(ret==0)
			break;
		visca_deinit();
		sleep(1);
	}

#ifdef PTZ_SUPPORT
  ret =RET_ERR;
  ret=pelco_Init(ptzDevID, 9600);
  if (ret != 0) {
    UTIL_ERR("pelco ptz init failed!!!");
  }

  return ret;
#endif 
}


uint16_t  switchToZspeed(float z)
{
	uint16_t speed;
	
	float z_value =  fabs(z);
	speed = (int)(7*z_value);
	
	return  speed;
}


void controlPtzPos(float X, float Y, float Z , unsigned short Speed)
{
	UTIL_INFO(" ctlPTZ:  x=%0.3f , y = %0.3f , z = %0.3f, Speed:%d",X, Y, Z , Speed);

	if (X > 0 && Y == 0) {	//右	    
		if ( pelco_Right(Speed) != RET_OK )   UTIL_INFO("set_config | pelco_Right faile.\n");
	} 
	else if (X < 0 && Y == 0) {	 //左
		if ( pelco_Left(Speed) != RET_OK )   UTIL_INFO("set_config | pelco_Left faile.\n");
	} 
	else if (Y > 0 && X == 0) { //上		
		if ( pelco_Up(Speed) != RET_OK )   UTIL_INFO("set_config | pelco_Up faile.\n");
	} 
	else if(Y < 0 && X == 0)  { //下    
		if ( pelco_Down(Speed) != RET_OK )   UTIL_INFO("set_config | pelco_Down faile.\n");
	} 
	else if (X > 0 && Y > 0) { // 右上		      
		if ( pelco_right_up(Speed) != RET_OK )   UTIL_INFO("set_config | pelco_right_up faile.\n");
	} 
	else if (X > 0 && Y < 0) { // 右下           
		if ( pelco_right_down(Speed) != RET_OK )   UTIL_INFO("set_config | pelco_right_down faile.\n");
	} 
	else if (X < 0 && Y > 0) { // 左上
		if ( pelco_left_up(Speed) != RET_OK )   UTIL_INFO("set_config | pelco_left_up faile.\n");
	} 
	else if (X < 0 && Y < 0) { // 左下
		if ( pelco_left_down(Speed) != RET_OK )   UTIL_INFO("set_config | pelco_left_down faile.\n");
	}

	uint16_t Zspeed = switchToZspeed(Z);
	if (1 == get_visca_status())
	{
		if(Z > 0)
			set_zoom_tele_speed(Zspeed);
		else if(Z < 0)
			set_zoom_wide_speed(Zspeed);
	}
}

void ptzStop()
{
	if (pelco_Stop() != 0)
			printf("set_config | send ptz  Stop error !!!\r\n");

	if (get_visca_status()==1)
		set_zoom_stop();
}


/* PTZ主位置 */
#define  HOMEZOOM  ("/user/cfg_files/HomeZoom.dat")    //保存homePreset的焦距
int readHomePos(CONFIG_Home * p_homeZoom)
{
	if (read_cfg_from_file(HOMEZOOM, (char*)p_homeZoom, sizeof(CONFIG_Home)) != 0) {
		return -1;
	}

	return 0;	
}
int writeHomePos(CONFIG_Home * p_homeZoom)
{
	if (save_cfg_to_file(HOMEZOOM, (char*)p_homeZoom, sizeof(CONFIG_Home)) != 0) {
		return -1;
	}

	return 0;	
}
//

/* 设置预置位 */
void setPtzPreset(unsigned short location)
{
	UTIL_INFO("set_config | pelco_set_point = %d",location);
	pelco_set_point(location);
}
/* 转到预置位 */
void gotoPtzPreset(unsigned short location)
{
	UTIL_INFO("set_config | pelco_get_point = %d",location);
	pelco_get_point(location);
}

/* 保存预置位到文件和从文件读取预置位出来（目的为了防止掉电消失）*/
#define  PRESETFILE  ("/user/cfg_files/Preset.dat")
int readPtzPreset(ONVIF_PTZPreset * p_preset, int cnt)
{
	if (read_cfg_from_file(PRESETFILE, (char*)p_preset, sizeof(ONVIF_PTZPreset)*cnt) != 0) {
		return -1;
	}

	return 0;	
}
int writePtzPreset(ONVIF_PTZPreset * p_preset, int cnt)
{
	if (save_cfg_to_file(PRESETFILE, (char*)p_preset, sizeof(ONVIF_PTZPreset)*cnt) != 0) {
		return -1;
	}

	return 0;	
}
				  
void focusMove(float zoom)
{
	UTIL_INFO("set_config  |  foucs move = %f",zoom);
	
	if(zoom>0)
		set_focus_far();
	else if(zoom<0)
		set_focus_near();	
}


int img_Stop()
{
	set_focus_stop();
	return 0;
}

/* 保存巡更到文件和从文件读取出来（目的为了防止掉电消失）*/
#define  PRESETTOURFILE  ("/user/cfg_files/PresetTour.dat")
/*
int readPresetTourNumber(void)
{
	char* device = "/user/cfg_files/";
	int count = 0;

	DIR *dir;
	struct dirent *ptr;

	if ((dir=opendir(device)) == NULL)
	{
		UTIL_ERR("Open dir error...");
		return -1;
	}

	while ((ptr=readdir(dir)) != NULL)
	{
		if(strstr((char*)ptr->d_name, "PRESET_TOUR_")!=NULL)
		{
			count ++;
		}
	}
	closedir(dir);

	return count;
}
*/
int readPtzPresetTour(PTZ_PresetsTours_t  *presetTours, int cnt)
{
	if (read_cfg_from_file(PRESETTOURFILE, (char*)presetTours, sizeof(PTZ_PresetsTours_t)*cnt) != 0) {
		return -1;
	}

	return 0;	
}
int writePtzPresetTour(PTZ_PresetsTours_t  *presetTours,  int cnt)
{
	if (save_cfg_to_file(PRESETTOURFILE, (char*)presetTours, sizeof(PTZ_PresetsTours_t)*cnt) != 0) {
		return -1;
	}

	return 0;	
}

/* 从文件读取图像参数 */
#define  IMGPARFILE  ("/user/cfg_files/Img.dat")
int getImgParam(ImgParam_t *imgParams)
{
	if (read_cfg_from_file(IMGPARFILE, (char *)imgParams,sizeof(ImgParam_t)) != 0) {

		imgParams->brightness = 50;
		imgParams->contrast = 50;
		imgParams->saturation = 50;
		imgParams->sharp = 50;

		if (save_cfg_to_file((char*)IMGPARFILE, (char*)imgParams, sizeof(ImgParam_t))<0)
			UTIL_ERR("save img param fail\n");

		return -1;
	}

	return 0;
}
/* 保存 图像参数到文件 */
int setImgParam(ImgParam_t *imgParams)
{
	UTIL_INFO("set_config | Brightness= %0.2f ,ColorSaturation= %0.2f , Contrast= %0.2f , Sharpness= %0.2f\n", 
				imgParams->brightness, imgParams->saturation, imgParams->contrast, imgParams->sharp);
			
	/* add your code of set img param here */
	//饱和度0-100
	if(0<=imgParams->saturation && imgParams->saturation<=100)
		set_colorsatuation_value(imgParams->saturation);

	//对比度0-100
	if(0<=imgParams->contrast && imgParams->contrast<=100)
		set_contract_value(imgParams->contrast);

	//亮度0-100
	if(0<=imgParams->brightness && imgParams->brightness<=100)
		set_exp_comp_value(imgParams->brightness);

	//0-15
	if(0<=imgParams->sharp && imgParams->sharp<=100)
	{
		int Sharpness = imgParams->sharp/6;
		if(Sharpness>15)Sharpness=15;
		set_aperture_value(Sharpness);
	}

	//更新数据保存于文件
	ImgParam_t  img_param;
	memset(&img_param, 0 ,sizeof(ImgParam_t));
	if (read_cfg_from_file(IMGPARFILE, (char *)&img_param, sizeof(ImgParam_t)) == 0)
	{
		if ( imgParams->brightness  == -1)	imgParams->brightness = img_param.brightness;
		if ( imgParams->saturation  == -1)	imgParams->saturation = img_param.saturation;
		if ( imgParams->contrast  == -1)	imgParams->contrast = img_param.contrast;
		if ( imgParams->sharp  == -1)	imgParams->sharp = img_param.sharp;
	}

	if (save_cfg_to_file(IMGPARFILE, (char*) imgParams, sizeof(ImgParam_t)) != 0) {
		return -1;
	}

	return 0;
}


#define IR_BASE_PARAM_FILE			"/user/cfg_files/irBase.dat"
//获取热成像基础参数,成功返回0，失败返回-1
int getThermalBaseParam(ThermalBaseParam *param)
{
	if (read_cfg_from_file((char*)IR_BASE_PARAM_FILE, (char*)param, sizeof(ThermalBaseParam))<0)
	{
		param->actime = 300;
		param->orgData = 0;
		param->userPalette = 2;
		param->wideDynamic = 0;
		UTIL_ERR("read ir base param fail\n");
		
		//保存参数
		if (save_cfg_to_file((char*)IR_BASE_PARAM_FILE, (char*)param, sizeof(ThermalBaseParam))<0)
			UTIL_ERR("save ir base param fail\n");
	}
	return 0;
}



/*  保存 热成像参数配置_1 */
int setThermalParam1(ThermalBaseParam *thermalParam1)
{
    GPTMessageSend(GPT_MSG_IR_SETBASEPARAM, 0, (int)thermalParam1, sizeof(ThermalBaseParam));
	//保存参数
	if (save_cfg_to_file((char*)IR_BASE_PARAM_FILE, (char*)thermalParam1, sizeof(ThermalBaseParam))<0)
		UTIL_ERR("save ir base param fail\n");

	return 0;
}

#define IR_ENV_PARAM_FILE			"/user/cfg_files/irEnv.dat"
//获取热成像环境参数,成功返回0，失败返回-1
int getThermalEnvParam(ThermalEnvParam *param)
{
	if (read_cfg_from_file((char*)IR_ENV_PARAM_FILE, (char*)param, sizeof(ThermalEnvParam))<0)
	{
		param->emissivity = 0.95;           //发射率    
		param->distance=3.0;             //距离
		param->humidity=0.45;             //湿度
		param->correction=0;           //修正
		param->reflection=25;           //反射温度
		param->amb=25;                  //环境温度

		UTIL_ERR("read ir env param fail\n");
		
		//保存参数
		if (save_cfg_to_file((char*)IR_ENV_PARAM_FILE, (char*)param, sizeof(ThermalEnvParam)) < 0)
			UTIL_ERR("save fusion param fail\n");
	}
	return 0;

}

/* 保存 热成像参数配置_2*/
int setThermalParam2(ThermalEnvParam *thermalParam2)
{
	//设置热成像环境参数，成功返回0，失败返回-1
	GPTMessageSend(GPT_MSG_IR_SETENVPARAM, 0, (int)thermalParam2, sizeof(ThermalEnvParam));
	//保存参数
	if (save_cfg_to_file((char*)IR_ENV_PARAM_FILE, (char*)thermalParam2, sizeof(ThermalEnvParam)) < 0)
		UTIL_ERR("save fusion param fail\n");

	return 0;
}

#define FUSION_PARAM_FILE			"/user/cfg_files/fusion.dat"
//获取参数,成功返回0，失败返回-1
int getFusionParam(DulaInformation_t *dalaInfo)
{
	//system_ex("chmod 777 %s\n",FUSION_PARAM_FILE);
	if (read_cfg_from_file((char*)FUSION_PARAM_FILE,(char*)dalaInfo, sizeof(DulaInformation_t)) < 0)
	{
		dalaInfo->focal = 1;
		dalaInfo->weightIrY = 0.5f;
		dalaInfo->weightIrC = 0.5f;

		dalaInfo->dula_model = 0;
		dalaInfo->x = 100;
		dalaInfo->y = 100;
		dalaInfo->scale = 2.40;
		//保存参数
		if (save_cfg_to_file((char*)FUSION_PARAM_FILE,(char*) dalaInfo, sizeof(DulaInformation_t))<0) 
		{
			UTIL_ERR("save fusion param fail");	
		}

		return 0;
	}
	else 
	{		
		//UTIL_INFO("focal=%d weightIrY=%3f weightIrC=%3f dula_model=%d x=%d  y=%d scale=%3f", 
		//	dalaInfo->focal, dalaInfo->weightIrY, dalaInfo->weightIrC, dalaInfo->dula_model,dalaInfo->x,dalaInfo->y, dalaInfo->scale);
        float b = -1.00;
		if ( dalaInfo->focal == -1) 				      dalaInfo->focal = 1;
		if ((fabs((dalaInfo->weightIrY)-(b))) < (1e-8))    dalaInfo->weightIrY = 0.5f;
		if ((fabs((dalaInfo->weightIrC)-(b))) < (1e-8))    dalaInfo->weightIrC = 0.5f;
		if ( dalaInfo->dula_model == -1)                  dalaInfo->dula_model = 0;
		if ( dalaInfo->x == (signed short int)-1) 		  dalaInfo->x = 100;
		if ( dalaInfo->y == (signed short int)-1) 		  dalaInfo->y = 100;
		if ((fabs((dalaInfo->scale)-(b))) < (1e-8)) 		  dalaInfo->scale = 2.4;
	}
	UTIL_INFO("focal=%d weightIrY=%3f weightIrC=%3f dula_model=%d x=%d  y=%d scale=%3f", 
		dalaInfo->focal, dalaInfo->weightIrY, dalaInfo->weightIrC, dalaInfo->dula_model,dalaInfo->x,dalaInfo->y, dalaInfo->scale);

	
	return 0;
}

/* 保存 dula数据参数 */
int setDulaParam(DulaInformation_t *dulaInfo)
{
	//更新数据保存于文件
	//此处包含自我修复的功能，在获取参数不合法的情况下，参数自动修复为设备可以正常操作的参数
	DulaInformation_t  readDulaInfo;
	memset(&readDulaInfo, 0 ,sizeof(DulaInformation_t));
	if (read_cfg_from_file(FUSION_PARAM_FILE, (char *)&readDulaInfo, sizeof(DulaInformation_t)) == 0 && dulaInfo)
	{
		UTIL_INFO("focal:%d, weightIrY:%0.2f, distance:%0.2f, dula_model:%d, x:%d, y:%d, xscale:%0.2f,%0.2f", 
				dulaInfo->focal, dulaInfo->weightIrY, dulaInfo->weightIrC, dulaInfo->dula_model, 
				dulaInfo->x, dulaInfo->y, dulaInfo->scale, readDulaInfo.scale); 
		float b = -1.00;
		//切换模式focal:-1, weightIrY:-1.00, weightIrC:-1.00, dula_model:0, x:-1, y:-1, xscale:-1.00
		if ((dulaInfo->focal == -1) && (fabs((dulaInfo->weightIrY)-(b))) < (1e-8)
			&& (fabs((dulaInfo->weightIrC)-(b))) < (1e-8) && (dulaInfo->x == (signed short int)-1) 
			&& (dulaInfo->y == (signed short int)-1) && (fabs((dulaInfo->scale)-(b))) < (1e-8)
			&& dulaInfo->dula_model ==  readDulaInfo.dula_model)
		{
			return 0;
		}
						
		if ((dulaInfo->focal == -1) && (fabs((dulaInfo->weightIrY)-(b))) < (1e-8)
			&& (fabs((dulaInfo->weightIrC)-(b))) < (1e-8) && (dulaInfo->x == (signed short int)-1) 
			&& (dulaInfo->y == (signed short int)-1) && (fabs((dulaInfo->scale)-(b))) < (1e-8)
			&& dulaInfo->dula_model !=  readDulaInfo.dula_model)
		{
			readDulaInfo.dula_model = dulaInfo->dula_model;
			goto __EXIT;
		}
		
		if ( dulaInfo->focal != readDulaInfo.focal)		
		{
			readDulaInfo.focal = dulaInfo->focal;
		}
		if (!((fabs((dulaInfo->weightIrY)-(readDulaInfo.weightIrY))) < (1e-8)))
		{
			readDulaInfo.weightIrY = dulaInfo->weightIrY;
		}
		
		if (!((fabs((dulaInfo->weightIrC)-(readDulaInfo.weightIrC))) < (1e-8)))	
		{
			readDulaInfo.weightIrC = dulaInfo->weightIrC;
		}
		
		if (dulaInfo->dula_model != readDulaInfo.dula_model)                  
		{
			readDulaInfo.dula_model = dulaInfo->dula_model;
		}

		if (dulaInfo->x != readDulaInfo.x)		
		{
			readDulaInfo.x = dulaInfo->x;
		}
		
		if (dulaInfo->y != readDulaInfo.y)		
		{
			readDulaInfo.y = dulaInfo->y;
		}
		
		if (!((fabs((dulaInfo->scale)-(readDulaInfo.scale))) < (1e-8)) )
		{
			readDulaInfo.scale = dulaInfo->scale;
		}
	}

__EXIT:
	/* add your code of set Thermal Param2 here */
	GPTMessageSend(GPT_MSG_DULA_SETFUSIONPARAM, 0, (int)&readDulaInfo, sizeof(DulaInformation_t));

	if (save_cfg_to_file(FUSION_PARAM_FILE, (char*)&readDulaInfo, sizeof(DulaInformation_t)) != 0) {
		return -1;
	}
	
	UTIL_INFO("focal:%d, weightIrY:%0.2f, distance:%0.2f, dula_model:%d, x:%d, y:%d, xscale:%0.2f", 
			readDulaInfo.focal, readDulaInfo.weightIrY,
					readDulaInfo.weightIrC, readDulaInfo.dula_model, 
					readDulaInfo.x, readDulaInfo.y, readDulaInfo.scale); 

	return 0;
}

#define  AUDIO_ENCODER_FILE  ("/user/cfg_files/AudioEncoder.dat")
/* 读取 设置音频编码器参数 */
int getAudioEncoder(Audio_Encoder *p_audio_encoder)
{
	if (read_cfg_from_file(AUDIO_ENCODER_FILE, (char *)p_audio_encoder,sizeof(Video_Encoder)) != 0) {
		//如果读取失败，则初始化它
		p_audio_encoder->session_timeout = 10;
		p_audio_encoder->sample_rate = 8;	 //采样率
		p_audio_encoder->bitrate = 64;		 //码率
		strncpy(p_audio_encoder->a_encoding,g711,32); //编码

		UTIL_ERR("read Audio Encoder param fail\n");
		
		//保存初始参数
		if (save_cfg_to_file(AUDIO_ENCODER_FILE, (char*)p_audio_encoder, sizeof(Video_Encoder)) < 0)
			UTIL_ERR("save Audio Encoder param fail\n");
	}

	return 0;
}

#define VIDEO_ENCODER_FILE  ("/user/cfg_files/VideoEncoder.dat")
/* 读取 视频编码器参数 */
int getVideoEncoder(Video_Encoder *p_video_encoder)
{
	if (read_cfg_from_file(VIDEO_ENCODER_FILE, (char *)p_video_encoder,sizeof(Video_Encoder)) != 0) {
		//如果读取失败，则初始化它
		p_video_encoder->width = 1920;	  //分辨率 宽
		p_video_encoder->height = 1080;	  //分辨率 高	
		p_video_encoder->quality = 4;
		p_video_encoder->session_timeout = 10;
		p_video_encoder->framerate = 25;	  //帧率	
		p_video_encoder->encoding_interval = 50;
		p_video_encoder->bitrate_limit = 2048;	//码流(码率) 

		p_video_encoder->video_encoding.v_encoding = VideoEncoding_H264; 
		p_video_encoder->video_encoding.v_encoding_profile.gov_length = 50; //GOP(关键帧)
		p_video_encoder->video_encoding.v_encoding_profile.encode_profile = H264Profile_Main; //编码级别
		
		UTIL_ERR("Init read Video Encoder param fail");
		
		//保存初始参数
		if (save_cfg_to_file(VIDEO_ENCODER_FILE, (char*)p_video_encoder, sizeof(Video_Encoder)) < 0)
			UTIL_ERR("save Video Encoder param fail");
	}

	return 0;
}
/* 设置 视频编码器参数 ,同时保存到文件 */
int setVideoEncoder(Video_Encoder *p_video_encoder)
{
	UTIL_INFO("width:%d,height:%d, bitrate_limit:%d, framerate:%d, GOP:%d, encode_profile:%d(0:Baseline 1:Main)",
				p_video_encoder->width, p_video_encoder->height,
				p_video_encoder->bitrate_limit,
				p_video_encoder->framerate,
				p_video_encoder->video_encoding.v_encoding_profile.gov_length,
				p_video_encoder->video_encoding.v_encoding_profile.encode_profile );
	
	Video_Encoder  readCameraEncoder;
	memset(&readCameraEncoder, 0 ,sizeof(Video_Encoder));

	if (read_cfg_from_file(VIDEO_ENCODER_FILE, (char *)&readCameraEncoder, sizeof(Video_Encoder)) < 0){
		UTIL_ERR("read Camera Encoder param fail\n");
	}

	//判断读出的分辨率是否符合
	if ( readCameraEncoder.width != 1920 && readCameraEncoder.height != 1080 ||
		 readCameraEncoder.width != 1280 && readCameraEncoder.height != 720 ||
		 readCameraEncoder.width != 720 && readCameraEncoder.height != 576 ||
		 readCameraEncoder.width != 640 && readCameraEncoder.height != 360 ||
		 readCameraEncoder.width != 352 && readCameraEncoder.height != 288 )	
	{
		readCameraEncoder.width = 1920;
		readCameraEncoder.height = 1080;
	}
	//判断读出的编码器的码流(码率)
	if (readCameraEncoder.bitrate_limit < 1 || readCameraEncoder.bitrate_limit > 4096)
	{
		readCameraEncoder.bitrate_limit = 2048;
	}
	//判断读出的编码器的帧率
	if (readCameraEncoder.framerate < 1 || readCameraEncoder.framerate > 30 )
	{
		readCameraEncoder.framerate = 25;
	}
	//判断读出的编码器的GOP(关键帧)
	if (readCameraEncoder.video_encoding.v_encoding_profile.gov_length < 10 ||
	    readCameraEncoder.video_encoding.v_encoding_profile.gov_length > 60 )
	{
		readCameraEncoder.video_encoding.v_encoding_profile.gov_length = 50;
	}
	//判断读出的编码器的编码级别
	if (readCameraEncoder.video_encoding.v_encoding_profile.encode_profile != 0 ||
		readCameraEncoder.video_encoding.v_encoding_profile.encode_profile != 1 )
	{
		readCameraEncoder.video_encoding.v_encoding_profile.encode_profile = 1;
	}


	//设置分辨率
	//onvif_VideoResolution resolution;
	//resolution.Width = p_video_encoder->width;
	//resolution.Height = p_video_encoder->height;

	u32_t bitrate = p_video_encoder->bitrate_limit;  	       //码流(码率)
	u32_t framerate = p_video_encoder->framerate; 		       //帧率
	u32_t gop = p_video_encoder->video_encoding.v_encoding_profile.gov_length;  //GOP(关键帧)
	u32_t encode_profile = p_video_encoder->video_encoding.v_encoding_profile.encode_profile;  //编码级别

	
	//设置编码器的码流(码率)
	if( p_video_encoder->bitrate_limit != readCameraEncoder.bitrate_limit ){
		GPTMessageSend(GPT_MSG_VIDEO_SETVENCCHNBITRATE, 0, bitrate, sizeof(int));
	}
	
	//设置编码器的帧率
	if( p_video_encoder->framerate != readCameraEncoder.framerate ){
		GPTMessageSend(GPT_MSG_VIDEO_SETVENCCHNFRAMERATE, 0, framerate, sizeof(int));
	}

	//设置编码器的GOP(关键帧) GovLength
	if( p_video_encoder->video_encoding.v_encoding_profile.gov_length != readCameraEncoder.video_encoding.v_encoding_profile.gov_length ){
		GPTMessageSend(GPT_MSG_VIDEO_SETVENCCHNGOP, 0, gop, sizeof(int));
	}
	
	//设置编码器的编码级别
	if( p_video_encoder->video_encoding.v_encoding_profile.encode_profile != readCameraEncoder.video_encoding.v_encoding_profile.encode_profile){
		GPTMessageSend(GPT_MSG_VIDEO_SETVENCCHNPROFILE, 0, encode_profile, sizeof(int));
	}

	/*目前视频分辨率设置无效处理，因为设置分辨率之后影响双光融合算法*/
	if( p_video_encoder->width != readCameraEncoder.width && p_video_encoder->height != readCameraEncoder.height ){
		//GPTMessageSend(GPT_MSG_VIDEO_SETVENCCHNRESOLUTION, 0, (int)&resolution, sizeof(resolution));
		p_video_encoder->width = 1920;
		p_video_encoder->height = 1080;
	}
    
	/* 更新保存于文件 */
	if (save_cfg_to_file(VIDEO_ENCODER_FILE, (char*)p_video_encoder, sizeof(Video_Encoder)) < 0) {
		printf(" setVideoEncoder | save_cfg_to_file faile\n");
		return -1;
	}

	return 0;
}

/* 读取NTPInformation数据参数 */
#define  NTPFILE  ("/user/cfg_files/ntp.dat")
int GetNTPInformation(onvif_NTPInformation		    *pNTPInformation)
{
	if (read_cfg_from_file(NTPFILE, (char *)pNTPInformation, 
		sizeof(onvif_NTPInformation)) != 0) {
		return -1;
	}

	return 0;
}

extern int sync_time(onvif_NTPInformation		*pNTPInformation);

/* 保存NTPInformation数据参数*/
int SetNTPInformation(onvif_NTPInformation *pNTPInformation, BOOL isSave)
{
    //开启NTP服务器获取时间
	if (sync_time(pNTPInformation) > 0) {
	    UTIL_INFO("Sync time success");
	}
    else{
        UTIL_INFO("fail to sync time");
    }

	if (isSave && save_cfg_to_file(NTPFILE, (char*)pNTPInformation, 
		sizeof(onvif_NTPInformation)) != 0) {
		return -1;
	}

	return 0;

}

/* 读取 SystemDateTime数据参数 */
#define  DATATIMEFILE  ("/user/cfg_files/datatime.dat")
int GetSystemDateTime(onvif_SystemDateTime *pDataTimeInfo)
{
	if (read_cfg_from_file(DATATIMEFILE, (char *)pDataTimeInfo, 
		sizeof(onvif_SystemDateTime)) != 0) {
		UTIL_ERR("DATATIMEFILE(%s)is not exsit!!!", DATATIMEFILE);
		return -1;
	}

	return 0;
}

time_t SystemTimeToTM(onvif_DateTime stStartTime)
{
	time_t t;
    time_t time_utc;
    struct tm tm_local;
    time(&time_utc);
    localtime_r(&time_utc, &tm_local);

    tm_local.tm_year = stStartTime.Date.Year-1900;
    tm_local.tm_mon = stStartTime.Date.Month -1;
    tm_local.tm_mday = stStartTime.Date.Day;
    tm_local.tm_hour = stStartTime.Time.Hour;
    tm_local.tm_min = stStartTime.Time.Minute;
    tm_local.tm_sec = stStartTime.Time.Second;
    t =  mktime(&tm_local);
    return t;
}

int opt_UTCTimeToSystemTime(onvif_DateTime *pUTCDateTime)
{
	char zone1 = 0, zone2 = 0, zone3 = 0;

	//CST-4:30:00
	sscanf(&g_onvif_cfg.SystemDateTime.TimeZone.TZ[4], 
				"%02d:%02d:%02d", &zone1, &zone2, &zone3);//CST-
    //东时区
	if (strncmp(&g_onvif_cfg.SystemDateTime.TimeZone.TZ[3], "-", 1) == 0)
	{
	    if ((pUTCDateTime->Time.Hour + zone1) > 23) {
			pUTCDateTime->Date.Day += 1;
			pUTCDateTime->Time.Hour += zone1;
			pUTCDateTime->Time.Hour -= 24;
	    }
		else {
			pUTCDateTime->Time.Hour += zone1;
		}
		if ((pUTCDateTime->Time.Minute + zone2) >  59) {
			pUTCDateTime->Time.Hour += 1;
			pUTCDateTime->Time.Minute += zone2;
			pUTCDateTime->Time.Minute -= 59;
		}
		else {
			pUTCDateTime->Time.Minute += zone2;
		}
	}
	else
	{
	    if ((pUTCDateTime->Time.Hour - zone1) < 0) {
			pUTCDateTime->Date.Day -= 1;
			pUTCDateTime->Time.Hour += 24 - zone1;
	    }
		else {
			pUTCDateTime->Time.Hour -= zone1;
		}
		if ((pUTCDateTime->Time.Minute - zone2) <  0) {
			pUTCDateTime->Time.Hour -= 1;
			pUTCDateTime->Time.Minute += 59 - zone2;
		}
		else {
			pUTCDateTime->Time.Minute -= zone2;
		}

	}
	
	UTIL_INFO("zone1=%d,zone2=%d,zone3=%d,%s", zone1, zone2, zone3,
			&g_onvif_cfg.SystemDateTime.TimeZone.TZ[4]);

	UTIL_INFO("date -s %04d%02d%02d%02d%02d.%02d", 
		pUTCDateTime->Date.Year, pUTCDateTime->Date.Month,
		pUTCDateTime->Date.Day, pUTCDateTime->Time.Hour,
		pUTCDateTime->Time.Minute, pUTCDateTime->Time.Second);
	
	system_ex("date -s %04d%02d%02d%02d%02d.%02d", 
		pUTCDateTime->Date.Year, pUTCDateTime->Date.Month,
		pUTCDateTime->Date.Day, pUTCDateTime->Time.Hour,
		pUTCDateTime->Time.Minute, pUTCDateTime->Time.Second);
	system_ex("hwclock -w;hwclock -s");
}

struct tm * GetSystemUTCTime()
{	
	time_t nowtime;
	struct tm *gtime;

	time(&nowtime);
	gtime = gmtime(&nowtime);
	UTIL_INFO("tm_hour=%d,tm_min=%d,tm_sec=%d,tm_year=%d,tm_mon=%d,tm_mday=%d",
		gtime->tm_hour, gtime->tm_min, gtime->tm_sec, 
		gtime->tm_year+1900, gtime->tm_mon+1, gtime->tm_mday);

	return gtime;
}
void set_system_clock_timezone(onvif_DateTime *pUTCDateTime, int utc)
{	
	struct tm now;

	if (!pUTCDateTime) return ;
	
	UTIL_INFO("1 date -s %04d%02d%02d%02d%02d.%02d", 
		pUTCDateTime->Date.Year, pUTCDateTime->Date.Month,
		pUTCDateTime->Date.Day, pUTCDateTime->Time.Hour,
		pUTCDateTime->Time.Minute, pUTCDateTime->Time.Second);
	
	opt_UTCTimeToSystemTime(pUTCDateTime);
		
#if 0
	struct tm p;
	time_t timep;
	
	p.tm_sec = pUTCDateTime->Time.Second;
	p.tm_min = pUTCDateTime->Time.Minute;
	p.tm_hour = pUTCDateTime->Time.Hour;
	p.tm_year = pUTCDateTime->Date.Year-1900;
	p.tm_mon = pUTCDateTime->Date.Month-1;
	p.tm_mday = pUTCDateTime->Date.Day;
	//p.tm_isdst = 
	timep = mktime(&p);
	localtime_r(&timep, &now);
	char cmd[524] = {0};
	snprintf(cmd, sizeof(cmd), "date -s %04d%02d%02d%02d%02d.%02d", now.tm_year+1900, now.tm_mon+1,
			now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec);
	UTIL_INFO("p.tm_isdst", p.tm_isdst);
	UTIL_INFO("1.set system time with comand \"%s\" timep===%llu", cmd, timep);
	char cmd[524] = {0};

	snprintf(cmd, sizeof(cmd), "date -s %04d%02d%02d%02d%02d.%02d", 
		pUTCDateTime->Date.Year, pUTCDateTime->Date.Month,
		pUTCDateTime->Date.Day, pUTCDateTime->Time.Hour,
		pUTCDateTime->Time.Minute, pUTCDateTime->Time.Second);
#endif
}

/* 保存SystemDateTime数据参数*/
int SetSystemDateTime(onvif_SystemDateTime	 *pDataTimeInfo, 
								onvif_DateTime *pUTCDateTime, BOOL isTZChange, BOOL isSave)
{
	// check datetime
	char linuxTz[16] = {0};
	int ret = -1;
	
	UTIL_INFO("linuxTz : %s",pDataTimeInfo->TimeZone.TZ);
	if (isTZChange) {
		//东八区是GMT-8而不是GMT+8，要不就设为了西八区 
		snprintf(linuxTz, sizeof(linuxTz), (char *)"GMT%s", 
							&pDataTimeInfo->TimeZone.TZ[3]);

		ret = setenv("TZ", linuxTz, 1);
		if (ret != 0) {
			UTIL_INFO("fail to set Tz %s", linuxTz);
		}
	}
	
	if (isSave && pDataTimeInfo->DateTimeType == SetDateTimeType_Manual){
		UTIL_INFO("SetDateTimeType_Manual");
		set_system_clock_timezone(pUTCDateTime, 1);
	}
	
	if (isSave && save_cfg_to_file(DATATIMEFILE, (char*)pDataTimeInfo, 
		sizeof(onvif_SystemDateTime)) != 0) {
		return -1;
	}

	if (isTZChange) {
		UTIL_ZONE("TZChange");
	}

	return 0;
}

/*1 读取TCPIP数据参数 */
#define  NETINTEREFILE  ("/user/cfg_files/NetInter.dat")
int GetNetworkInterfaces(onvif_NetworkInterface	*pNetworkInterface)
{
	if (read_cfg_from_file(NETINTEREFILE, (char *)pNetworkInterface, 
		sizeof(onvif_NetworkInterface)) != 0) {
		UTIL_ERR("NETINTEREFILE:%s  not exsit!!!", NETINTEREFILE);
		return -1;
	}

	return 0;
}

int Opt_SetDeviceGateway(const char *gateway)
{
    int fd = -1;
    int ret = -1;
	int errno_val = 0;
    struct sockaddr_in sin;
    struct rtentry  rt;

	if (!gateway) {
		UTIL_ERR("ifname or gateway == NULL!!");
		return -1;
	}
	
	UTIL_INFO("route setgateway %s", gateway);

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        UTIL_ERR("socket error");     
        return -1;     
    }

    //设置设备网关
    memset(&rt, 0, sizeof(struct rtentry));
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = 0;
    if (inet_aton(gateway, &sin.sin_addr) < 0 ) {
		UTIL_ERR("inet_aton gateway error");
	   	ret = -1;
		goto __EXIT;
    }

    memcpy (&rt.rt_gateway, &sin, sizeof(struct sockaddr_in));
    ((struct sockaddr_in *)&rt.rt_dst)->sin_family = AF_INET;
    ((struct sockaddr_in *)&rt.rt_genmask)->sin_family = AF_INET;
    rt.rt_flags = RTF_GATEWAY;
	
    if (ioctl(fd, SIOCADDRT, &rt) < 0) {
		errno_val = errno;
		//如果网关已经存在则无需处理，视为正常
        UTIL_ERR("ioctl(SIOCADDRT) error=%d %s", errno_val, strerror(errno_val));
		if (EEXIST == errno_val) {
			ret = 0;
		}
		else {
        	ret = -1;
		}
		goto __EXIT;
    }

	ret = 0;

__EXIT:

    if (fd > 0) {
    	close(fd);
		fd = -1;
    }

	return ret;
}

int Opt_SetDeviceIpAddr(const char *ifname, const char *ipaddr, 
									const char *mask)
{
    int fd = -1;
    int ret = -1;
    struct ifreq ifr; 
    struct sockaddr_in sin;
	
	if (!ifname || !ipaddr || !mask) {
		UTIL_ERR("ifname or ipaddr or mask== NULL!!");
		return -1;
	}
	
	UTIL_INFO("ifname=%s,ipaddr=%s,netmask==%s", ifname, ipaddr, mask);

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        UTIL_ERR("socket   error");     
        return -1;     
    }
	
    memset(&ifr,0,sizeof(ifr)); 
    strcpy(ifr.ifr_name, ifname); 
	
    if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {     
        UTIL_ERR("ioctl SIOCGIFADDR error");     
        ret = -1;
		goto __EXIT;
    }
	
    //设置设备IP地址
    memset(&sin, 0, sizeof(struct sockaddr));
    sin.sin_family = AF_INET;
    if (inet_aton(ipaddr, &(sin.sin_addr)) < 0) {     
        UTIL_INFO("inet_aton error");     
        ret = -1;
		goto __EXIT;
    }    
    memcpy(&ifr.ifr_addr, &sin, sizeof(struct sockaddr));

    if (ioctl(fd, SIOCSIFADDR, &ifr) < 0) {     
        UTIL_ERR("ioctl SIOCSIFADDR error:%d,%s", errno, strerror(errno));     
        ret = -1;
		goto __EXIT;
    }
	
    //设置设备子网掩码
    if (inet_aton(mask, &(sin.sin_addr)) < 0) {     
        UTIL_ERR("inet_pton error");     
        ret = -1;
		goto __EXIT;
    } 
	
    memcpy(&ifr.ifr_addr, &sin, sizeof(struct sockaddr));
    if (ioctl(fd, SIOCSIFNETMASK, &ifr) < 0) {
        UTIL_ERR("ioctl SIOCSIFNETMASK :%d,%s", errno, strerror(errno));
        ret = -1;
		goto __EXIT;
    }
	
	ret = 0;
__EXIT:

	if (fd > 0) {
		close(fd);
		fd = -1;
	}

	return ret;
}

int opt_SetNetworkInterfaces(onvif_NetworkInterface	*pNetworkInterface)
{
    int ret = -1; 
	if (pNetworkInterface->IPv4Flag && pNetworkInterface->IPv4.Enabled) {
		//自定义设置ip地址与子网掩码
		if (pNetworkInterface->IPv4.Config.DHCP == FALSE) {
			ret = Opt_SetDeviceIpAddr("eth0", pNetworkInterface->IPv4.Config.Address,
						get_mask_by_prefix_len(pNetworkInterface->IPv4.Config.PrefixLength));
			if (ret < 0) {
				UTIL_ERR("Opt_SetDeviceIpAddr failed!!!");
				return -1;
			}
			
			UTIL_INFO("Opt_SetDeviceIpAddr success token=%s!!!", pNetworkInterface->token);
	    }
		else {//dhcp获取ip地址与子网掩码
			system_ex("killall udhcpc; udhcpc -i eth0");
			UTIL_INFO("killall udhcpc; udhcpc -i eth0");
		}
    }

	return 0;
}

/* 保存TCPIP数据参数*/
int SetNetworkInterfaces(onvif_NetworkInterface	*pNetworkInterface, BOOL isSave)
{
	opt_SetNetworkInterfaces(pNetworkInterface);
	
	if (isSave && save_cfg_to_file(NETINTEREFILE, (char*)pNetworkInterface, 
		sizeof(onvif_NetworkInterface)) != 0) {
		return -1;
	}

	return 0;
}

/*2 读取网关数据参数 */
#define  GATEWAYFILE  ("/user/cfg_files/gateway.dat")
int GetNetworkGateway(onvif_NetworkGateway		      *pNetworkGateway)
{
	if (read_cfg_from_file(GATEWAYFILE, (char *)pNetworkGateway, 
		sizeof(onvif_NetworkGateway)) != 0) {
		UTIL_ERR("GATEWAYFILE:%s  not exsit!!!", GATEWAYFILE);
		return -1;
	}
	return 0;
}
	
/*设置网关数据参数*/
int SetNetworkGateway(onvif_NetworkGateway		     *pNetworkGateway, BOOL isSave)
{
	ONVIF_NetworkInterface * p_net_inf = g_onvif_cfg.network.interfaces;
	int ret = -1;
	//网关设置只在手动IP地址的时候起作用
    if (FALSE == p_net_inf->NetworkInterface.IPv4.Config.DHCP) {
		ret = Opt_SetDeviceGateway(pNetworkGateway->IPv4Address[0]);
		if (ret < 0) {
			UTIL_ERR("Opt_SetDeviceGateway failed!!!");
			return -1;
		}
		
		UTIL_INFO("Opt_SetDeviceGateway=%s success!!!", pNetworkGateway->IPv4Address[0]);
    }
		
	if (isSave && save_cfg_to_file(GATEWAYFILE, (char*)pNetworkGateway, 
		sizeof(onvif_NetworkGateway)) != 0) {
		return -1;
	}

	return 0;
}

/*3 读取DNS数据参数 */
#define  DNSFILE  ("/user/cfg_files/DNS.dat")
int GetDNSInformation(onvif_DNSInformation		     *pDNSInformation)
{
	if (read_cfg_from_file(DNSFILE, (char *)pDNSInformation, 
			sizeof(onvif_DNSInformation)) != 0) {
		UTIL_ERR("DNSFILE:%s  not exsit!!!", DNSFILE);
		return -1;
	}
			
	return 0;
}
	
/* 设置DNS数据参数*/
int SetDNSInformation(onvif_DNSInformation		     *pDNSInformation, BOOL isSave)
{
	ONVIF_NetworkInterface * p_net_inf = g_onvif_cfg.network.interfaces;
	//DNS设置只在手动IP地址的时候起作用
    if (FALSE == p_net_inf->NetworkInterface.IPv4.Config.DHCP) {
	    if (FALSE == pDNSInformation->FromDHCP) {
			if (strlen(pDNSInformation->DNSServer[0]) > 0) {
				system_ex("echo \"nameserver %s\" > /etc/resolv.conf", 
							pDNSInformation->DNSServer[0]);
			}
			if (strlen(pDNSInformation->DNSServer[1]) > 0) {
				system_ex("echo \"nameserver %s\" >> /etc/resolv.conf", 
							pDNSInformation->DNSServer[1]);
			}
    	}
    }

	if (isSave && save_cfg_to_file(DNSFILE, (char*)pDNSInformation, 
		    sizeof(onvif_DNSInformation)) != 0) {
		return -1;
	}

	return 0;
}

/*4 读取网络协议数据参数 */
#define  NETPROFILE  ("/user/cfg_files/netpro.dat")
int GetNetworkProtocols(onvif_NetworkProtocol	*pNetworkProtocol)
{
	if (read_cfg_from_file(NETPROFILE, (char *)pNetworkProtocol, 
			sizeof(onvif_NetworkProtocol)) != 0) {
		UTIL_ERR("NETPROFILE:%s  not exsit!!!", NETPROFILE);
		return -1;
	}

	return 0;
}
	
/* 设置网络协议数据参数*/
int SetNetworkProtocols(onvif_NetworkProtocol	*pNetworkProtocol, BOOL isSave)
{
	if (isSave && save_cfg_to_file(NETPROFILE, (char*)pNetworkProtocol, 
			sizeof(onvif_NetworkProtocol)) != 0) {
		return -1;
	}

	return 0;
}

/*3 读取事件上传数据参数 */
#define  EVENTSNAPFILE  ("/user/cfg_files/EventSnap.dat")
int GetEventSnapInformation(onvif_EventSnapUploadInfo	       *pEventSnap)
{
	if (read_cfg_from_file(EVENTSNAPFILE, (char *)pEventSnap, 
			sizeof(onvif_EventSnapUploadInfo)) != 0) {
		UTIL_ERR("EVENTSNAPFILE:%s  not exsit!!!", EVENTSNAPFILE);
		return -1;
	}
			
	return 0;
}
	
/* 设置事件上传数据参数*/
int SetEventSnapInformation(onvif_EventSnapUploadInfo	       *pEventSnap, BOOL isSave)
{
	if (isSave && save_cfg_to_file(EVENTSNAPFILE, (char*)pEventSnap, 
		    sizeof(onvif_EventSnapUploadInfo)) != 0) {
		return -1;
	}

	return 0;
}

void SystemReboot()
{
	system_ex("reboot");
}

void SetSystemFactoryDefault(int type /* 0:soft, 1:hard */)
{
	if (0 == access("/user/cfg_files", F_OK)) {
		//system_ex("rm /userdata/cfg_files/* -rf");
		UTIL_INFO("SetSystemFactoryDefault!!");
	}
}

static void CloseAllFd(void)
{
	DIR *dir;
	char szPath[32];
	struct dirent *entry;
	int fd, fd_max=0;
	int i;

	sprintf(szPath, "/proc/%d/fd", getpid());

	dir = opendir(szPath);
	if(NULL == dir)
		return;

	while((entry = readdir(dir))) {
		if(entry->d_name[0] == '.')
			continue;
		fd = atoi(entry->d_name);

		if(fd > fd_max)
			fd_max = fd;
	}

	closedir(dir);

	for( i=3; i<fd_max+1; i++)
		close(i);
}

FILE *vpopen(const char* cmdstring, const char *type)  
{  
	int pfd[2];  
	FILE *fp;  
	pid_t  pid;  
	int i = 0;

	if((type[0]!='r' && type[0]!='w')||type[1]!=0)	
	{  
		errno = EINVAL;  
		return(NULL);  
	}  
  

	if(pipe(pfd)!=0)  
	{  
		return NULL;  
	}  

	if((pid = vfork())<0)  
	{  
		return(NULL);	/* errno set by fork() */	 
	}  
	else if (pid == 0) {	/* child */  
		if (*type == 'r')  
		{  
			close(pfd[0]);	  
			if (pfd[1] != STDOUT_FILENO) {	  
				dup2(pfd[1], STDOUT_FILENO);	
				close(pfd[1]);	  
			}			  
		}  
		else  
		{  
			close(pfd[1]);	  
			if (pfd[0] != STDIN_FILENO) {	 
				dup2(pfd[0], STDIN_FILENO);    
				close(pfd[0]);	  
			}			  
		}  

		CloseAllFd();	 

		execl("/bin/sh", "sh", "-c", cmdstring, (char *) 0);	
		_exit(127); 	  
	}  

	if (*type == 'r') {    
		close(pfd[1]);	  
		if ( (fp = fdopen(pfd[0], type)) == NULL)	 
			return(NULL);	 
	} else {	
		close(pfd[0]);	  
		if ( (fp = fdopen(pfd[1], type)) == NULL)	 
			return(NULL);	 
	}  
 
	return(fp); 	  
}  

int vpclose(FILE *fp)  
{  
	int 	stat;	 
	pid_t	pid;	
	   
	if (fclose(fp) == EOF)	  
		return(-1);    

	while (waitpid(pid, &stat, 0) < 0)	  
		if (errno != EINTR)    
			return(-1); /* error other than EINTR from waitpid() */    

	return(stat);	/* return child's termination status */    
}  

/* Copied from linux/rtc.h to eliminate the kernel dependency */  
struct linux_rtc_time {  
    int tm_sec;  
    int tm_min;  
    int tm_hour;  
    int tm_mday;  
    int tm_mon;  
    int tm_year;  
    int tm_wday;  
    int tm_yday;  
    int tm_isdst;  
};  
  
#define RTC_SET_TIME   _IOW('p', 0x0a, struct linux_rtc_time) /* Set RTC time    */  
#define RTC_RD_TIME    _IOR('p', 0x09, struct linux_rtc_time) /* Read RTC time   */  

time_t read_rtc(int utc)  
{  
    int rtc;  
    struct tm tm;  
    char *oldtz = 0;  
    time_t t = 0;  
  
    if (( rtc = open ( "/dev/rtc0", O_RDONLY )) < 0 ) {  
        if (( rtc = open ( "/dev/misc/rtc", O_RDONLY )) < 0 ) {  
            UTIL_ERR("Could not access RTC");  
			return -1;
        }  
    }  
          
    memset (&tm, 0, sizeof( struct tm ));  
    if (ioctl ( rtc, RTC_RD_TIME, &tm ) < 0) { 
        UTIL_ERR("Could not read time from RTC" );
		return -1;
    }
	
    tm. tm_isdst = -1; // not known  
      
    close ( rtc );  
    t = mktime ( &tm ); 
	
    return t;  
}  

int sync_hwclock_tosys()  
{  
    int rtc;  
    struct tm tm;  
    char *oldtz = 0;  
    time_t t = 0;  
  
    if (( rtc = open ( "/dev/rtc0", O_RDONLY )) < 0 ) {  
        if (( rtc = open ( "/dev/misc/rtc", O_RDONLY )) < 0 ) {  
            UTIL_ERR("Could not access RTC");  
			return -1;
        }  
    }  
          
    memset (&tm, 0, sizeof( struct tm ));  
    if (ioctl ( rtc, RTC_RD_TIME, &tm ) < 0) { 
        UTIL_ERR("Could not read time from RTC");
	    close(rtc);  
		return -1;
    }
	
    tm. tm_isdst = -1; // not known  
      
    close(rtc);  
  
	system_ex("date -s %04d%02d%02d%02d%02d.%02d", tm.tm_year+1900, tm.tm_mon+1,
			tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	UTIL_INFO("date -s %04d%02d%02d%02d%02d.%02d", tm.tm_year+1900, tm.tm_mon+1,
			tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    return 0;  
}  

int IRParamGetHanldeMsg(int msgtype, int chan,
	                                   int u32DataParam, int u32DataLen, 
	                                   void * parg)
{
    int ret = 0;
	pthread_mutex_lock(&m_get_ir_param_lock);

	switch(msgtype) {
		case GPT_MSG_IR_GETBASEPARAM:
		{
			if(sizeof(ThermalBaseParam) != u32DataLen){
				pthread_mutex_unlock(&m_get_ir_param_lock);
				return -1;
			}
			getThermalBaseParam((ThermalBaseParam *)u32DataParam);
		}
		break;

		case GPT_MSG_IR_GETENVPARAM:
		{
			if(sizeof(ThermalEnvParam) != u32DataLen){
				pthread_mutex_unlock(&m_get_ir_param_lock);
				return -1;
			}
			getThermalEnvParam((ThermalEnvParam *)u32DataParam);
		}
		break;
		default:
			break;
	}
	pthread_mutex_unlock(&m_get_ir_param_lock);
	return ret;

}

int FusionParamGetHanldeMsg(int msgtype, int chan, int u32DataParam, int u32DataLen, void * parg)
{
   int ret = 0;
   pthread_mutex_lock(&m_fusion_param_lock);

   switch(msgtype) {
	   case GPT_MSG_DULA_GETFUSIONPARAM:
	   {
		   if(sizeof(DulaInformation_t) != u32DataLen){
			   pthread_mutex_unlock(&m_fusion_param_lock);
			   return -1;
		   }
		   getFusionParam((DulaInformation_t *)u32DataParam);
	   }
	   break;
	   default:
		   break;
   }
   pthread_mutex_unlock(&m_fusion_param_lock);
   return ret;

}

int onvif_ir_message_register()
{
   GPTMessageRegister(GPT_MSG_IR_GETBASEPARAM, IRParamGetHanldeMsg, 0, NULL);
   GPTMessageRegister(GPT_MSG_IR_GETENVPARAM, IRParamGetHanldeMsg, 0, NULL);
   return 0;

}

int onvif_fusion_message_register()
{
   GPTMessageRegister(GPT_MSG_DULA_GETFUSIONPARAM, FusionParamGetHanldeMsg, 0, NULL);
   return 0;

}

int onvif_message_init()
{
	onvif_ir_message_register();
	onvif_fusion_message_register();

	return 0;
}
