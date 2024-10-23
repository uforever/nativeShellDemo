#include <jni.h>
#include <string>
#include <android/log.h>

#define LOG_TAG "CustomInfo"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

jstring jstringConcat(JNIEnv *env, jstring str1, jstring str2) {
    // 获取StringBuilder类和构造函数ID
    jclass StringBuilder = env->FindClass("java/lang/StringBuilder");
    jmethodID constructor = env->GetMethodID(StringBuilder, "<init>", "()V");
    jobject stringBuilder = env->NewObject(StringBuilder, constructor);

    // 获取append方法ID
    jmethodID append = env->GetMethodID(StringBuilder, "append", "(Ljava/lang/String;)Ljava/lang/StringBuilder;");

    // 将jstring转换并拼接
    env->CallObjectMethod(stringBuilder, append, str1);
    env->CallObjectMethod(stringBuilder, append, str2);

    // 获取最终结果
    jmethodID toString = env->GetMethodID(StringBuilder, "toString", "()Ljava/lang/String;");
    return (jstring) env->CallObjectMethod(stringBuilder, toString);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_zhang3_outer_OuterApp_attachBaseContext(JNIEnv *env, jobject thiz, jobject base) {
    LOGI("OuterApp.attachBaseContext() called");

    jclass OuterApp = env->GetObjectClass(thiz);
    jclass Application = env->GetSuperclass(OuterApp);
    jclass ContextWrapper = env->GetSuperclass(Application);
    jmethodID attachBaseContext = env->GetMethodID(ContextWrapper, "attachBaseContext", "(Landroid/content/Context;)V");
    env->CallNonvirtualVoidMethod(thiz, ContextWrapper, attachBaseContext, base);

    jmethodID getDir = env->GetMethodID(OuterApp, "getDir", "(Ljava/lang/String;I)Ljava/io/File;");
    jstring dirName = env->NewStringUTF("temp");
    jobject targetDir = env->CallObjectMethod(thiz, getDir, dirName, 0);

    jclass File = env->FindClass("java/io/File");
    jmethodID getAbsolutePath = env->GetMethodID(File, "getAbsolutePath", "()Ljava/lang/String;");
    jstring targetDirString = (jstring) env->CallObjectMethod(targetDir, getAbsolutePath);

    jstring targetPathString = jstringConcat(env, targetDirString, env->NewStringUTF("/encrypted.dex"));

    jmethodID getAssets = env->GetMethodID(OuterApp, "getAssets", "()Landroid/content/res/AssetManager;");
    jobject assetManager = env->CallObjectMethod(thiz, getAssets);

    jclass AssetManager = env->GetObjectClass(assetManager);
    jmethodID open = env->GetMethodID(AssetManager, "open", "(Ljava/lang/String;)Ljava/io/InputStream;");
    jstring inputFileName = env->NewStringUTF("encrypted.png");
    jobject inputStream = env->CallObjectMethod(assetManager, open, inputFileName);

    jclass FileOutputStream = env->FindClass("java/io/FileOutputStream");
    jmethodID initFileOutputStream = env->GetMethodID(FileOutputStream, "<init>", "(Ljava/lang/String;)V");
    jobject outputStream = env->NewObject(FileOutputStream, initFileOutputStream, targetPathString);

    jclass Cipher = env->FindClass("javax/crypto/Cipher");
    jmethodID getInstance = env->GetStaticMethodID(Cipher, "getInstance", "(Ljava/lang/String;)Ljavax/crypto/Cipher;");
    jstring transformation = env->NewStringUTF("DES/ECB/PKCS5Padding");
    jobject cipher = env->CallStaticObjectMethod(Cipher, getInstance, transformation);

    jclass SecretKeySpec = env->FindClass("javax/crypto/spec/SecretKeySpec");
    jmethodID initSecretKeySpec = env->GetMethodID(SecretKeySpec, "<init>", "([BLjava/lang/String;)V");
    jbyteArray keyArray = env->NewByteArray(8);
    jbyte keyBytes[8] = {'P', 'a', '5', 'S', 'W', '0', 'R', 'd'};
    env->SetByteArrayRegion(keyArray, 0, 8, keyBytes);
    jobject secretKey = env->NewObject(SecretKeySpec, initSecretKeySpec, keyArray, env->NewStringUTF("DES"));

    jmethodID init = env->GetMethodID(Cipher, "init", "(ILjava/security/Key;)V");
    env->CallVoidMethod(cipher, init, 2, secretKey);

    jclass InputStream = env->GetObjectClass(inputStream);
    jmethodID read = env->GetMethodID(InputStream, "read", "([B)I");
    jmethodID write = env->GetMethodID(FileOutputStream, "write", "([B)V");

    jbyteArray buffer = env->NewByteArray(64);
    int bytesRead;
    while ((bytesRead = env->CallIntMethod(inputStream, read, buffer)) != -1) {

        jmethodID updateMethod = env->GetMethodID(Cipher, "update", "([BII)[B");
        jobject outputBytes = env->CallObjectMethod(cipher, updateMethod, buffer, 0, bytesRead);

        // 写入输出流
        if (outputBytes) {
            env->CallVoidMethod(outputStream, write, outputBytes);
        }
    }
    jmethodID doFinal = env->GetMethodID(Cipher, "doFinal", "()[B");
    jobject finalBytes = env->CallObjectMethod(cipher, doFinal);
    if (finalBytes) {
        env->CallVoidMethod(outputStream, write, finalBytes);
    }


    jmethodID getApplicationInfo = env->GetMethodID(OuterApp, "getApplicationInfo", "()Landroid/content/pm/ApplicationInfo;");
    jobject applicationInfo = env->CallObjectMethod(thiz, getApplicationInfo);
    jclass ApplicationInfo = env->GetObjectClass(applicationInfo);
    jfieldID nativeLibraryDir = env->GetFieldID(ApplicationInfo, "nativeLibraryDir", "Ljava/lang/String;");
    jstring nativeLibraryDirString = (jstring) env->GetObjectField(applicationInfo, nativeLibraryDir);

    jmethodID getClassLoader = env->GetMethodID(OuterApp, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject classLoader = env->CallObjectMethod(thiz, getClassLoader);

    jclass DexClassLoader = env->FindClass("dalvik/system/DexClassLoader");
    jmethodID initDexClassLoader = env->GetMethodID(DexClassLoader, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/ClassLoader;)V");
    jobject dexClassLoader = env->NewObject(DexClassLoader, initDexClassLoader, targetPathString, targetDirString, nativeLibraryDirString, classLoader);

    jclass ActivityThread = env->FindClass("android/app/ActivityThread");
    jmethodID currentActivityThread = env->GetStaticMethodID(ActivityThread, "currentActivityThread", "()Landroid/app/ActivityThread;");
    jobject activityThread = env->CallStaticObjectMethod(ActivityThread, currentActivityThread);

    jfieldID mPackages = env->GetFieldID(ActivityThread, "mPackages", "Landroid/util/ArrayMap;");
    jobject packages = env->GetObjectField(activityThread, mPackages);

    jmethodID getPackageName = env->GetMethodID(OuterApp, "getPackageName", "()Ljava/lang/String;");
    jstring packageName = (jstring) env->CallObjectMethod(thiz, getPackageName);

    jclass ArrayMap = env->GetObjectClass(packages);
    jmethodID getArrayMap = env->GetMethodID(ArrayMap, "get", "(Ljava/lang/Object;)Ljava/lang/Object;");
    jobject packageWeakReference = env->CallObjectMethod(packages, getArrayMap, packageName);

    jclass WeakReference = env->GetObjectClass(packageWeakReference);
    jmethodID getWeakReference = env->GetMethodID(WeakReference, "get", "()Ljava/lang/Object;");
    jobject package = env->CallObjectMethod(packageWeakReference, getWeakReference);

    jclass LoadedApk = env->GetObjectClass(package);
    jfieldID mClassLoader = env->GetFieldID(LoadedApk, "mClassLoader", "Ljava/lang/ClassLoader;");
    env->SetObjectField(package, mClassLoader, dexClassLoader);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_zhang3_outer_OuterApp_onCreate(JNIEnv *env, jobject thiz) {

    LOGI("OuterApp.onCreate() called");
    jclass OuterApp = env->GetObjectClass(thiz);
    jclass Application = env->GetSuperclass(OuterApp);
    jmethodID onCreate = env->GetMethodID(Application, "onCreate", "()V");
    env->CallNonvirtualVoidMethod(thiz, Application, onCreate);

    jclass ActivityThread = env->FindClass("android/app/ActivityThread");
    jmethodID currentActivityThread = env->GetStaticMethodID(ActivityThread, "currentActivityThread", "()Landroid/app/ActivityThread;");
    jobject activityThread = env->CallStaticObjectMethod(ActivityThread, currentActivityThread);

    jfieldID mBoundApplication = env->GetFieldID(ActivityThread, "mBoundApplication", "Landroid/app/ActivityThread$AppBindData;");
    jobject appBindData = env->GetObjectField(activityThread, mBoundApplication);

    jclass AppBindData = env->GetObjectClass(appBindData);
    jfieldID info = env->GetFieldID(AppBindData, "info", "Landroid/app/LoadedApk;");
    jobject loadedApk = env->GetObjectField(appBindData, info);

    jclass LoadedApk = env->GetObjectClass(loadedApk);
    jfieldID mApplication = env->GetFieldID(LoadedApk, "mApplication", "Landroid/app/Application;");
    env->SetObjectField(loadedApk, mApplication, nullptr);

    jfieldID mApplicationInfo = env->GetFieldID(LoadedApk, "mApplicationInfo", "Landroid/content/pm/ApplicationInfo;");
    jobject applicationInfo = env->GetObjectField(loadedApk, mApplicationInfo);

    jclass ApplicationInfo = env->GetObjectClass(applicationInfo);
    jfieldID className = env->GetFieldID(ApplicationInfo, "className", "Ljava/lang/String;");
    // pay attention to the package name
    jstring appClassName = env->NewStringUTF("com.zhang3.inner.InnerApp");
    env->SetObjectField(applicationInfo, className, appClassName);

    jmethodID makeApplication = env->GetMethodID(LoadedApk, "makeApplication", "(ZLandroid/app/Instrumentation;)Landroid/app/Application;");
    jobject innerApp = env->CallObjectMethod(loadedApk, makeApplication, JNI_FALSE, nullptr);

    jclass InnerApp = env->GetObjectClass(innerApp);
    env->CallVoidMethod(innerApp, env->GetMethodID(InnerApp, "onCreate", "()V"));


    jfieldID mInitialApplication = env->GetFieldID(ActivityThread, "mInitialApplication", "Landroid/app/Application;");
    jobject initialApplication = env->GetObjectField(activityThread, mInitialApplication);

    jfieldID mAllApplications = env->GetFieldID(ActivityThread, "mAllApplications", "Ljava/util/ArrayList;");
    jobject allApplications = env->GetObjectField(activityThread, mAllApplications);

    jclass ArrayList = env->GetObjectClass(allApplications);
    jmethodID remove = env->GetMethodID(ArrayList, "remove", "(Ljava/lang/Object;)Z");
    env->CallBooleanMethod(allApplications, remove, initialApplication);
    env->SetObjectField(activityThread, mInitialApplication, innerApp);
}