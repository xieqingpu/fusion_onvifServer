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
#include "onvif_ptz.h"
#include "onvif_utils.h"

#ifdef MEDIA2_SUPPORT
#include "onvif_media2.h"
#endif

// #include "ptz.h"   ///
#include "set_config.h"
#include "visca_api.h"
#include  <math.h>
#include <pthread.h>
#include "utils_log.h"

#ifdef PTZ_SUPPORT


extern PTZ_PresetsTours_t * onvif_find_PresetTour(const char  * preset_token);
extern PTZ_PresetsTours_t  PTZPresetsTour[MAX_PRESETS_TOUR];

// #define PTOURS_TIME_NUMBER    //add xie

/***************************************************************************************/
extern ONVIF_CLS g_onvif_cls;
extern ONVIF_CFG g_onvif_cfg;



/***************************************************************************************/
ONVIF_RET onvif_ptz_GetStatus(ONVIF_PROFILE * p_profile, onvif_PTZStatus * p_ptz_status)
{
	// todo : add get ptz status code ...
	
	p_ptz_status->PositionFlag = 1;
	p_ptz_status->Position.PanTiltFlag = 1;
	p_ptz_status->Position.PanTilt.x = 0;
	p_ptz_status->Position.PanTilt.y = 0;
	p_ptz_status->Position.ZoomFlag = 1;
	p_ptz_status->Position.Zoom.x = 0;
	
	p_ptz_status->MoveStatusFlag = 1;
	p_ptz_status->MoveStatus.PanTiltFlag = 1;
	p_ptz_status->MoveStatus.PanTilt = MoveStatus_IDLE;
	p_ptz_status->MoveStatus.ZoomFlag = 1;
	p_ptz_status->MoveStatus.Zoom = MoveStatus_IDLE;

	p_ptz_status->ErrorFlag = 0;
    p_ptz_status->UtcTime = time(NULL);	
	
	return ONVIF_OK;
}


//
uint16_t  switchSpeed(float x, float y, float z)
{
	uint16_t speed;
	
	float x_value =  fabs(x);
	float y_value =  fabs(y);
	float z_value =  fabs(z);

    if ( x_value  > 0.000001 ) {
	 	speed = (int)(63*x_value);
	}
	else  if ( y_value > 0.000001 ) {
	 	speed = (int)(63*y_value);
	}
	else  if ( z_value > 0.000001 ) {
	 	speed = (int)(63*z_value);
	}
	
	return  speed;
}
//

ONVIF_RET onvif_ContinuousMove(ContinuousMove_REQ * p_req)
{
	ONVIF_PROFILE * p_profile = onvif_find_profile(p_req->ProfileToken);
	if (NULL == p_profile)
	{
		return ONVIF_ERR_NoProfile;
	}
    else if (NULL == g_onvif_cfg.ptz_node)
	{
		return ONVIF_ERR_NoPTZProfile;
	}
	
	if (p_req->Velocity.PanTiltFlag)
	{
	
		if (p_req->Velocity.PanTilt.x - g_onvif_cfg.ptz_node->PTZNode.SupportedPTZSpaces.ContinuousPanTiltVelocitySpace.XRange.Min < -FPP || 
		 	p_req->Velocity.PanTilt.x - g_onvif_cfg.ptz_node->PTZNode.SupportedPTZSpaces.ContinuousPanTiltVelocitySpace.XRange.Max > FPP)
		{
			return ONVIF_ERR_InvalidPosition;
		}

		if (p_req->Velocity.PanTilt.y - g_onvif_cfg.ptz_node->PTZNode.SupportedPTZSpaces.ContinuousPanTiltVelocitySpace.YRange.Min < -FPP || 
			p_req->Velocity.PanTilt.y - g_onvif_cfg.ptz_node->PTZNode.SupportedPTZSpaces.ContinuousPanTiltVelocitySpace.YRange.Max > FPP)
		{
			return ONVIF_ERR_InvalidPosition;
		}
	}
	
	if (p_req->Velocity.ZoomFlag && 
		(p_req->Velocity.Zoom.x - g_onvif_cfg.ptz_node->PTZNode.SupportedPTZSpaces.ContinuousZoomVelocitySpace.XRange.Min < -FPP ||     //Min = -1.0
		 p_req->Velocity.Zoom.x - g_onvif_cfg.ptz_node->PTZNode.SupportedPTZSpaces.ContinuousZoomVelocitySpace.XRange.Max > FPP))		//Max = 1.0
	{
		return ONVIF_ERR_InvalidPosition;
	}	

	// todo : add continuous move code ... 
	// printf("\nPTZvelocity :  x=%f , y = %f , z = %f \n",p_req->Velocity.PanTilt.x, p_req->Velocity.PanTilt.y, p_req->Velocity.Zoom.x);
	float x = p_req->Velocity.PanTilt.x;
	float y = p_req->Velocity.PanTilt.y;
	float z = p_req->Velocity.Zoom.x;

    uint16_t ptzSpeed;
	ptzSpeed  = switchSpeed(x,  y,  z);
	
	controlPtzPos(x, y, z , ptzSpeed);   //// add by xieqingpu

    return ONVIF_OK;
}

ONVIF_RET onvif_ptz_Stop(PTZ_Stop_REQ * p_req)
{
	ONVIF_PROFILE * p_profile = onvif_find_profile(p_req->ProfileToken);
	if (NULL == p_profile)
	{
		return ONVIF_ERR_NoProfile;
	}
	else if (NULL == g_onvif_cfg.ptz_node)
	{
		return ONVIF_ERR_NoPTZProfile;
	}

	// todo : add stop PTZ moving code ... 
	ptzStop();

    return ONVIF_OK;
}

