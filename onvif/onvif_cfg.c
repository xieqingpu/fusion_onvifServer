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
#include "onvif.h"
#include "onvif_cm.h"
#include "onvif_cfg.h"
#include "xml_node.h"
#include "onvif_utils.h"

#include "set_config.h"		// add by xieqingpu


/***************************************************************************************/
extern ONVIF_CFG g_onvif_cfg;
extern ONVIF_CLS g_onvif_cls;


/***************************************************************************************/
void onvif_parse_information_cfg(XMLN * p_node)
{
	XMLN * p_Manufacturer;
	XMLN * p_Model;
	XMLN * p_FirmwareVersion;
	XMLN * p_SerialNumber;
	XMLN * p_HardwareId;

	p_Manufacturer = xml_node_get(p_node, "Manufacturer");
	if (p_Manufacturer && p_Manufacturer->data)
	{
		strncpy(g_onvif_cfg.DeviceInformation.Manufacturer, p_Manufacturer->data, sizeof(g_onvif_cfg.DeviceInformation.Manufacturer)-1);
	}

	p_Model = xml_node_get(p_node, "Model");
	if (p_Model && p_Model->data)
	{
		strncpy(g_onvif_cfg.DeviceInformation.Model, p_Model->data, sizeof(g_onvif_cfg.DeviceInformation.Model)-1);
	}
	
	p_FirmwareVersion = xml_node_get(p_node, "FirmwareVersion");
	if (p_FirmwareVersion && p_FirmwareVersion->data)
	{
		strncpy(g_onvif_cfg.DeviceInformation.FirmwareVersion, p_FirmwareVersion->data, sizeof(g_onvif_cfg.DeviceInformation.FirmwareVersion)-1);
	}

	p_SerialNumber = xml_node_get(p_node, "SerialNumber");
	if (p_SerialNumber && p_SerialNumber->data)
	{
		strncpy(g_onvif_cfg.DeviceInformation.SerialNumber, p_SerialNumber->data, sizeof(g_onvif_cfg.DeviceInformation.SerialNumber)-1);
	}

	p_HardwareId = xml_node_get(p_node, "HardwareId");
	if (p_HardwareId && p_HardwareId->data)
	{
		strncpy(g_onvif_cfg.DeviceInformation.HardwareId, p_HardwareId->data, sizeof(g_onvif_cfg.DeviceInformation.HardwareId));
	}
}

////// add by xieqingpu
void onvif_set_devinfo(CONFIG_Information * p_devInfo)
{
	if (p_devInfo->manufacturer[0] != '\0'){
		strncpy(g_onvif_cfg.DeviceInformation.Manufacturer, p_devInfo->manufacturer, sizeof(g_onvif_cfg.DeviceInformation.Manufacturer)-1);
	}

	if (p_devInfo->model[0] != '\0'){
		strncpy(g_onvif_cfg.DeviceInformation.Model, p_devInfo->model, sizeof(g_onvif_cfg.DeviceInformation.Model)-1);
	}

	if (p_devInfo->firmware_version[0] != '\0'){
		strncpy(g_onvif_cfg.DeviceInformation.FirmwareVersion, p_devInfo->firmware_version, sizeof(g_onvif_cfg.DeviceInformation.FirmwareVersion)-1);
	}

	if (p_devInfo->serial_number[0] != '\0'){
		strncpy(g_onvif_cfg.DeviceInformation.SerialNumber, p_devInfo->serial_number, sizeof(g_onvif_cfg.DeviceInformation.SerialNumber)-1);
	}

	if (p_devInfo->hardware_id[0] != '\0'){
		strncpy(g_onvif_cfg.DeviceInformation.HardwareId ,p_devInfo->hardware_id, sizeof(g_onvif_cfg.DeviceInformation.HardwareId)-1);
	}
}

/////

BOOL onvif_parse_user(XMLN * p_node, onvif_User * p_user)
{
	XMLN * p_username;
    XMLN * p_password;
    XMLN * p_userlevel;
    
	p_username = xml_node_get(p_node, "username");
	if (p_username && p_username->data)
	{
		strncpy(p_user->Username, p_username->data, sizeof(p_user->Username)-1);
	}
	else
	{
		return FALSE;
	}

	p_password = xml_node_get(p_node, "password");
	if (p_password && p_password->data)
	{
		strncpy(p_user->Password, p_password->data, sizeof(p_user->Password)-1);
	}

	p_userlevel = xml_node_get(p_node, "userlevel");
	if (p_userlevel && p_userlevel->data)
	{
		p_user->UserLevel = onvif_StringToUserLevel(p_userlevel->data);
	}

	return TRUE;
}

