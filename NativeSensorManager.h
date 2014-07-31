/*--------------------------------------------------------------------------
Copyright (c) 2014, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--------------------------------------------------------------------------*/

#ifndef SENSOR_INFO_MANAGER_H
#define SENSOR_INFO_MANAGER_H

#include <string.h>
#include <dirent.h>
#include <utils/Log.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <SensorBase.h>

#include <utils/Singleton.h>
#include <cutils/list.h>
#include <sensors.h>

#include "AccelSensor.h"
#include "LightSensor.h"
#include "ProximitySensor.h"
#include "CompassSensor.h"
#include "GyroSensor.h"
#include "PressureSensor.h"
#include "VirtualSensor.h"

using namespace android;

#define EVENT_PATH "/dev/input/"
#define DEPEND_ON(m, t) (m & (1ULL << t))

enum {
	TYPE_STRING = 0,
	TYPE_INTEGER,
	TYPE_FLOAT,
};

struct SensorContext {
	char   name[SYSFS_MAXLEN]; // name of the sensor
	char   vendor[SYSFS_MAXLEN]; // vendor of the sensor
	char   *enable_path; // the control path to enable this sensor
	char   *data_path; // the data path to get sensor events

	struct sensor_t *sensor; // point to the sensor_t structure in the sensor list
	SensorBase     *driver; // point to the sensor driver instance

	int data_fd; // the file descriptor of the data device node
	int enable; // indicate if the sensor is enabled
	bool is_virtual; // indicate if this is a virtual sensor
	int64_t delay_ns; // the poll delay setting of this sensor
	uint64_t dep_mask; // the background sensor type needed for this sensor

	struct listnode listener; // the head of listeners of this sensor
};

struct SensorEventMap {
      char data_name[80];
      char data_path[PATH_MAX];
};

struct SysfsMap {
	int offset;
	const char *node;
	int type;
};

/* To contain the listener list and denpend list */
struct SensorRefMap {
	struct listnode list;
	struct SensorContext *ctx;
};

class NativeSensorManager : public Singleton<NativeSensorManager> {
	friend class Singleton<NativeSensorManager>;
	NativeSensorManager();
	struct sensor_t sensor_list[MAX_SENSORS];
	struct SensorContext context[MAX_SENSORS];
	struct SensorEventMap event_list[MAX_SENSORS];
	static const struct SysfsMap node_map[];
	static const struct sensor_t virtualSensorList[];

	int mSensorCount;
	int getNode(char *buf, char *path, const struct SysfsMap *map);
	int getSensorListInner();
	int getDataInfo();
	int registerListener(struct SensorContext *hw, struct SensorContext *virt);
	int unregisterListener(struct SensorContext *hw, struct SensorContext *virt);
	int syncDelay(int handle);
	int initVirtualSensor(struct SensorContext *ctx, int handle, int dep, struct sensor_t info);
public:
	int getSensorList(const sensor_t **list);
	const SensorContext* getInfoByFd(int fd);
	const SensorContext* getInfoByHandle(int handle);
	const SensorContext* getInfoByType(int type);
	int getSensorCount() {return mSensorCount;}
	void dump();
	int hasPendingEvents(int handle);
	int activate(int handle, int enable);
	int setDelay(int handle, int64_t ns);
	int readEvents(int handle, sensors_event_t *data, int count);
};

#endif