ONVIF_RET onvif_AbsoluteMove(AbsoluteMove_REQ * p_req)
{	
	ONVIF_PROFILE * p_profile = onvif_find_profile(p_req->ProfileToken);
	if (NULL == p_profile)
	{
		return ONVIF_ERR_NoProfile;
	}
	else if (NULL == g_onvif_cfg.ptz_node)
	{
		return ONVIF_ERR_NoPTZProfile;
	}

	if (p_req->Position.PanTiltFlag)
	{
		if (p_req->Position.PanTilt.x - g_onvif_cfg.ptz_node->PTZNode.SupportedPTZSpaces.AbsolutePanTiltPositionSpace.XRange.Min < -FPP || 
		 	p_req->Position.PanTilt.x - g_onvif_cfg.ptz_node->PTZNode.SupportedPTZSpaces.AbsolutePanTiltPositionSpace.XRange.Max > FPP)
		{
			return ONVIF_ERR_InvalidPosition;
		}

		if (p_req->Position.PanTilt.y - g_onvif_cfg.ptz_node->PTZNode.SupportedPTZSpaces.AbsolutePanTiltPositionSpace.YRange.Min < -FPP || 
			p_req->Position.PanTilt.y - g_onvif_cfg.ptz_node->PTZNode.SupportedPTZSpaces.AbsolutePanTiltPositionSpace.YRange.Max > FPP)
		{
			return ONVIF_ERR_InvalidPosition;
		}
	}

	if (p_req->Position.ZoomFlag && 
		(p_req->Position.Zoom.x - g_onvif_cfg.ptz_node->PTZNode.SupportedPTZSpaces.AbsoluteZoomPositionSpace.XRange.Min < -FPP || 
		 p_req->Position.Zoom.x - g_onvif_cfg.ptz_node->PTZNode.SupportedPTZSpaces.AbsoluteZoomPositionSpace.XRange.Max > FPP))
	{
		return ONVIF_ERR_InvalidPosition;
	}	
	
	
	// todo : add absolute move code ...
	
    return ONVIF_OK;
}

ONVIF_RET onvif_RelativeMove(RelativeMove_REQ * p_req)
{
	ONVIF_PROFILE * p_profile = onvif_find_profile(p_req->ProfileToken);
	if (NULL == p_profile)
	{
		return ONVIF_ERR_NoProfile;
	}
	else if (NULL == g_onvif_cfg.ptz_node)
	{
		return ONVIF_ERR_NoPTZProfile;
	}

	if (p_req->Translation.PanTiltFlag)
	{
		if (p_req->Translation.PanTilt.x - g_onvif_cfg.ptz_node->PTZNode.SupportedPTZSpaces.RelativePanTiltTranslationSpace.XRange.Min < -FPP || 
			p_req->Translation.PanTilt.x - g_onvif_cfg.ptz_node->PTZNode.SupportedPTZSpaces.RelativePanTiltTranslationSpace.XRange.Max > FPP)
		{
			return ONVIF_ERR_InvalidPosition;
		}

		if (p_req->Translation.PanTilt.y - g_onvif_cfg.ptz_node->PTZNode.SupportedPTZSpaces.RelativePanTiltTranslationSpace.YRange.Min < -FPP || 
			p_req->Translation.PanTilt.y - g_onvif_cfg.ptz_node->PTZNode.SupportedPTZSpaces.RelativePanTiltTranslationSpace.YRange.Max > FPP)
		{
			return ONVIF_ERR_InvalidPosition;
		}
	}
	
	if (p_req->Translation.ZoomFlag && 
		(p_req->Translation.Zoom.x - g_onvif_cfg.ptz_node->PTZNode.SupportedPTZSpaces.RelativeZoomTranslationSpace.XRange.Min < -FPP || 
		 p_req->Translation.Zoom.x - g_onvif_cfg.ptz_node->PTZNode.SupportedPTZSpaces.RelativeZoomTranslationSpace.XRange.Max > FPP))
	{
		return ONVIF_ERR_InvalidPosition;
	}
	
	// todo : add relative move code ...
	
    return ONVIF_OK;
}

////// add by xieqingpu
int  onvif_find_PTZPreset_index(const char * profile_token, const char  * preset_token)
{
    int i;
    ONVIF_PROFILE * p_profile = onvif_find_profile(profile_token);
    if (NULL == p_profile)
    {
        return  -1;
    }

    for (i = 0; i < ARRAY_SIZE(p_profile->presets); i++)
    {
        if (strcmp(preset_token, p_profile->presets[i].PTZPreset.token) == 0)
        {
            return i;
        }
    }

	return  -1;
}
//////


ONVIF_RET onvif_SetPreset(SetPreset_REQ * p_req)
{
    ONVIF_PTZPreset * p_preset = NULL;
	ONVIF_PROFILE * p_profile;

	p_profile = onvif_find_profile(p_req->ProfileToken);
	if (NULL == p_profile)
	{
		return ONVIF_ERR_NoProfile;
	}
	else if (NULL == g_onvif_cfg.ptz_node)
	{
		return ONVIF_ERR_NoPTZProfile;
	}
	
    if (p_req->PresetTokenFlag && p_req->PresetToken[0] != '\0')
    {
        p_preset = onvif_find_PTZPreset(p_req->ProfileToken, p_req->PresetToken);	// &p_profile->presets[i]  creat preset tour
        if (NULL == p_preset)
        {
        	return ONVIF_ERR_NoToken;
        }
    }
    else
    {
        p_preset = onvif_get_idle_PTZPreset(p_req->ProfileToken);  // &p_profile->presets[i]
        if (NULL == p_preset)
        {
        	return ONVIF_ERR_TooManyPresets;
        }
     }

	/* add by xieqingpu 获取空闲的预置位的下标 */
	g_onvif_cls.preset_idx = onvif_get_idle_PTZPreset_idx(p_req->ProfileToken); 
	if (g_onvif_cls.preset_idx < 0)    g_onvif_cls.preset_idx++;
    
	if (p_req->PresetNameFlag && p_req->PresetName[0] != '\0')     // Preset Name
    {
    	strcpy(p_preset->PTZPreset.Name, p_req->PresetName);
    }
    else
    {
    	sprintf(p_preset->PTZPreset.Name, "PRESET_NAME_%d", g_onvif_cls.preset_idx);
    	strcpy(p_req->PresetName, p_preset->PTZPreset.Name);
    	// g_onvif_cls.preset_idx++;
    }
    
    if (p_req->PresetTokenFlag && p_req->PresetToken[0] != '\0')  // Preset Token
    {
        strcpy(p_preset->PTZPreset.token, p_req->PresetToken);
    }
    else
    {
        sprintf(p_preset->PTZPreset.token, "PRESET_%d", g_onvif_cls.preset_idx);
        strcpy(p_req->PresetToken, p_preset->PTZPreset.token);
        // g_onvif_cls.preset_idx++;
    }

   	// todo : get PTZ current position ...
 	// add by xieqingpu
	int i;
 	p_preset->UsedFlag = 1;	

 	int index = onvif_find_PTZPreset_index(p_req->ProfileToken, p_req->PresetToken);

	short location = index < 0 ? 1:index;
	/* 设置预置位 */
	setPtzPreset(location);


	/* 预置位对应的截取的图像区域 */
    if (p_req->VectorList_Flag )
	{
		p_profile->presets[index].VectorListFlag = 1;

		for (i = 0; i < p_req->VectorNumber; i++)
		{
			// printf("xxx \033[0;34m===onvif__SetPreset| VectorList: X=%0.3f, Y=%0.3f, W=%0.3f, H=%0.3f ===\033[0m\n", p_req->VectorList[i].x, p_req->VectorList[i].y, p_req->VectorList[i].w, p_req->VectorList[i].h);  
			p_profile->presets[index].Vector_Number = p_req->VectorNumber;
			p_profile->presets[index].Vector_list[i].x = p_req->VectorList[i].x;
			p_profile->presets[index].Vector_list[i].y = p_req->VectorList[i].y;
			p_profile->presets[index].Vector_list[i].w = p_req->VectorList[i].w;
			p_profile->presets[index].Vector_list[i].h = p_req->VectorList[i].h;
		}
	}
	else {
		p_profile->presets[index].VectorListFlag = 0;
	}
	

	/* 预置位对应的相机焦距 */
	uint16_t z = get_zoom_val();
	p_profile->presets[index].zoomVal = z;
	printf("xxx ===== onvif__SetPreset |p_profile->presets[%d].zoomVal: %d == z=%d\n", index, p_profile->presets[index].zoomVal, z);
 
	if (writePtzPreset(p_profile->presets, MAX_PTZ_PRESETS) != 0) //ARRAY_SIZE(p_profile->presets) //MAX_PTZ_PRESETS:其实该ptz设备最多支持256个预置位，但我只设置最多100个
		printf("write Ptz Preset faile.\n");
 ////

    p_preset->PTZPreset.PTZPositionFlag = 1;
    p_preset->PTZPreset.PTZPosition.PanTiltFlag = 1;
    p_preset->PTZPreset.PTZPosition.PanTilt.x = 0;
    p_preset->PTZPreset.PTZPosition.PanTilt.y = 0;
    p_preset->PTZPreset.PTZPosition.ZoomFlag = 1;
    p_preset->PTZPreset.PTZPosition.Zoom.x = 0;

    // p_preset->UsedFlag = 1; 
    
    return ONVIF_OK;
}