void onvif_parse_h264_options(Encoding_profile * p_encoding_profile, ONVIF_VideoEncoder2Configuration * p_v_enc_cfg)
{		
	if(p_encoding_profile->gov_length){
		p_v_enc_cfg->Configuration.GovLengthFlag = 1;
		p_v_enc_cfg->Configuration.GovLength = p_encoding_profile->gov_length;  //关键帧
	}

	if (p_encoding_profile->encode_profile >= 0){
		p_v_enc_cfg->Configuration.ProfileFlag = 1;						//编码级别
		strncpy(p_v_enc_cfg->Configuration.Profile, onvif_H264ProfileToString(p_encoding_profile->encode_profile), sizeof(p_v_enc_cfg->Configuration.Profile)-1);
	}
}

#ifdef MEDIA2_SUPPORT
void onvif_parse_h265_options(Encoding_profile * p_encoding_profile, ONVIF_VideoEncoder2Configuration * p_v_enc_cfg)
{	
	if(p_encoding_profile->gov_length){
		p_v_enc_cfg->Configuration.GovLengthFlag = 1;
		p_v_enc_cfg->Configuration.GovLength = p_encoding_profile->gov_length;  //关键帧
	}

	if (p_encoding_profile->encode_profile >= 0){
		p_v_enc_cfg->Configuration.ProfileFlag = 1;						//编码级别
		strncpy(p_v_enc_cfg->Configuration.Profile, onvif_H264ProfileToString(p_encoding_profile->encode_profile), sizeof(p_v_enc_cfg->Configuration.Profile)-1);
	}
}
#endif

void onvif_parse_mpeg4_options(Encoding_profile * p_encoding_profile, ONVIF_VideoEncoder2Configuration * p_v_enc_cfg)
{	
	if(p_encoding_profile->gov_length){
		p_v_enc_cfg->Configuration.GovLengthFlag = 1;
		p_v_enc_cfg->Configuration.GovLength = p_encoding_profile->gov_length;  //关键帧
	}

	if (p_encoding_profile->encode_profile >= 0){
		p_v_enc_cfg->Configuration.ProfileFlag = 1;						//编码级别
		strncpy(p_v_enc_cfg->Configuration.Profile, onvif_Mpeg4ProfileToString(p_encoding_profile->encode_profile), sizeof(p_v_enc_cfg->Configuration.Profile)-1);
	}
}

ONVIF_VideoSourceConfiguration * onvif_parse_video_source_cfg()
{
	int w = 0, h = 0;
	ONVIF_VideoSourceConfiguration * p_v_src_cfg = NULL;
	
	onvif_VideoResolution videoSource;
	videoSource.Width = 1920;
	videoSource.Height = 1080;

	w = videoSource.Width;
	h = videoSource.Height;

	if (w == 0 || h == 0)
	{
		return NULL;
	}
	
	p_v_src_cfg = onvif_find_VideoSourceConfiguration_by_size(w, h);
	if (p_v_src_cfg)
	{
		return p_v_src_cfg;
	}

	p_v_src_cfg = onvif_add_VideoSourceConfiguration(w, h);

	return p_v_src_cfg;
}

