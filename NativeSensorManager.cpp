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
#include "NativeSensorManager.h"

ANDROID_SINGLETON_STATIC_INSTANCE(NativeSensorManager);

enum {
	ORIENTATION = 0,
	PSEUDO_GYROSCOPE,
	ROTATION_VECTOR,
	LINEAR_ACCELERATION,
	GRAVITY,
	VIRTUAL_SENSOR_COUNT,
};

const struct sensor_t NativeSensorManager::virtualSensorList [VIRTUAL_SENSOR_COUNT] = {
	[ORIENTATION] = {
		.name = "oem-orientation",
		.vendor = "oem",
		.version = 1,
		.handle = '_dmy',
		.type = SENSOR_TYPE_ORIENTATION,
		.maxRange = 360.0f,
		.resolution = 1.0f/256.0f,
		.power = 1,
		.minDelay = 10000,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
#if defined(SENSORS_DEVICE_API_VERSION_1_3)
		.stringType = NULL,
		.requiredPermission = NULL,
		.maxDelay = 0,
		.flags = 0,
#endif
		.reserved = {},
	},

	[PSEUDO_GYROSCOPE] = {
		.name = "oem-pseudo-gyro",
		.vendor = "oem",
		.version = 1,
		.handle = '_dmy',
		.type = SENSOR_TYPE_GYROSCOPE,
		.maxRange = 50.0f,
		.resolution = 0.01f,
		.power = 1,
		.minDelay = 10000,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
#if defined(SENSORS_DEVICE_API_VERSION_1_3)
		.stringType = NULL,
		.requiredPermission = NULL,
		.maxDelay = 0,
		.flags = 0,
#endif
		.reserved = {},
	},

	[ROTATION_VECTOR] = {
		.name = "oem-rotation-vector",
		.vendor = "oem",
		.version = 1,
		.handle = '_dmy',
		.type = SENSOR_TYPE_ROTATION_VECTOR,
		.maxRange = 1,
		.resolution = 1.0f / (1<<24),
		.power = 1,
		.minDelay = 10000,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
#if defined(SENSORS_DEVICE_API_VERSION_1_3)
		.stringType = NULL,
		.requiredPermission = NULL,
		.maxDelay = 0,
		.flags = 0,
#endif
		.reserved = {},
	},

	[LINEAR_ACCELERATION] = {
		.name = "oem-linear-acceleration",
		.vendor = "oem",
		.version = 1,
		.handle = '_dmy',
		.type = SENSOR_TYPE_LINEAR_ACCELERATION,
		.maxRange = 40.0f,
		.resolution = 0.01f,
		.power = 1,
		.minDelay = 10000,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
#if defined(SENSORS_DEVICE_API_VERSION_1_3)
		.stringType = NULL,
		.requiredPermission = NULL,
		.maxDelay = 0,
		.flags = 0,
#endif
		.reserved = {},
	},

	[GRAVITY] = {
		.name = "oem-gravity",
		.vendor = "oem",
		.version = 1,
		.handle = '_dmy',
		.type = SENSOR_TYPE_GRAVITY,
		.maxRange = 40.0f,
		.resolution = 0.01f,
		.power = 1,
		.minDelay = 10000,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
#if defined(SENSORS_DEVICE_API_VERSION_1_3)
		.stringType = NULL,
		.requiredPermission = NULL,
		.maxDelay = 0,
		.flags = 0,
#endif
		.reserved = {},
	},
};

int NativeSensorManager::initVirtualSensor(struct SensorContext *ctx, int handle, int dep,
		struct sensor_t info)
{
	CalibrationManager *cm = CalibrationManager::defaultCalibrationManager();

	*(ctx->sensor) = info;
	if (cm->getCalAlgo(ctx->sensor) == NULL) {
		return -1;
	}

	ctx->sensor->handle = handle;
	ctx->driver = new VirtualSensor(ctx);
	ctx->data_fd = -1;
	ctx->data_path = NULL;
	ctx->enable_path = NULL;
	ctx->is_virtual = true;
	ctx->dep_mask = dep;

	return 0;
}


const struct SysfsMap NativeSensorManager::node_map[] = {
	{offsetof(struct sensor_t, name), SYSFS_NAME, TYPE_STRING},
	{offsetof(struct sensor_t, vendor), SYSFS_VENDOR, TYPE_STRING},
	{offsetof(struct sensor_t, version), SYSFS_VERSION, TYPE_INTEGER},
	{offsetof(struct sensor_t, type), SYSFS_TYPE, TYPE_INTEGER},
	{offsetof(struct sensor_t, maxRange), SYSFS_MAXRANGE, TYPE_FLOAT},
	{offsetof(struct sensor_t, resolution), SYSFS_RESOLUTION, TYPE_FLOAT},
	{offsetof(struct sensor_t, power), SYSFS_POWER, TYPE_FLOAT},
	{offsetof(struct sensor_t, minDelay), SYSFS_MINDELAY, TYPE_INTEGER},
};