ONVIF_RET onvif_RemovePreset(RemovePreset_REQ * p_req)
{
    ONVIF_PROFILE * p_profile;
    ONVIF_PTZPreset * p_preset;

    p_profile = onvif_find_profile(p_req->ProfileToken);
	if (NULL == p_profile)
	{
		return ONVIF_ERR_NoProfile;
	}

	if (NULL == g_onvif_cfg.ptz_node)
	{
		return ONVIF_ERR_NoPTZProfile;
	}

    p_preset = onvif_find_PTZPreset(p_req->ProfileToken, p_req->PresetToken);  // &p_profile->presets[i]
    if (NULL == p_preset)
    {
		onvif_find_PTZPreset(p_req->ProfileToken, p_req->PresetToken);
    }

    memset(p_preset, 0, sizeof(ONVIF_PTZPreset));

	//// add by xieqingpu
	if (writePtzPreset(p_profile->presets, MAX_PTZ_PRESETS) != 0) //ARRAY_SIZE(p_profile->presets) //MAX_PTZ_PRESETS:其实该ptz设备最多支持256个预置位，但我只设置最多支持100个
		printf("write Ptz Preset faile.\n");
	////

    return ONVIF_OK;
}

ONVIF_RET onvif_GotoPreset(GotoPreset_REQ * p_req)
{	
	ONVIF_PROFILE * p_profile;
    ONVIF_PTZPreset * p_preset;
    
	p_profile = onvif_find_profile(p_req->ProfileToken);
	if (NULL == p_profile)
	{
		return ONVIF_ERR_NoProfile;
	}

	if (NULL == g_onvif_cfg.ptz_node)
	{
		return ONVIF_ERR_NoPTZProfile;
	}

    p_preset = onvif_find_PTZPreset(p_req->ProfileToken, p_req->PresetToken);  // &p_profile->presets[i]
    if (NULL == p_preset)
    {
        return ONVIF_ERR_NoToken;
    }

    // todo : add goto preset code ...
 	//// add by xieqingpu
 	int index = onvif_find_PTZPreset_index(p_req->ProfileToken, p_req->PresetToken); //获取preset的下标只是为了设置预置位

	short location = index < 0 ? 1 : index;
	gotoPtzPreset(location);

	/* 在 GetPresets/build_GetPresets_rly_xml 中已经读取到了 */
	/* if (readPtzPreset(p_profile->presets, MAX_PTZ_PRESETS) != 0)		//MAX_PTZ_PRESETS:其实该ptz设备最多支持256个预置位，但我只设置最多100个
			printf("read PTZ preset faile.\n"); */

	// printf("xxx ==== onvif_GotoPreset |p_profile->presets[%d].zoomVal: %d =====\n", index, p_profile->presets[index].zoomVal);

	uint16_t zoomValue ;
	zoomValue = p_profile->presets[index].zoomVal;
	set_zoom(zoomValue);
 	////

    return ONVIF_OK;
}

ONVIF_RET onvif_GotoHomePosition(GotoHomePosition_REQ * p_req)
{
    ONVIF_PROFILE * p_profile = onvif_find_profile(p_req->ProfileToken);
    if (NULL == p_profile)
    {
        return ONVIF_ERR_NoProfile;
    }

    if (NULL == g_onvif_cfg.ptz_node)
    {
    	return ONVIF_ERR_NoPTZProfile;
    }

    // todo : add goto home position code ...
	gotoPtzPreset(126);		  		 //HomePosition其实也是预置位的一个

	CONFIG_Home homePreset;
	memset(&homePreset, 0, sizeof(CONFIG_Home));
	if (readHomePos(&homePreset) != 0) 
		printf("write Ptz Preset faile.\n");

	uint16_t homeZoomVal ;
	homeZoomVal = homePreset.homeZoom;
	set_zoom(homeZoomVal);

    return ONVIF_OK;
}

