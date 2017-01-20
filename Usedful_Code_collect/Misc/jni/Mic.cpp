#include <fcntl.h>
#include <stdio.h>
#include <linux/ioctl.h>
#include <jni.h>
#include <JNIHelp.h>

#include <utils/Log.h>
#include "android_runtime/AndroidRuntime.h"
#include "android_runtime/android_view_Surface.h"

#include "Mic.h"

const char * const kMicClassPathName = "com/autochips/mic/Mic";



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

    close(fd);           
	
    ALOGI("%s: result.", __FUNCTION__);
    
    return (iVal==1);
}



static JNINativeMethod sMethods[] = {
	{"nativeMicSetVal", "(I)Z", (void *)com_autochips_mic_Mic_nativeMicSetVal},
	{"nativeMicGetVal", "()Z", (void *)com_autochips_mic_Mic_nativeMicGetVal},
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