ONVIF_VideoEncoder2Configuration * onvif_parse_video_encoder_cfg()
{
    ONVIF_VideoEncoder2Configuration * p_v_enc_cfg;
	ONVIF_VideoEncoder2Configuration v_enc_cfg;
	
	memset(&v_enc_cfg, 0, sizeof(v_enc_cfg));

	Video_Encoder encoder;
	memset(&encoder, 0, sizeof(encoder));

	if( getVideoEncoder(&encoder) !=0 )		//读取 视频编码器参数
		printf("get Video Encoder para faile.\n");

	//判断读出的分辨率是否符合
	if ( encoder.width != 1920 && encoder.height != 1080 ||
		 encoder.width != 1280 && encoder.height != 720 ||
		 encoder.width != 720 && encoder.height != 576 ||
		 encoder.width != 640 && encoder.height != 360 ||
		 encoder.width != 352 && encoder.height != 288 )	
	{
		encoder.width = 1920;
		encoder.height = 1080;
	}
	// printf("xxx getVideoEncode | 分辨率encoder.width = %d,encoder.height = %d xxx\n",encoder.width, encoder.height);//分辨率
	v_enc_cfg.Configuration.Resolution.Width = encoder.width;	//分辨率 宽
	v_enc_cfg.Configuration.Resolution.Height = encoder.height;	 //分辨率 高

	//判断读出的编码器的码流(码率)
	if (encoder.bitrate_limit < 1 || encoder.bitrate_limit > 4096)
	{
		encoder.bitrate_limit = 2048;
	}
	v_enc_cfg.Configuration.RateControl.BitrateLimit = encoder.bitrate_limit; //码率

	//判断读出的编码器的帧率
	if (encoder.framerate < 1 || encoder.framerate > 30 )
	{
		encoder.framerate = 25;
	}
	v_enc_cfg.Configuration.RateControl.FrameRateLimit = encoder.framerate; //帧率


	//判断读出的编码器的GOP(关键帧)
	if (encoder.video_encoding.v_encoding_profile.gov_length < 10 ||
	    encoder.video_encoding.v_encoding_profile.gov_length > 60 )
	{
		encoder.video_encoding.v_encoding_profile.gov_length = 50;
	}
	// v_enc_cfg.Configuration.GovLength = encoder.video_encoding.v_encoding_profile.gov_length;  //关键帧
		
	//判断读出的编码器的编码级别
	if (encoder.video_encoding.v_encoding_profile.encode_profile != 0 ||
		encoder.video_encoding.v_encoding_profile.encode_profile != 1 )
	{
		encoder.video_encoding.v_encoding_profile.encode_profile = 1;
	}


	if (encoder.quality){
		v_enc_cfg.Configuration.Quality = (float)(encoder.quality);
	}		

	if (encoder.session_timeout){
		v_enc_cfg.Configuration.SessionTimeout = encoder.session_timeout; 
	}

	v_enc_cfg.Configuration.RateControlFlag = 1;  ////
	

	if (encoder.encoding_interval){
		v_enc_cfg.Configuration.RateControl.EncodingInterval = encoder.encoding_interval;
	}

	//判断读出的编码器的类型JPEG=0,MPEG4=1，H264=2，H265=3...
	/* if (encoder.video_encoding.v_encoding != 0 ||
		encoder.video_encoding.v_encoding != 1 ||
		encoder.video_encoding.v_encoding != 2 ) */
	if ( encoder.video_encoding.v_encoding != 2 )  //因为现在只有一个H264编码器,所以只判断是否为H264
	{
		encoder.video_encoding.v_encoding = 2;
	}
	// printf("xxx getVideoEncode | encoder.video_encoding.v_encoding = %d (H264=2)xxx\n",encoder.video_encoding.v_encoding );
	if (encoder.video_encoding.v_encoding > -1 && encoder.video_encoding.v_encoding < 4 )
	{
		// strncpy(v_enc_cfg.Configuration.Encoding, encoder.video_encoding.v_encoding, sizeof(v_enc_cfg.Configuration.Encoding)-1);
		strncpy(v_enc_cfg.Configuration.Encoding, onvif_VideoEncodingToString(encoder.video_encoding.v_encoding), sizeof(v_enc_cfg.Configuration.Encoding)-1);
		// v_enc_cfg.Configuration.VideoEncoding = onvif_StringToVideoEncoding(encoder.video_encoding.v_encoding);
		v_enc_cfg.Configuration.VideoEncoding = encoder.video_encoding.v_encoding;
	}
	if (strcasecmp(v_enc_cfg.Configuration.Encoding, "MPEG4") == 0 || 
	    strcasecmp(v_enc_cfg.Configuration.Encoding, "MPV4-ES") == 0)
	{
	    strcpy(v_enc_cfg.Configuration.Encoding, "MPV4-ES");
	    v_enc_cfg.Configuration.VideoEncoding = VideoEncoding_MPEG4;
	    
		onvif_parse_mpeg4_options(&encoder.video_encoding.v_encoding_profile, &v_enc_cfg); //修改的
	}
	else if (strcasecmp(v_enc_cfg.Configuration.Encoding, "H264") == 0)
	{
		onvif_parse_h264_options(&encoder.video_encoding.v_encoding_profile, &v_enc_cfg);  //修改的
	}
#ifdef MEDIA2_SUPPORT	
	else if (strcasecmp(v_enc_cfg.Configuration.Encoding, "H265") == 0)
	{
		onvif_parse_h265_options(&encoder.video_encoding.v_encoding_profile, &v_enc_cfg);  //修改的
	}	
#endif
    else if (strcasecmp(v_enc_cfg.Configuration.Encoding, "JPEG"))
	{
	    return NULL;   
	}

	p_v_enc_cfg = onvif_find_VideoEncoderConfiguration_by_param(&v_enc_cfg);
	if (p_v_enc_cfg)
	{
		return p_v_enc_cfg;
	}

	p_v_enc_cfg = onvif_add_VideoEncoderConfiguration(&v_enc_cfg);

	return p_v_enc_cfg;
}