ONVIF_RET onvif_SetHomePosition(const char * token)      // ssetPreset
{
	ONVIF_PROFILE * p_profile = onvif_find_profile(token);
    if (NULL == p_profile)
    {
        return ONVIF_ERR_NoProfile;
    }

    if (NULL == g_onvif_cfg.ptz_node)
    {
    	return ONVIF_ERR_NoPTZProfile;
    }

	if (g_onvif_cfg.ptz_node->PTZNode.FixedHomePositionFlag && 
	    g_onvif_cfg.ptz_node->PTZNode.FixedHomePosition)
	{
		return ONVIF_ERR_CannotOverwriteHome;
	}
	
    // todo : add set home position code ...
	setPtzPreset(126);         //HomePosition其实也是预置位的一个

	CONFIG_Home homePreset;
	memset(&homePreset, 0, sizeof(CONFIG_Home));

	homePreset.homeZoom = get_zoom_val();

	if ( writeHomePos(&homePreset) != 0)
		printf("write Ptz Preset faile.\n");

    return ONVIF_OK;
}

ONVIF_RET onvif_SetConfiguration(SetConfiguration_REQ * p_req)
{
	ONVIF_PTZConfiguration * p_ptz_cfg;
	ONVIF_PTZNode * p_ptz_node;

	p_ptz_cfg = onvif_find_PTZConfiguration(p_req->PTZConfiguration.token);
	if (NULL == p_ptz_cfg)
	{
		return ONVIF_ERR_NoConfig;
	}
	
	p_ptz_node = onvif_find_PTZNode(p_req->PTZConfiguration.NodeToken);
	if (NULL == p_ptz_node)
	{
		return ONVIF_ERR_ConfigModify;
	}

	if (p_req->PTZConfiguration.DefaultPTZTimeoutFlag)
	{
		if (p_req->PTZConfiguration.DefaultPTZTimeout < g_onvif_cfg.PTZConfigurationOptions.PTZTimeout.Min ||
			p_req->PTZConfiguration.DefaultPTZTimeout > g_onvif_cfg.PTZConfigurationOptions.PTZTimeout.Max)
		{
			return ONVIF_ERR_ConfigModify;
		}
	}

	// todo : add set ptz configuration code ...


    if (p_req->PTZConfiguration.MoveRampFlag)
    {
        p_ptz_cfg->Configuration.MoveRamp = p_req->PTZConfiguration.MoveRamp;
    }
    
    if (p_req->PTZConfiguration.PresetRampFlag)
    {
        p_ptz_cfg->Configuration.PresetRamp = p_req->PTZConfiguration.PresetRamp;
    }
    
    if (p_req->PTZConfiguration.PresetTourRampFlag)
    {
        p_ptz_cfg->Configuration.PresetTourRamp = p_req->PTZConfiguration.PresetTourRamp;
    }    

	strcpy(p_ptz_cfg->Configuration.Name, p_req->PTZConfiguration.Name);
	if (p_req->PTZConfiguration.DefaultPTZSpeedFlag)
	{
		if (p_req->PTZConfiguration.DefaultPTZSpeed.PanTiltFlag)
		{
			p_ptz_cfg->Configuration.DefaultPTZSpeed.PanTilt.x = p_req->PTZConfiguration.DefaultPTZSpeed.PanTilt.x;
			p_ptz_cfg->Configuration.DefaultPTZSpeed.PanTilt.y = p_req->PTZConfiguration.DefaultPTZSpeed.PanTilt.y;
		}

		if (p_req->PTZConfiguration.DefaultPTZSpeed.ZoomFlag)
		{
			p_ptz_cfg->Configuration.DefaultPTZSpeed.Zoom.x = p_req->PTZConfiguration.DefaultPTZSpeed.Zoom.x;
		}
	}

	if (p_req->PTZConfiguration.DefaultPTZTimeoutFlag)
	{
		p_ptz_cfg->Configuration.DefaultPTZTimeout = p_req->PTZConfiguration.DefaultPTZTimeout;
	}

	if (p_req->PTZConfiguration.PanTiltLimitsFlag)
	{
		memcpy(&p_ptz_cfg->Configuration.PanTiltLimits, &p_req->PTZConfiguration.PanTiltLimits, sizeof(onvif_PanTiltLimits));
	}

	if (p_req->PTZConfiguration.ZoomLimitsFlag)
	{
		memcpy(&p_ptz_cfg->Configuration.ZoomLimits, &p_req->PTZConfiguration.ZoomLimits, sizeof(onvif_ZoomLimits));
	}

	if (p_req->PTZConfiguration.ExtensionFlag)
	{
		if (p_req->PTZConfiguration.Extension.PTControlDirectionFlag)
		{
			if (p_req->PTZConfiguration.Extension.PTControlDirection.EFlipFlag)
			{
				p_ptz_cfg->Configuration.Extension.PTControlDirection.EFlip = p_req->PTZConfiguration.Extension.PTControlDirection.EFlip;
			}

			if (p_req->PTZConfiguration.Extension.PTControlDirection.ReverseFlag)
			{
				p_ptz_cfg->Configuration.Extension.PTControlDirection.Reverse = p_req->PTZConfiguration.Extension.PTControlDirection.Reverse;
			}
		}
	}
    
#ifdef MEDIA2_SUPPORT
    onvif_MediaConfigurationChangedNotify(p_req->PTZConfiguration.token, "PTZ");
#endif

	return ONVIF_OK;
}

/* add PresetTour by xieqingpu */

ONVIF_RET onvif_CreatePresetTour(PresetTour_REQ * p_req)
{
    ONVIF_PresetTour * p_PresetTour = NULL;
	ONVIF_PROFILE * p_profile;

	p_profile = onvif_find_profile(p_req->ProfileToken);
	if (NULL == p_profile)
	{
		return ONVIF_ERR_NoProfile;
	}
	else if (NULL == g_onvif_cfg.ptz_node)
	{
		return ONVIF_ERR_NoPTZProfile;
	}
	

	p_PresetTour = onvif_add_PresetTour(&p_profile->PresetTours); 
	if (p_PresetTour)
	{
		/*  获取空闲的巡更的下标 */
		g_onvif_cls.preset_tour_idx = onvif_get_idle_PresetTour_idx(); 
		if (g_onvif_cls.preset_tour_idx < 0)    g_onvif_cls.preset_tour_idx++;

		/* p_presetTour = onvif_get_idle_PresetTour(p_req->ProfileToken); */

        sprintf(p_PresetTour->PresetTour.token, "PRESET_TOUR_%d", g_onvif_cls.preset_tour_idx);
        strcpy(p_req->PresetTourToken, p_PresetTour->PresetTour.token);
        // g_onvif_cls.preset_tour_idx++;
		
        strcpy(PTZPresetsTour[g_onvif_cls.preset_tour_idx].PresetTourToken, p_PresetTour->PresetTour.token);

		ONVIF_PTZPresetTourSpot * p_tour_spot = onvif_add_TourSpot(&p_PresetTour->PresetTour.TourSpot);
		if (NULL == p_tour_spot)
		{
			free(p_PresetTour);
			return ONVIF_ERR_OTHER;
		}
	}

	return ONVIF_OK;
}