NativeSensorManager::NativeSensorManager():
	mSensorCount(0)
{
	int i;

	memset(sensor_list, 0, sizeof(sensor_list));
	memset(context, 0, sizeof(context));

	for (i = 0; i < MAX_SENSORS; i++) {
		context[i].sensor = &sensor_list[i];
		sensor_list[i].name = context[i].name;
		sensor_list[i].vendor = context[i].vendor;
		list_init(&context[i].listener);
	}

	if(getDataInfo()) {
		ALOGE("Get data info failed\n");
	}

	dump();
}

void NativeSensorManager::dump()
{
	int i;
	struct listnode *node;
	struct SensorRefMap* ref;

	for (i = 0; i < mSensorCount; i++) {
		ALOGI("\nname:%s\ntype:%d\nhandle:%d\ndata_fd=%d\nis_virtual=%d",
				context[i].sensor->name,
				context[i].sensor->type,
				context[i].sensor->handle,
				context[i].data_fd,
				context[i].is_virtual);

		ALOGI("data_path=%s\nenable_path=%s\ndelay_ns:%lld\nenable=%d dep_mask=%lld\n",
				context[i].data_path,
				context[i].enable_path,
				context[i].delay_ns,
				context[i].enable,
				context[i].dep_mask);

		ALOGI("Listener:");
		list_for_each(node, &context[i].listener) {
			ref = node_to_item(node, struct SensorRefMap, list);
			ALOGI("name:%s handle:%d\n", ref->ctx->sensor->name, ref->ctx->sensor->handle);
		}
	}

	ALOGI("\n");
}

const SensorContext* NativeSensorManager::getInfoByFd(int fd) {
	int i;
	struct SensorContext *list;

	for (i = 0; i < mSensorCount; i++) {
		list = &context[i];
		if (fd == list->data_fd)
			return list;
	}

	return NULL;
}

const SensorContext* NativeSensorManager::getInfoByHandle(int handle) {
	int i;
	struct SensorContext *list;

	for (i = 0; i < mSensorCount; i++) {
		list = &context[i];
		if (handle == list->sensor->handle)
			return list;
	}

	return NULL;
}

const SensorContext* NativeSensorManager::getInfoByType(int type) {
	int i;
	struct SensorContext *list;

	for (i = 0; i < mSensorCount; i++) {
		list = &context[i];
		if (type == list->sensor->type)
			return list;
	}

	return NULL;
}

