/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation, nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*============================================================================
  INCLUDE FILES
  ==========================================================================*/

/* ADB logcat */
#define LOG_TAG "qc_os_sensors_hal"
#define LOG_NDEBUG 0

#include <cutils/log.h>
#include <stdlib.h>
#include "hardware/sensors.h"
#include "hardware/hardware.h"
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <dlfcn.h>

/*============================================================================
                   INTERNAL DEFINITIONS AND TYPES
============================================================================*/

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define QCOM_SNS_HAL_LIB_PATH      "/system/lib/hw/sensors_qcom.so"
#define QCOM_NATIVE_HAL_LIB_PATH   "/system/lib/hw/sensors_native_hal.so"
#define SYSFS_PLATFORMID           "/sys/devices/soc0/soc_id"
#define SYSFS_PLATFORMID_OLD       "/sys/devices/system/soc/soc0/id"

/*============================================================================
                    FUNCTION DECLARATIONS
============================================================================*/

/* function defined by Sensors HAL in (hardware/libhardware/include/hardware/sensors.h) */
static int _hal_sensors_open( const struct hw_module_t* module, const char* name,
                              struct hw_device_t* *device);
static int
_hal_sensors_get_sensors_list( struct sensors_module_t* module,
                               struct sensor_t const** list );

/*============================================================================
  Static Variable Definitions
  ===========================================================================*/

static struct hw_module_methods_t sensors_module_methods = {
	.open = _hal_sensors_open
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
	.common = {
		.tag = HARDWARE_MODULE_TAG,
		.version_major = 2,
		.version_minor = 0,
		.id = SENSORS_HARDWARE_MODULE_ID,
		.name = "Sensors Module",
		.author = "Linux Foundation.",
		.methods = &sensors_module_methods,
	},
	.get_sensors_list = _hal_sensors_get_sensors_list
};

struct sensors_poll_device_t *poll_device;

enum sensor_msm_id {
	MSM_UNKNOWN = 0,
	MSM_8930,
};

struct sensor_msm_soc_type {
	enum sensor_msm_id msm_id;
	int  soc_id;
};

static struct sensor_msm_soc_type msm_soc_table[] = {
	{MSM_8930, 116},
	{MSM_8930, 117},
	{MSM_8930, 118},
	{MSM_8930, 119},
	{MSM_8930, 142},
	{MSM_8930, 143},
	{MSM_8930, 144},
	{MSM_8930, 154},
	{MSM_8930, 155},
	{MSM_8930, 156},
	{MSM_8930, 157},
	{MSM_8930, 179},
	{MSM_8930, 180},
	{MSM_8930, 181},
};

/*============================================================================
  Static Function Definitions and Documentation
  ==========================================================================*/

static int get_node(char *buf, const char *path) {
	char * fret;
	int len = 0;
	FILE * fd;

	fd = fopen(path, "r");
	if (NULL == fd)
		return -1;

	fret = fgets(buf,sizeof(buf),fd);
	if (NULL == fret)
		return -1;

	len = strlen(buf);

	if (buf[len - 1] == '\n')
		buf[len - 1] = '\0';

	fclose(fd);
	return 0;

}

static int get_msm_id(void) {
	int ret;
	int soc_id = 0;
	int msm_id;
	unsigned index = 0;
	char buf[10];

	if (!access(SYSFS_PLATFORMID, F_OK))
	{
		ret = get_node(buf, SYSFS_PLATFORMID);
	} else {
		ret = get_node(buf, SYSFS_PLATFORMID_OLD);
	}
	if(ret == -1)
		return ret;
	soc_id = atoi(buf);

	for(index = 0; index < ARRAY_SIZE(msm_soc_table); index++) {
		if(soc_id == msm_soc_table[index].soc_id) {
			msm_id = msm_soc_table[index].msm_id;
			break;
		}
	}
	if(!msm_id) {
		ALOGE("The msm_id for sensor is invalided!");
		return -1;
	}
	return msm_id;
}

static int
_hal_sensors_get_sensors_list( struct sensors_module_t* module, struct sensor_t const** list )
{
  return module->get_sensors_list( (struct sensors_module_t*)poll_device->common.module, list );
}

static int _hal_sensors_open( const struct hw_module_t *module, const char *name,
                              struct hw_device_t **device)
{
  void *qcom_sns_hal_lib_handle;
  int err, msm_id;
  struct sensors_module_t *qcom_sns_module;
  struct sensors_poll_device_t *dev = malloc( sizeof(struct sensors_poll_device_t) );
  if( NULL == dev )
  {
    ALOGE( "%s: Malloc failure", __FUNCTION__ );
    return -1;
  }

  msm_id = get_msm_id();

  switch(msm_id) {
    case MSM_8930:
      qcom_sns_hal_lib_handle = dlopen( QCOM_NATIVE_HAL_LIB_PATH, RTLD_NOW );
      break;
    default :
      qcom_sns_hal_lib_handle = dlopen( QCOM_SNS_HAL_LIB_PATH, RTLD_NOW );
      break;
  }

  if( NULL == qcom_sns_hal_lib_handle )
  {
    ALOGE( "%s: ERROR: Could not open QCOM HAL library", __FUNCTION__ );
    free( dev );
    return -1;
  }

  qcom_sns_module = (struct sensors_module_t *)dlsym( qcom_sns_hal_lib_handle, HAL_MODULE_INFO_SYM_AS_STR );
  if (NULL == qcom_sns_module)
  {
    ALOGE( "%s: ERROR: Could not find symbol %s", __FUNCTION__, HAL_MODULE_INFO_SYM_AS_STR );
    dlclose( qcom_sns_hal_lib_handle );
    free( dev );
    qcom_sns_hal_lib_handle = NULL;
    return -1;
  }

  err = qcom_sns_module->common.methods->open( (struct hw_module_t const*)qcom_sns_module, name, (hw_device_t**)&poll_device );
  if( 0 > err )
  {
    ALOGE( "%s: ERROR: Could not find symbol %s", __FUNCTION__, HAL_MODULE_INFO_SYM_AS_STR );
    dlclose( qcom_sns_hal_lib_handle );
    free( dev );
    qcom_sns_hal_lib_handle = NULL;
    return -1;
  }

  *device = &poll_device->common;

  return 0;
}