static int presetTourStatus =  PTZPresetTourOperation_Start;
//毫秒
static void onvif_preset_usleep(int milisec)
{
	struct timeval delay;
	int count = 1;
	if (milisec/100000 > 1)
		count = milisec/100000;

	while (count)
	{
		if (presetTourStatus == PTZPresetTourOperation_Stop)
		{
			break;
		}

		delay.tv_sec = 0;
		//10*1000是10毫秒，如果延时的时间是在一秒以内只需要改下面这句
		//毫秒级别的定时把10改了就好，比如29毫秒延时改成，29*1000
		delay.tv_usec = 100*1000;
		select(0, NULL, NULL, NULL, &delay);
		count--;
	}
}

void *PresetTour_state_touring_Forward(void *args)
{
    ONVIF_PresetTour * p_PresetTour;
	PresetsTours_t presetTours;
	PresetsTours_t * tempresetTours = (PresetsTours_t *)args;
	if (tempresetTours == NULL)
	{
		return NULL;
    }

	memset(&presetTours, 0x0, sizeof(PresetsTours_t));
    memcpy(&presetTours, tempresetTours, sizeof(PresetsTours_t));
    free(tempresetTours);
	tempresetTours = NULL;


#ifdef PTOURS_TIME_NUMBER
	p_PresetTour = onvif_find_PTZPresetTour(presetTours.ProfileToken, presetTours.PresetTourToken);
    if (NULL == p_PresetTour)
    {
		printf("xxx PresetTour_state_touring_Forward | onvif_find_PTZPresetTour==NULL\n");
        return NULL;
    }
#endif

	uint32_t j = 0;
	uint32_t i = presetTours.presetCount;
	// printf("xxxx PresetTour_state_touring_Forward | presetTours->presetCount预置位数 = %d xxx\n", presetTours.presetCount);

	static uint32_t runningTime = 0;    //巡航了多久
	static uint32_t runningNumber = 0;	//巡航了多少次

	while (1)
	{
		if (presetTourStatus == PTZPresetTourOperation_Stop)
		{
			// UTIL_INFO("!!!PTZPresetTourOperation_Stop!!!!");
			return NULL;
		}
		else if (presetTourStatus == PTZPresetTourOperation_Pause)
		{
			UTIL_INFO("!!!PTZPresetTourOperation_Pause Pause Pause !!!\n");
			usleep(500*1000);
			continue;
		}

#ifdef PTOURS_TIME_NUMBER
		if ((presetTours.runTimeFlag && (runningTime >= presetTours.runTime)) || 
			(presetTours.runNumberFlag && (runningNumber >= presetTours.runNumber)))
		{
			p_PresetTour->PresetTour.Status.State = PTZPresetTourState_Idle;    /* 条件已到结束巡航，让巡航处于空闲状态 */
			ptzStop();
			return NULL;
		}
#endif

		gotoPtzPreset(presetTours.presets[j].index);
		set_zoom(presetTours.presets[j].zoomValue);

		// UTIL_INFO("\033[0;33mxxxx touringp_PresetTour_Forward presets.index = %d zoomValue = %d presetTourStatus=%d xxxx\033[0m\n", 
		// 		presetTours.presets[j].index, presetTours.presets[j].zoomValue, presetTourStatus);    //33黄
		int staytime = (6 + presetTours.presets[j].StayTime)*1000*1000;
		onvif_preset_usleep(staytime);

		// for(int ms = 0; ms < staytime; ms+=100)
		// {
		// 	if (presetTourStatus == PTZPresetTourOperation_Stop)
		// 	{
		// 		UTIL_INFO("!!!PTZPresetTourOperation_Stop!Stop!Stop!!");
		// 		return NULL;
		// 	}
		// 	usleep(100*1000); 
		// }

#ifdef PTOURS_TIME_NUMBER
		if (presetTours.runTimeFlag)
		{
			runningTime += (5 + presetTours.presets[j].StayTime);
		}
#endif

		j += 1;
		j = j%i;

#ifdef PTOURS_TIME_NUMBER
		if (presetTours.runNumberFlag)
		{
			if (j == 0)   //j归零说明巡航完一次
			{
				runningNumber++;
			}
		}
#endif
	}

	return NULL;
}

void *PresetTour_state_touring_Backward(void *args)
{
    ONVIF_PresetTour * p_PresetTour;
	PresetsTours_t presetTours;
	PresetsTours_t * tempresetTours = (PresetsTours_t *)args;
	if (tempresetTours == NULL)
	{
		return NULL;
    }

	memset(&presetTours, 0x0, sizeof(PresetsTours_t));
    memcpy(&presetTours, tempresetTours, sizeof(PresetsTours_t));
    free(tempresetTours);
	tempresetTours = NULL;


#ifdef PTOURS_TIME_NUMBER
	p_PresetTour = onvif_find_PTZPresetTour(presetTours.ProfileToken, presetTours.PresetTourToken);
    if (NULL == p_PresetTour)
    {
		printf("xxx PresetTour_state_touring_Forward | onvif_find_PTZPresetTour==NULL\n");
        return NULL;
    }
#endif

	uint32_t i = presetTours.presetCount;
	uint32_t j = i-1;

	static uint32_t runningTime = 0;     //巡航了多久
	static uint32_t runningNumber = 0;	//巡航了多少次

	while (1)
	{
		if (presetTourStatus == PTZPresetTourOperation_Stop)
		{
			// UTIL_INFO("!!!PTZPresetTourOperation_Stop!!!!");
			return NULL;
		}
		else if(presetTourStatus == PTZPresetTourOperation_Pause)
		{
			UTIL_INFO("!!!PTZPresetTourOperation_Pause Pause Pause !!!\n");
			usleep(500*1000);
			continue;
		}

#ifdef PTOURS_TIME_NUMBER
		if ((presetTours.runTimeFlag && (runningTime >= presetTours.runTime)) || 
			(presetTours.runNumberFlag && (runningNumber >= presetTours.runNumber)))
		{
			p_PresetTour->PresetTour.Status.State = PTZPresetTourState_Idle;    /* 条件已到结束巡航，让巡航处于空闲状态 */
			ptzStop();
			return NULL;
		}
#endif

		gotoPtzPreset(presetTours.presets[j].index);
		set_zoom(presetTours.presets[j].zoomValue);

		// UTIL_INFO("\033[0;33mxxxx touring_Backward presets.index = %d zoomValue = %d xxxx\033[0m\n", 
		// 		presetTours.presets[j].index, presetTours.presets[j].zoomValue);    //33黄
		int staytime = (6 + presetTours.presets[j].StayTime)*1000*1000;
		onvif_preset_usleep(staytime);

		// for(int ms = 0; ms < staytime; ms+=100)
		// {
		// 	if (presetTourStatus == PTZPresetTourOperation_Stop)
		// 	{
		// 		UTIL_INFO("!!!PTZPresetTourOperation_Stop!!!!");
		// 		return NULL;
		// 	}
		// 	usleep(100*1000);
		// }

#ifdef PTOURS_TIME_NUMBER
		if (presetTours.runTimeFlag)
		{
			runningTime += (5 + presetTours.presets[j].StayTime);
		}
#endif

		if (j == 0)  
		{
			j = i;
#ifdef PTOURS_TIME_NUMBER
			if (presetTours.runNumberFlag)
			{
				runningNumber++;  //j归零说明巡航完一次
			}
#endif
		}

		j -= 1;
		j = j%i;
	}

	return NULL;
}