int NativeSensorManager::getDataInfo() {
	struct dirent **namelist;
	char *file;
	char path[PATH_MAX];
	char name[80];
	int nNodes;
	int i, j;
	int fd = -1;
	struct SensorContext *list;
	int has_acc = 0;
	int has_compass = 0;
	int has_gyro = 0;
	int event_count = 0;
	struct sensor_t sensor_mag;

	strlcpy(path, EVENT_PATH, sizeof(path));
	file = path + strlen(EVENT_PATH);
	nNodes = scandir(path, &namelist, 0, alphasort);
	if (nNodes < 0) {
		ALOGE("scan %s failed.(%s)\n", EVENT_PATH, strerror(errno));
		return -1;
	}

	for (event_count = 0, j = 0; (j < nNodes) && (j < MAX_SENSORS); j++) {
		if (namelist[j]->d_type != DT_CHR) {
			continue;
		}

		strlcpy(file, namelist[j]->d_name, sizeof(path) - strlen(EVENT_PATH));

		fd = open(path, O_RDONLY);
		if (fd < 0) {
			ALOGE("open %s failed(%s)", path, strerror(errno));
			continue;
		}

		if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
			name[0] = '\0';
		}

		strlcpy(event_list[event_count].data_name, name, sizeof(event_list[0].data_name));
		strlcpy(event_list[event_count].data_path, path, sizeof(event_list[0].data_path));
		close(fd);
		event_count++;
	}

	for (j = 0; j <nNodes; j++ ) {
		free(namelist[j]);
	}

	free(namelist);

	mSensorCount = getSensorListInner();
	for (i = 0; i < mSensorCount; i++) {
		list = &context[i];
		list->is_virtual = false;
		list->dep_mask |= 1ULL << list->sensor->type;

		/* Initialize data_path and data_fd */
		for (j = 0; (j < event_count) && (j < MAX_SENSORS); j++) {
			if (strcmp(list->sensor->name, event_list[j].data_name) == 0) {
				list->data_path = strdup(event_list[j].data_path);
				break;
			}

			if (strcmp(event_list[j].data_name, type_to_name(list->sensor->type)) == 0) {
				list->data_path = strdup(event_list[j].data_path);
			}
		}

		list->data_fd = open(list->data_path, O_RDONLY);

		switch (list->sensor->type) {
			case SENSOR_TYPE_ACCELEROMETER:
				has_acc = 1;
				list->driver = new AccelSensor(list);
				break;
			case SENSOR_TYPE_MAGNETIC_FIELD:
				has_compass = 1;
				list->driver = new CompassSensor(list);
				sensor_mag = *(list->sensor);
				break;
			case SENSOR_TYPE_PROXIMITY:
				list->driver = new ProximitySensor(list);
				break;
			case SENSOR_TYPE_LIGHT:
				list->driver = new LightSensor(list);
				break;
			case SENSOR_TYPE_GYROSCOPE:
				has_gyro = 1;
				list->driver = new GyroSensor(list);
				break;
			case SENSOR_TYPE_PRESSURE:
				list->driver = new PressureSensor(list);
				break;
			default:
				list->driver = NULL;
				ALOGE("No handle %d for this type sensor!", i);
				break;
		}
	}


	/* Some vendor or the reference design implements some virtual sensors
	 * or pseudo sensors. These sensors are required by some of the applications.
	 * Here we check the CalibratoinManager to decide whether to enable them.
	 */
	CalibrationManager *cm = CalibrationManager::defaultCalibrationManager();
	struct SensorRefMap *ref;

	if ((cm != NULL) && has_compass) {
		/* The uncalibrated magnetic field sensor shares the same vendor/name as the
		 * calibrated one. */
		sensor_mag.type = SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED;
		if (!initVirtualSensor(&context[mSensorCount], SENSORS_HANDLE(mSensorCount),
					1ULL << SENSOR_TYPE_MAGNETIC_FIELD, sensor_mag)) {
			mSensorCount++;
		}
	}

	if ((cm != NULL) && has_acc && has_compass) {
		int dep = (1ULL << SENSOR_TYPE_ACCELEROMETER) | (1ULL << SENSOR_TYPE_MAGNETIC_FIELD);

		/* HAL implemented orientation. Android will replace it for
		 * platform with Gyro with SensorFusion.
		 * The calibration manager will first match "oem-orientation" and
		 * then match "orientation" to select the algorithms. */
		if (!initVirtualSensor(&context[mSensorCount], SENSORS_HANDLE(mSensorCount), dep,
					virtualSensorList[ORIENTATION])) {
			mSensorCount++;
		}

		if (!has_gyro) {
			/* Pseudo gyroscope is a pseudo sensor which implements by accelerometer and
			 * magnetometer. Some sensor vendors provide such implementations. The pseudo
			 * gyroscope sensor is low cost but the performance is worse than the actual
			 * gyroscope. So disable it for the system with actual gyroscope. */
			if (!initVirtualSensor(&context[mSensorCount], SENSORS_HANDLE(mSensorCount), dep,
						virtualSensorList[PSEUDO_GYROSCOPE])) {
				mSensorCount++;
			}

			/* For linear acceleration */
			if (!initVirtualSensor(&context[mSensorCount], SENSORS_HANDLE(mSensorCount), dep,
						virtualSensorList[LINEAR_ACCELERATION])) {
				mSensorCount++;
			}

			/* For rotation vector */
			if (!initVirtualSensor(&context[mSensorCount], SENSORS_HANDLE(mSensorCount), dep,
						virtualSensorList[ROTATION_VECTOR])) {
				mSensorCount++;
			}

			/* For gravity */
			if (!initVirtualSensor(&context[mSensorCount], SENSORS_HANDLE(mSensorCount), dep,
						virtualSensorList[GRAVITY])) {
				mSensorCount++;
			}
		}
	}

	return 0;
}

/* Register a listener on "hw" for "virt".
 * The "hw" specify the actual background sensor type, and "virt" is one kind of virtual sensor.
 * Generally the virtual sensor specified by "virt" can only work when the hardware sensor specified
 * by "hw" is activiated.
 */