#ifdef AUDIO_SUPPORT

ONVIF_AudioSourceConfiguration * onvif_parse_audio_source_cfg()
{		
	ONVIF_AudioSourceConfiguration * p_a_src_cfg = g_onvif_cfg.a_src_cfg;
	if (p_a_src_cfg)
	{
		return p_a_src_cfg;
	}

	p_a_src_cfg = onvif_add_AudioSourceConfiguration();

	return p_a_src_cfg;
}

ONVIF_AudioEncoder2Configuration * onvif_parse_audio_encoder_cfg_t()
{
    ONVIF_AudioEncoder2Configuration * p_a_enc_cfg;    
	ONVIF_AudioEncoder2Configuration a_enc_cfg;
	
	memset(&a_enc_cfg, 0, sizeof(ONVIF_AudioEncoder2Configuration));

	Audio_Encoder audio_encoder;
	memset(&audio_encoder, 0, sizeof(audio_encoder));
	if( getAudioEncoder(&audio_encoder) !=0 )	  //读取音频编码器参数
		printf("get Camera Encoder para faile.\n");

	if (audio_encoder.session_timeout){
		a_enc_cfg.Configuration.SessionTimeout = audio_encoder.session_timeout;
	}

	if (audio_encoder.sample_rate){
		a_enc_cfg.Configuration.SampleRate = audio_encoder.sample_rate;
	}

	if (audio_encoder.bitrate){
		a_enc_cfg.Configuration.Bitrate = audio_encoder.bitrate;
	}	

	if (audio_encoder.a_encoding[0] != '\0')
	{
		strncpy(a_enc_cfg.Configuration.Encoding, audio_encoder.a_encoding, sizeof(a_enc_cfg.Configuration.Encoding)-1);
		a_enc_cfg.Configuration.AudioEncoding = onvif_StringToAudioEncoding(audio_encoder.a_encoding);
	}

#ifdef MEDIA2_SUPPORT
    if (strcasecmp(a_enc_cfg.Configuration.Encoding, "PCMU") == 0 || 
	    strcasecmp(a_enc_cfg.Configuration.Encoding, "G711") == 0)
	{
	    strcpy(a_enc_cfg.Configuration.Encoding, "PCMU");
	    a_enc_cfg.Configuration.AudioEncoding = AudioEncoding_G711;
	}
	else if (strcasecmp(a_enc_cfg.Configuration.Encoding, "AAC") == 0 || 
	    strcasecmp(a_enc_cfg.Configuration.Encoding, "MP4A-LATM") == 0)
	{
	    strcpy(a_enc_cfg.Configuration.Encoding, "MP4A-LATM");
	    a_enc_cfg.Configuration.AudioEncoding = AudioEncoding_AAC;
	}
#endif

	p_a_enc_cfg = onvif_find_AudioEncoderConfiguration_by_param(&a_enc_cfg);
	if (p_a_enc_cfg)
	{
		return p_a_enc_cfg;
	}

	p_a_enc_cfg = onvif_add_AudioEncoderConfiguration(&a_enc_cfg);

	return p_a_enc_cfg;
}

#endif // end of AUDIO_SUPPORT

void onvif_parse_profile()
{
	ONVIF_PROFILE * profile;

	profile = onvif_add_profile(TRUE);	//g_onvif_cfg.profiles 结构为链表类型不是数组
	if (NULL == profile)
	{
		return;
	}
	
	profile->v_src_cfg = onvif_parse_video_source_cfg();	//g_onvif_cfg.v_src_cfg
	profile->v_src_cfg->Configuration.UseCount++;

	profile->v_enc_cfg = onvif_parse_video_encoder_cfg();   //g_onvif_cfg.v_enc_cfg 
	if (profile->v_enc_cfg)
	{
		profile->v_enc_cfg->Configuration.UseCount++;
	}

#ifdef AUDIO_SUPPORT
	profile->a_src_cfg = onvif_parse_audio_source_cfg();
	profile->a_src_cfg->Configuration.UseCount++;

	profile->a_enc_cfg = onvif_parse_audio_encoder_cfg_t();
	profile->a_enc_cfg->Configuration.UseCount++;
#endif
}
void onvif_parse_event_cfg()
{
	g_onvif_cfg.evt_renew_time = 60;

	g_onvif_cfg.evt_sim_flag = 1;

	g_onvif_cfg.evt_sim_interval = 10;
}

BOOL onvif_parse_server(ONVIF_SRV * p_req)
{

	p_req->serv_port = 8000;
	if (p_req->serv_port < 0 || p_req->serv_port > 65535)
		return FALSE;

	return TRUE;
}