void *PresetTour_state_touring_Random(void *args)
{
    ONVIF_PresetTour * p_PresetTour;
	PresetsTours_t presetTours;
	PresetsTours_t * tempresetTours = (PresetsTours_t *)args;
	if (tempresetTours == NULL)
	{
		return NULL;
    }

	memset(&presetTours, 0x0, sizeof(PresetsTours_t));
    memcpy(&presetTours, tempresetTours, sizeof(PresetsTours_t));
    free(tempresetTours);
	tempresetTours = NULL;


#ifdef PTOURS_TIME_NUMBER
	p_PresetTour = onvif_find_PTZPresetTour(presetTours.ProfileToken, presetTours.PresetTourToken);
    if (NULL == p_PresetTour)
    {
		printf("xxx PresetTour_state_touring_Forward | onvif_find_PTZPresetTour==NULL\n");
        return NULL;
    }
#endif

	uint32_t i = presetTours.presetCount;
	uint32_t j, tmp = 0;
	uint32_t preset_index[64];
	uint32_t x = 0;

	static uint32_t runningTime = 0;     //巡航了多久
	static uint32_t runningNumber = 0;	 //巡航了多少次

	while (1)
	{
		if (presetTourStatus == PTZPresetTourOperation_Stop)
		{
			// UTIL_INFO("!!!PTZPresetTourOperation_Stop!!!!");
			return NULL;
		}
		else if (presetTourStatus == PTZPresetTourOperation_Pause)
		{
			UTIL_INFO("!!!PTZPresetTourOperation_Pause Pause Pause !!!\n");
			usleep(500*1000);
			continue;
		}

#ifdef PTOURS_TIME_NUMBER
		if ((presetTours.runTimeFlag && (runningTime >= presetTours.runTime)) || 
			(presetTours.runNumberFlag && (runningNumber >= presetTours.runNumber)))
		{
			p_PresetTour->PresetTour.Status.State = PTZPresetTourState_Idle;    /* 条件已到结束巡航，让巡航处于空闲状态 */
			ptzStop();
			return NULL;
		}
#endif

		//
		srand((uint32_t)time(NULL));  //随机选择预置位
		j = rand()%i;

		while (tmp == j)	//若巡更预置位与上一次的一样，则再次随机选择预置位，选择与上次不一样的预置位
		{
   			static uint16_t seed;
			seed = seed%100;

			srand((uint32_t)time(NULL) + ++seed);
			j = rand()%i;
		}
		//

		gotoPtzPreset(presetTours.presets[j].index);
		set_zoom(presetTours.presets[j].zoomValue);

		// UTIL_INFO("\033[0;33mxxxx touring_Random presets.index = %d zoomValue = %d xxxx\033[0m\n", 
		// 		presetTours.presets[j].index, presetTours.presets[j].zoomValue);    //33黄
		int staytime = (6 + presetTours.presets[j].StayTime)*1000*1000;
		onvif_preset_usleep(staytime);

		// for(int ms = 0; ms < staytime; ms+=100)
		// {
		// 	if (presetTourStatus == PTZPresetTourOperation_Stop)
		// 	{
		// 		UTIL_INFO("!!!PTZPresetTourOperation_Stop!!!!");
		// 		return NULL;
		// 	}
		// 	usleep(100*1000);
		// }

#ifdef PTOURS_TIME_NUMBER
		if (presetTours.runTimeFlag)
		{
			runningTime += (5 + presetTours.presets[j].StayTime);
		}

		if (presetTours.runNumberFlag)
		{
			x += 1;
			x = x%i;
			if (x == 0)   //随机巡航i个预置位（i:巡航的预置位个数）说明巡航完一次
			{
				runningNumber++;
			}
		}
#endif

		tmp = j;
	}

	return NULL;
}