int NativeSensorManager::registerListener(struct SensorContext *hw, struct SensorContext *virt)
{
	struct listnode *node;
	struct SensorContext *ctx;
	struct SensorRefMap *item;

	list_for_each(node, &hw->listener) {
		item = node_to_item(node, struct SensorRefMap, list);
		if (item->ctx->sensor->handle == virt->sensor->handle) {
			ALOGE("Already registered as listener for %s:%s\n", hw->sensor->name, virt->sensor->name);
			return -1;
		}
	}

	item = new SensorRefMap;
	item->ctx = virt;

	list_add_tail(&hw->listener, &item->list);

	return 0;
}

/* Remove the virtual sensor listener from the list specified by "hw" */
int NativeSensorManager::unregisterListener(struct SensorContext *hw, struct SensorContext *virt)
{
	struct listnode *node;
	struct SensorContext *ctx;
	struct SensorRefMap *item;

	list_for_each(node, &hw->listener) {
		item = node_to_item(node, struct SensorRefMap, list);
		if (item->ctx == virt) {
			list_remove(&item->list);
			delete item;
			return 0;
		}
	}

	ALOGE("%s is not a listener of %s\n", virt->sensor->name, hw->sensor->name);
	return -1;
}

int NativeSensorManager::getSensorList(const sensor_t **list) {
	*list = mSensorCount ? sensor_list:NULL;

	return mSensorCount;
}

int NativeSensorManager::getNode(char *buf, char *path, const struct SysfsMap *map) {
	char * fret;
	ssize_t len = 0;
	int fd;
	char tmp[SYSFS_MAXLEN];

	if (NULL == buf || NULL == path)
		return -1;

	memset(tmp, 0, sizeof(tmp));

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		ALOGE("open %s failed.(%s)\n", path, strerror(errno));
		return -1;
	}

	len = read(fd, tmp, sizeof(tmp));
	if (len <= 0) {
		ALOGE("read %s failed.(%s)\n", path, strerror(errno));
		close(fd);
		return -1;
	}

	tmp[len - 1] = '\0';

	if (tmp[strlen(tmp) - 1] == '\n')
		tmp[strlen(tmp) - 1] = '\0';

	if (map->type == TYPE_INTEGER) {
		int *p = (int *)(buf + map->offset);
		*p = atoi(tmp);
	} else if (map->type == TYPE_STRING) {
		char **p = (char **)(buf + map->offset);
		strlcpy(*p, tmp, SYSFS_MAXLEN);
	} else if (map->type == TYPE_FLOAT) {
		float *p = (float*)(buf + map->offset);
		*p = atof(tmp);
	}

	close(fd);
	return 0;
}

int NativeSensorManager::getSensorListInner()
{
	int number = 0;
	int err = -1;
	const char *dirname = SYSFS_CLASS;
	char devname[PATH_MAX];
	char *filename;
	char *nodename;
	DIR *dir;
	struct dirent *de;
	struct SensorContext *list;
	unsigned int i;

	dir = opendir(dirname);
	if(dir == NULL) {
		return 0;
	}
	strlcpy(devname, dirname, PATH_MAX);
	filename = devname + strlen(devname);

	while ((de = readdir(dir))) {
		if(de->d_name[0] == '.' &&
			(de->d_name[1] == '\0' ||
				(de->d_name[1] == '.' && de->d_name[2] == '\0')))
			continue;

		list = &context[number];

		strlcpy(filename, de->d_name, PATH_MAX - strlen(SYSFS_CLASS));
		nodename = filename + strlen(de->d_name);
		*nodename++ = '/';

		for (i = 0; i < ARRAY_SIZE(node_map); i++) {
			strlcpy(nodename, node_map[i].node, PATH_MAX - strlen(SYSFS_CLASS) - strlen(de->d_name));
			err = getNode((char*)(list->sensor), devname, &node_map[i]);
			if (err)
				break;
		}

		if (i < ARRAY_SIZE(node_map))
			continue;

		if (!((1ULL << list->sensor->type) & SUPPORTED_SENSORS_TYPE))
			continue;

		/* Setup other information */
		list->sensor->handle = SENSORS_HANDLE(number);
		list->data_path = NULL;

		strlcpy(nodename, "", SYSFS_MAXLEN);
		list->enable_path = strdup(devname);

		number++;
	}
	closedir(dir);
	return number;
}

