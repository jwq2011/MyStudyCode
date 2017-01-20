#include <fcntl.h>
#include <stdio.h>
#include <linux/ioctl.h>
#include <jni.h>
#include <JNIHelp.h>

#include <utils/Log.h>
#include <android/log.h>
#include "android_runtime/AndroidRuntime.h"
#include "android_runtime/android_view_Surface.h"

#include "Mic.h"
#include <stdlib.h>
#include <string.h>

const char * const kMicClassPathName = "com/autochips/mic/Mic";

//#define SUPPORT_UC20_3G_MODULE	true

#ifdef SUPPORT_UC20_3G_MODULE
//static jboolean com_autochips_mic_Mic_nativeMicSet3GVal(JNIEnv *env, jobject obj, jint iVal)
static jboolean com_autochips_mic_Mic_nativeMicSet3GVal(jint iVal)
{
	int fd;
	//This command is to set MIC gains, which is used to change uplink volume.
	const char *buf_mic = "AT+QMIC=1, 35000,35000\r\n";
	//This command is used to select the volume of the internal loudspeaker of the MT.
	const char *buf_speaker = "AT+CLVL=6\r\n";

	printf("open 3G device\n");
	fd = open("/dev/ttyUSB0", O_RDWR);
	if(fd == -1){
		LOGE("open 3G device failed\r\n");
		printf("open 3G device failed\r\n");
		return JNI_FALSE;
	}
	LOGI("open /dev/ttyUSB0 success! fd=%d\r\n",fd);
	printf("open /dev/ttyUSB0 success! fd=%d\r\n", fd);

	write(fd, buf_mic, BUFSIZ);
	usleep(10000);	//delay 10ms
	write(fd, buf_speaker, BUFSIZ);

    close(fd);
    ALOGI("%s: result", __FUNCTION__);

    return JNI_TRUE;
}

//static jboolean com_autochips_mic_Mic_nativeMicGetVal(JNIEnv *env, jobject obj)
static jboolean com_autochips_mic_Mic_nativeMicGet3GVal(void)
{
	int fd;
	const char *buf = "AT+QMIC?\r\n";

	fd = open("/dev/ttyUSB0", O_RDWR);
	if(fd == -1){
		LOGE("open 3G device failed\r\n");
		return JNI_FALSE;
	}

	write(fd, buf, sizeof(buf));

	close(fd);
    ALOGI("%s: result", __FUNCTION__);

	return JNI_TRUE;
}
#endif

static jboolean com_autochips_mic_Mic_nativeMicSetVal(JNIEnv *env, jobject obj, jint iVal)
{

    int fd;
	jboolean ret=JNI_FALSE;

    fd = open("/dev/mic", O_RDWR | O_NONBLOCK);
    if(fd == -1){
        LOGE("open mic driver failed\r\n");
        return JNI_FALSE;
    }
    LOGI("open dev/mic success! fd=%d\r\n",fd);

   	write(fd,&iVal,sizeof(iVal));
#ifdef SUPPORT_UC20_3G_MODULE
	if(iVal == 0){//3G MIC Enable
		com_autochips_mic_Mic_nativeMicSet3GVal(25000);
	}
#endif

    close(fd);

    ALOGI("%s: result", __FUNCTION__);
    
    return JNI_TRUE;
}


static jboolean com_autochips_mic_Mic_nativeMicGetVal(JNIEnv *env, jobject obj)
{
    jint iVal;

    int fd;
	jboolean ret=JNI_FALSE;

    fd = open("/dev/mic", O_RDWR | O_NONBLOCK);
    if(fd == -1){
        LOGE("open mic driver failed\r\n");
        return JNI_FALSE;
    }
    LOGI("open dev/mic success! fd=%d\r\n",fd);

   	read(fd,&iVal,sizeof(iVal));
#ifdef SUPPORT_UC20_3G_MODULE
	com_autochips_mic_Mic_nativeMicGet3GVal();
#endif
    close(fd);           
	
    ALOGI("%s: result.", __FUNCTION__);
    
    return (iVal==1);
}



static JNINativeMethod sMethods[] = {
	{"nativeMicSetVal", "(I)Z", (void *)com_autochips_mic_Mic_nativeMicSetVal},
	{"nativeMicGetVal", "()Z", (void *)com_autochips_mic_Mic_nativeMicGetVal},
//	{"nativeMicSet3GVal", "(I)Z", (void *)com_autochips_mic_Mic_nativeMicSet3GVal},
//	{"nativeMicGet3GVal", "()Z", (void *)com_autochips_mic_Mic_nativeMicGet3GVal},
};

/*
 * JNI Initialization
 */
extern "C" jint JNI_OnLoad(JavaVM *jvm, void *reserved)
{
    JNIEnv   *env;
    int       status;
    jclass    clazz;

    ALOGI("Mic: loading JNI\n");

    // Check JNI version
    if (jvm->GetEnv((void **)&env, JNI_VERSION_1_6)) {
        ALOGE("JNI version mismatch error");
       return JNI_ERR;
    }
	
	clazz = env->FindClass(kMicClassPathName);
	LOG_FATAL_IF(clazz == NULL, "Unable to find class com.autochips.mic.Mic");

	status = jniRegisterNativeMethods(env, "com/autochips/mic/Mic",
                                      sMethods, NELEM(sMethods));
    if (status < 0)
    {
        return JNI_ERR;
    }

    return JNI_VERSION_1_6;
}