int onvif_OperatePresetTour_task(OperatePresetTour_REQ * OpPresetTour)
{
	if (OpPresetTour == NULL)
	{
		return ONVIF_ERR_OTHER;
    }
		
	ONVIF_PROFILE * p_profile;
    ONVIF_PresetTour * p_PresetTour;
	uint32_t i = 0;
	PresetsTours_t  *presetTours = NULL;
	pthread_t tid;


	p_profile = onvif_find_profile(OpPresetTour->ProfileToken);
	if (NULL == p_profile)
	{
		return ONVIF_ERR_OTHER;
	}

	p_PresetTour = onvif_find_PTZPresetTour(OpPresetTour->ProfileToken, OpPresetTour->PresetTourToken);
    if (NULL == p_PresetTour)
    {
        return ONVIF_ERR_OTHER;
    }

	onvif_PTZPresetTourOperation op = OpPresetTour->Operation;
	onvif_PTZPresetTourDirection direction = p_PresetTour->PresetTour.StartingCondition.Direction;
	ONVIF_PTZPresetTourSpot * p_TourSpot = p_PresetTour->PresetTour.TourSpot;
    
	if(PTZPresetTourOperation_Start == op && presetTourStatus != PTZPresetTourOperation_Pause)
	{
		presetTours = (PresetsTours_t *)malloc(sizeof(PresetsTours_t));
		if (NULL == presetTours)
		{
			return ONVIF_ERR_OTHER;
		}
		memset(presetTours, 0, sizeof(PresetsTours_t));


	#ifdef PTOURS_TIME_NUMBER
		if (OpPresetTour->ProfileToken[0] != '\0'){
			strncpy(presetTours->ProfileToken, OpPresetTour->ProfileToken, sizeof(OpPresetTour->ProfileToken)-1);
		}

		if (OpPresetTour->PresetTourToken[0] != '\0'){
			strncpy(presetTours->PresetTourToken, OpPresetTour->PresetTourToken, sizeof(OpPresetTour->PresetTourToken)-1);
		}
	#endif

		while (p_TourSpot)
		{
			presetTours->presets[i].index = onvif_find_PTZPreset_index(OpPresetTour->ProfileToken, p_TourSpot->PTZPresetTourSpot.PresetDetail.PresetToken);
			presetTours->presets[i].StayTime = p_TourSpot->PTZPresetTourSpot.StayTime;
			presetTours->presets[i].zoomValue = p_profile->presets[presetTours->presets[i].index].zoomVal;
			i++;    //预置位数

			p_TourSpot = p_TourSpot->next;
		}

		// printf("xxxx onvif_OperatePresetTour_task | presetTours->presetCount预置位数 = %d xxx\n", i);
		presetTours->presetCount = i;
		presetTours->direction = direction;
		if (p_PresetTour->PresetTour.StartingCondition.RecurringTimeFlag)
		{
			presetTours->runNumberFlag = 1;
			presetTours->runNumber = p_PresetTour->PresetTour.StartingCondition.RecurringTime;
		}else {
			presetTours->runNumberFlag = 0;
		}
		
		if (p_PresetTour->PresetTour.StartingCondition.RecurringDurationFlag)
		{
			presetTours->runTimeFlag = 1;
			presetTours->runTime = p_PresetTour->PresetTour.StartingCondition.RecurringDuration;
		}else {
			presetTours->runTimeFlag = 0;
		}
	}
    

	switch (op)
	{
		case PTZPresetTourOperation_Start:
			p_PresetTour->PresetTour.Status.State = PTZPresetTourState_Touring;

			if (presetTourStatus == PTZPresetTourOperation_Pause)
			{
				presetTourStatus = PTZPresetTourOperation_Start;
				UTIL_INFO("PAUSE!!!!!!!!!RESTART!!!!!!!!!!!");
				break;
			}
            presetTourStatus = PTZPresetTourOperation_Start;
			
			if (p_PresetTour->PresetTour.StartingCondition.DirectionFlag && direction == PTZPresetTourDirection_Forward)		
			{
				tid = sys_os_create_thread((void *)PresetTour_state_touring_Forward, presetTours);	 //向前按顺序巡航 0				
			}
			else if (p_PresetTour->PresetTour.StartingCondition.DirectionFlag && direction == PTZPresetTourDirection_Backward)	
			{
				tid = sys_os_create_thread((void *)PresetTour_state_touring_Backward, presetTours);  //向后按顺序巡航 1
			}

			if (p_PresetTour->PresetTour.StartingCondition.RandomPresetOrderFlag 
				&& p_PresetTour->PresetTour.StartingCondition.RandomPresetOrder)   //是否随机，true:1,FALSE:0，随机则方向将被忽略，并随机调用巡更的预设值
			{
				tid = sys_os_create_thread((void *)PresetTour_state_touring_Random, presetTours);
			}

			if (tid == 0)
			{
				free(presetTours);
				presetTours = NULL;
				log_print(LOG_ERR, "%s, create PresetTour_state_touring_xxx failed\r\n", __FUNCTION__);
				return ONVIF_ERR_OTHER;
			}
			break;

		case PTZPresetTourOperation_Stop:
			p_PresetTour->PresetTour.Status.State = PTZPresetTourState_Idle;    /* 停止即是让巡航处于空闲状态 */
            presetTourStatus = PTZPresetTourOperation_Stop;  
			ptzStop();
			break;

		case PTZPresetTourOperation_Pause:
			p_PresetTour->PresetTour.Status.State = PTZPresetTourState_Paused;
			presetTourStatus = PTZPresetTourOperation_Pause;
			ptzStop();
			break;

		case PTZPresetTourOperation_Extended:

			break;
	}

    return ONVIF_OK;
}

ONVIF_RET onvif_OperatePresetTour(OperatePresetTour_REQ * p_req)
{
	ONVIF_PROFILE * p_profile;
	int ret = ONVIF_OK;
    
	p_profile = onvif_find_profile(p_req->ProfileToken);
	if (NULL == p_profile)
	{
		return ONVIF_ERR_NoProfile;
	}

	if (NULL == g_onvif_cfg.ptz_node)
	{
		return ONVIF_ERR_NoPTZProfile;
	}
	// if (g_onvif_cfg.ptz_node->PTZNode.Extension.SupportedPresetTourFlag)
	// {
	// 	return ONVIF_ERR_OTHER;
	// }

    // todo : add Operate Preset Tour code ...

	ret = onvif_OperatePresetTour_task(p_req);
	if (ret < 0)
	{
		return ret;
	}

    return ONVIF_OK;
}