int NativeSensorManager::activate(int handle, int enable)
{
	const SensorContext *list;
	int index;
	int i;
	int number = getSensorCount();
	int err = 0;
	struct list_node *node;
	struct SensorContext *ctx;

	list = getInfoByHandle(handle);
	if (list == NULL) {
		ALOGE("Invalid handle(%d)", handle);
		return -EINVAL;
	}

	index = list - &context[0];

	for (i = 0; i < number; i++) {
		/* Search for the background sensor for the sensor specified by handle. */
		if (list->dep_mask & (1ULL << context[i].sensor->type)) {
			if (enable) {
				/* Enable the background sensor and register a listener on it. */
				err = context[i].driver->enable(context[i].sensor->handle, 1);
				if (!err) {
					registerListener(&context[i], &context[index]);
				}
			} else {
				/* The background sensor has other listeners, we need
				 * to unregister the current sensor from it and sync the
				 * poll delay settings.
				 */
				if (!list_empty(&context[i].listener)) {
					unregisterListener(&context[i], &context[index]);
					/* We're activiating the hardware sensor itself */
					if ((i == index) && (context[i].enable))
						context[i].enable = 0;
					syncDelay(context[i].sensor->handle);
				}

				/* Disable the background sensor if it doesn't have any listeners. */
				if (list_empty(&context[i].listener)) {
					context[i].driver->enable(context[i].sensor->handle, 0);
				}
			}
		}
	}

	context[index].enable = enable;

	return err;
}

int NativeSensorManager::syncDelay(int handle)
{
	const SensorRefMap *item;
	SensorContext *ctx;
	const SensorContext *list;
	struct listnode *node;
	int64_t min_ns;
	int index;

	list = getInfoByHandle(handle);
	if (list == NULL) {
		ALOGE("Invalid handle(%d)", handle);
		return -EINVAL;
	}

	index = list - &context[0];

	if (list_empty(&list->listener)) {
		min_ns = list->delay_ns;
	} else {
		node = list_head(&list->listener);
		item = node_to_item(node, struct SensorRefMap, list);
		min_ns = item->ctx->delay_ns;

		list_for_each(node, &list->listener) {
			item = node_to_item(node, struct SensorRefMap, list);
			ctx = item->ctx;
			/* To handle some special case that the polling delay is 0. This
			 * may happen if the background sensor is not enabled but the virtual
		         * sensor is enabled case.
			 */
			if (ctx->delay_ns == 0) {
				ALOGW("Listener delay is 0. Fix it to minDelay");
				ctx->delay_ns = ctx->sensor->minDelay;
			}

			if (min_ns > ctx->delay_ns)
				min_ns = ctx->delay_ns;
		}
	}

	if ((context[index].delay_ns != 0) && (context[index].delay_ns < min_ns) &&
			(context[index].enable))
		min_ns = context[index].delay_ns;

	return list->driver->setDelay(list->sensor->handle, min_ns);
}

/* TODO: The polling delay may not correctly set for some special case */
int NativeSensorManager::setDelay(int handle, int64_t ns)
{
	const SensorContext *list;
	int i;
	int number = getSensorCount();
	int index;
	int64_t delay = ns;


	list = getInfoByHandle(handle);
	if (list == NULL) {
		ALOGE("Invalid handle(%d)", handle);
		return -EINVAL;
	}

	index = list - &context[0];
	context[index].delay_ns = delay;

	if (ns < context[index].sensor->minDelay) {
		context[index].delay_ns = context[index].sensor->minDelay;
	}

	if (context[index].delay_ns == 0)
		context[index].delay_ns = 1000000; //  clamped to 1ms

	for (i = 0; i < number; i++) {
		if (list->dep_mask & (1ULL << context[i].sensor->type)) {
			syncDelay(context[i].sensor->handle);
		}
	}

	return 0;
}

int NativeSensorManager::readEvents(int handle, sensors_event_t* data, int count)
{
	const SensorContext *list;
	int i, j;
	int number = getSensorCount();
	int nb;

	list = getInfoByHandle(handle);
	if (list == NULL) {
		ALOGE("Invalid handle(%d)", handle);
		return -EINVAL;
	}
	nb = list->driver->readEvents(data, count);

	/* Need to make some enhancement to use hash search to improve the performance */
	for (j = 0; j < nb; j++) {
		for (i = 0; i < number; i++) {
			if ((context[i].dep_mask & (1ULL << list->sensor->type)) && context[i].enable) {
				context[i].driver->injectEvents(&data[j], 1);
			}
		}
	}

	return nb;
}

int NativeSensorManager::hasPendingEvents(int handle)
{
	const SensorContext *list;

	list = getInfoByHandle(handle);
	if (list == NULL) {
		ALOGE("Invalid handle(%d)", handle);
		return -EINVAL;
	}

	return list->driver->hasPendingEvents();
}