void onvif_load_cfg()
{
	/* 服务端口 */
    if (onvif_parse_server(&g_onvif_cfg.servs[0]))
    {
        g_onvif_cfg.servs_num++;
    }
	g_onvif_cfg.http_max_users = 16;

#ifdef HTTPS
	g_onvif_cfg.https_enable = 0;   // 0:diable 1:enable
#endif
	g_onvif_cfg.need_auth = 1;    // 0:不鉴权 1:鉴权
	
	/* 设备信息 */
	int ret;
	CONFIG_Information devInfo;
	memset(&devInfo, 0, sizeof(CONFIG_Information));

    ret = onvif_get_devinfo(&devInfo);
	if ( ret == 0)
		onvif_set_devinfo(&devInfo);


	/* 用户 */
	add_to_Gusers();  //从文件读取用户复制给全区变量g_onvif_cfg.users，给soap_GetUsers显示在前端
	
	/* 音频视频编码器参数 */
    onvif_parse_profile();  

	/* scope */
	int i, scope_item_len;
	onvif_Scope scope[MAX_SCOPE_NUMS];
	scope_item_len = ARRAY_SIZE(scope[0].ScopeItem);

	strncpy(scope[0].ScopeItem, "onvif://www.onvif.org/Profile/Streaming" ,scope_item_len-1);
	strncpy(scope[1].ScopeItem, "onvif://www.onvif.org/location/country/china" ,scope_item_len-1);
	strncpy(scope[2].ScopeItem, "onvif://www.onvif.org/type/video_encoder" ,scope_item_len-1);
	strncpy(scope[3].ScopeItem, "onvif://www.onvif.org/name/IP-Camera" ,scope_item_len-1);
	strncpy(scope[4].ScopeItem, "onvif://www.onvif.org/hardware/HI3518C" ,scope_item_len-1);
	// for (i = 0; i < ARRAY_SIZE(g_onvif_cfg.scopes); i++)
	for (i = 0; i < 5; i++) {
		// printf("xxx scope[%d].scope_item = %s xxx\n",i, scope[i].ScopeItem);
		if (scope[i].ScopeItem[0] != '\0') {
			onvif_add_scope(scope[i].ScopeItem,TRUE);
		}
		else {
			break;
		}
	}

	onvif_parse_event_cfg();
}


BOOL onvif_read_device_uuid(char * buff, int bufflen)
{
    int len, rlen;
    FILE * fp;
    
	fp = fopen("uuid.txt", "r");
	if (NULL == fp)
	{
		return FALSE;
	}

	fseek(fp, 0, SEEK_END);

	// get file length
	len = ftell(fp);
	if (len <= 0)
	{
		fclose(fp);
		return FALSE;
	}
	fseek(fp, 0, SEEK_SET);

    if (bufflen <= len)
    {
        printf("filelen = %d, bufflen = %d\r\n", len, bufflen);
    }
    else
    {
    	rlen = fread(buff, 1, len, fp);

        // remove \r\n
    	while (rlen > 0 && (buff[rlen-1] == '\r' || buff[rlen-1] == '\n'))
    	{
    	    rlen--;
    	    buff[rlen] = '\0';
    	}
	}
	
	fclose(fp);

	return TRUE;
}

BOOL onvif_save_device_uuid(char * buff)
{
    FILE * fp;
	
	fp = fopen("uuid.txt", "w");
	if (NULL == fp)
	{
	    printf("open file uuid.txt failed\r\n");
		return FALSE;
	}

	fwrite(buff, 1, strlen(buff), fp);

	fclose(fp);

	return TRUE;
}

#ifdef PROFILE_Q_SUPPORT

int onvif_read_device_state()
{
    int state = 0, rlen;
    char buff[8] = {'\0'};
    FILE * fp;
    
	fp = fopen("devst.txt", "r");
	if (NULL == fp)
	{
		return state;
	}
	
	rlen = fread(buff, 1, 1, fp);
	if (1 == rlen)
	{
	    state = atoi(buff);
	}
	
	fclose(fp);

	return state;
}

BOOL onvif_save_device_state(int state)
{
    char buff[8] = {'\0'};
    FILE * fp;
	
	fp = fopen("devst.txt", "w");
	if (NULL == fp)
	{
	    printf("open file devst.txt failed\r\n");
		return FALSE;
	}

    sprintf(buff, "%d", state);
	fwrite(buff, 1, strlen(buff), fp);

	fclose(fp);

	return TRUE;
}

#endif // end of PROFILE_Q_SUPPORT