ONVIF_RET onvif_RemovePresetTour(PresetTour_REQ * p_req)
{
	ONVIF_PROFILE * p_profile;
    ONVIF_PresetTour * p_PresetTour;
    
	p_profile = onvif_find_profile(p_req->ProfileToken);
	if (NULL == p_profile)
	{
		return ONVIF_ERR_NoProfile;
	}

	if (NULL == g_onvif_cfg.ptz_node)
	{
		return ONVIF_ERR_NoPTZProfile;
	}
	// if (g_onvif_cfg.ptz_node->PTZNode.Extension.SupportedPresetTourFlag)
	// {
	// 	return ONVIF_ERR_OTHER;
	// }

    // p_preset = onvif_find_PTZPreset(p_req->ProfileToken, p_req->PresetToken);  // &p_profile->presets[i]
    p_PresetTour = onvif_find_PTZPresetTour(p_req->ProfileToken, p_req->PresetTourToken);  //p_profile->PresetTours
    if (NULL == p_PresetTour)
    {
        return ONVIF_ERR_OTHER;
    }

	onvif_remove_PresetTour(&p_profile->PresetTours, p_PresetTour);


    // todo : add Remove Preset Tour code ...
	PTZ_PresetsTours_t * p_presetTour = NULL;
	p_presetTour = onvif_find_PresetTour(p_req->PresetTourToken);    //&PTZPresetsTour[i]
	if (NULL == p_presetTour)
	{
		return ONVIF_ERR_OTHER;
	}

	memset(p_presetTour, 0, sizeof(PTZ_PresetsTours_t));

	//删掉某个巡更后,更新保存起来
	if (writePtzPresetTour(PTZPresetsTour, MAX_PRESETS_TOUR) != 0)
	{
		printf("onvif_RemovePresetTour | writePtzPresetTour faile...\n");
	}

    return ONVIF_OK;
}


ONVIF_RET onvif_ModifyPresetTour(ModifyPresetTour_REQ * p_req)
{
	ONVIF_PROFILE * p_profile;
    ONVIF_PresetTour * p_PresetTour;
	ONVIF_PresetTour * p_tmp;
	ONVIF_PresetTour * p_prev;
    
	p_profile = onvif_find_profile(p_req->ProfileToken);
	if (NULL == p_profile)
	{
		return ONVIF_ERR_NoProfile;
	}

	if (NULL == g_onvif_cfg.ptz_node)
	{
		return ONVIF_ERR_NoPTZProfile;
	}
	// if (g_onvif_cfg.ptz_node->PTZNode.Extension.SupportedPresetTourFlag)
	// {
	// 	return ONVIF_ERR_OTHER;
	// }

	p_tmp = p_req->PresetTour_req;
	//while (p_tmp)
	{
		p_PresetTour = onvif_find_PTZPresetTour(p_req->ProfileToken, p_tmp->PresetTour.token);
		if (NULL == p_PresetTour)
		{
			// onvif_free_PresetTours(&p_tmp);
			onvif_free_TourSpots(&(p_tmp->PresetTour.TourSpot));
			free(p_tmp);
			p_tmp = NULL;

			return ONVIF_ERR_OTHER;
		}

		p_prev = onvif_get_prev_presetTour(&p_profile->PresetTours, p_PresetTour);
		if (NULL == p_prev)
		{
			p_profile->PresetTours = p_tmp;
			p_tmp->next = p_PresetTour->next;
		}
		else
		{
			p_prev->next = p_tmp;
			p_tmp->next = p_PresetTour->next;
		}
		
		onvif_free_TourSpots(&(p_PresetTour->PresetTour.TourSpot));
		free(p_PresetTour);

		//p_tmp = p_tmp->next;
	}

	//todo:creat preset tour    
    PTZ_PresetsTours_t * p_presetTour = NULL;
	p_presetTour = onvif_find_PresetTour(p_req->PresetTour_req->PresetTour.token);    //&PTZPresetsTour[i]
	if (NULL == p_presetTour)
	{
		return ONVIF_ERR_OTHER;
	}
	/* p_presetTour = onvif_get_idle_PresetTour(); 
	if (NULL == p_presetTour)
	{
		return ONVIF_ERR_OTHER;
	} */

	p_presetTour->UsedFlag = 1;
	strcpy(p_presetTour->Name, p_req->PresetTour_req->PresetTour.Name);
	if (p_req->PresetTour_req->PresetTour.StartingCondition.RecurringTimeFlag){
		p_presetTour->PresetsTour.runNumber = p_req->PresetTour_req->PresetTour.StartingCondition.RecurringTime;
	}
	if (p_req->PresetTour_req->PresetTour.StartingCondition.RecurringDurationFlag){
		p_presetTour->PresetsTour.runTime   = p_req->PresetTour_req->PresetTour.StartingCondition.RecurringDuration;
	}
	if (p_req->PresetTour_req->PresetTour.StartingCondition.DirectionFlag){    //与RandomPresetOrder互斥
		p_presetTour->PresetsTour.direction = p_req->PresetTour_req->PresetTour.StartingCondition.Direction;
	}
	if (p_req->PresetTour_req->PresetTour.StartingCondition.RandomPresetOrderFlag){   //与Direction互斥
		p_presetTour->PresetsTour.RandomOrder = p_req->PresetTour_req->PresetTour.StartingCondition.RandomPresetOrder;
	}

	uint32_t i = 0;
	ONVIF_PTZPresetTourSpot * p_TourSpot = p_req->PresetTour_req->PresetTour.TourSpot;
	while (p_TourSpot)
	{
		p_presetTour->PresetsTour.presets[i].index = onvif_find_PTZPreset_index(p_req->ProfileToken, p_TourSpot->PTZPresetTourSpot.PresetDetail.PresetToken);
		p_presetTour->PresetsTour.presets[i].zoomValue = p_profile->presets[p_presetTour->PresetsTour.presets[i].index].zoomVal;
		p_presetTour->PresetsTour.presets[i].StayTime = p_TourSpot->PTZPresetTourSpot.StayTime;
		// printf("\033[0;34m onvif_ModifyPresetTour | presets[%d].StayTime= %d, p_TourSpot->PTZPresetTourSpot.StayTime= %d.\033[0m\n",
		// 			i, p_presetTour->PresetsTour.presets[i].StayTime, p_TourSpot->PTZPresetTourSpot.StayTime);   //34蓝
		strcpy(p_presetTour->PresetsTour.presets[i].PresetToken, p_TourSpot->PTZPresetTourSpot.PresetDetail.PresetToken);
		i++;    //巡更的预置位数量

		p_TourSpot = p_TourSpot->next;
	}
	p_presetTour->PresetsTour.presetCount = i;

	/* 计算巡更数 */
	/* int PT_count = 0;
	ONVIF_PresetTour * PT_tmp = p_profile->PresetTours;
	while (PT_tmp)
	{
		PT_count++;
		PT_tmp = PT_tmp->next;
	}*/
	
	//更新某个巡更后,将所有巡更保存起来
	if (writePtzPresetTour(PTZPresetsTour, MAX_PRESETS_TOUR) != 0)
	{
		printf("onvif_ModifyPresetTour | writePtzPresetTour faile...\n");
	}

	return ONVIF_OK;
}

/* add PresetTour end */

#endif // PTZ_SUPPORT


