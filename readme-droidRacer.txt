/*
 * @author Pallavi Maiya
 */

Note: 
*use a machine with atleast 16 threads and atleast 8GB RAM for 
faster builds (first build takes nearly 25 mins, and incremental builds between 
2-6 minutes), otherwise may take hours based on resources provided.

* DroidRacer has only been tested on Ubuntu 10.04 
If Android source installation related requirements are satisfied, DroidRacer may
work on later versions of Ubuntu too.

reference paper to understand the race detection theory and technique: 
"Race Detection for Android Applications", PLDI 2014.

Downloading Android source
---------------------------
1. download android-4.0.1_r1 (this is the version modified to instrument 
   DroidRacer) from http://source.android.com/source/initializing.html
   All instructions are available on the website. This version seems to work 
   on Debian 6 too. Follow link http://www.techsomnia.net/2012/07/building-android-on-debian-6/


Compiling Android source 
------------------------
** if you encounter error when building android from source check if the solution is listed below in error section **

1. Build Android only after performing all environment initializations as described in 
   Android source site.

   For building source do the following from android-source-root
   $. build/envsetup.sh
   $lunch full-eng
   $make -j<n>  #n depends on the number of hardware threads your machine has
   $emulator  #launches android emulator

   -> To set proxy, increase data partition size, RAM size and specify sdcard path 
   issue command similar to:

   $emulator -http-proxy http://<username>:<password>@<domain>:<port> 
    -partition-size 2047 -memory 2048 -sdcard <path-to-sdcard>/sdcard.img



Some errors you may see on building android
-------------------------------------------
1. If build fails, firstly check if you have installed gcc-4.4, g++-4.4, g++-4.4-multilib on your system
   Some errors can be resolved by installing these packages alongside your existing gcc or g++ versions.
   E.g., $sudo apt-get install gcc-4.4     #if you are on ubuntu
  
   Then make your environment point to this version of gcc and g++ by giving following commands in the 
   terminal where you are building android. If you want this environment to be reflected always add these
   commands to your ~/.bashrc file instead of setting in terminal.
   $export CC=gcc-4.4
   $export CXX=g++-4.4

2. If you see the following error:
   "dalvik/vm/native/dalvik_system_Zygote.cpp:193: error: aggregate ‘rlimit rlim’ has incomplete type and cannot be defined"

   then, open dalvik/vm/native/dalvik_system_Zygote.cpp and add the following INCLUDE sirective to this cpp file
   #include <sys/resource.h>   

3. Add the following line if XInitThreads undefined error is obtained during make: 

   add-line-to-file: /development/tools/emulator/opengl/host/renderer/Android.mk 
   in android source root: 
   LOCAL_LDLIBS += -lX11 
 


About droidRacer repository
----------------------------
droidRacer repository has several branches. To know which branch has
the latest code view the information displayed in 
https://bitbucket.org/hppulse/droidracer/branches . 
To reproduce results corresponding to our paper at PLDI 2014 clone the
droidracer repository from https://hppulse@bitbucket.org/hppulse/droidracer.git

Switch to branch pldi-2014 and follow instructions given on this page.



DroidRacer related modifications and initializations
----------------------------------------------------
1. Make sure your android source builds and runs fine. 
   Then, replace the original files with the corresponding files provided 
   in droidRacer repository on bitbucket (https://bitbucket.org/hppulse/droidracer). 
   When there is no corresponding file/folder in the original source, add it 
   from repository.

2. Goto dalvik/vm/mterp and run $./rebuild.sh

3. Run the following commands from android source root
   $make update-api   #needed as the replaced files modify some Java APIs
   $make -j<n>  #sometimes you might have to issue this twice for make to complete
   $emulator

4. When the emulator is running issue command:
   $adb shell "echo dalvik.vm.execution-mode = int:portable >> /data/local.prop"

   #this shifts emulator's interpreter from assembly (fast) to C mode..some of
   DroidRacer instrumentation is on the C interpreter.

   #adb tool is distributed as part of Android SDK. Download the entire Android
   Developer tool bundle from http://developer.android.com/sdk/index.html

   # ***NOTE****
   If using the above command does not change the mode to interpreter mode after restarting
   the emulator, then do the following:

   $adb shell stop
   $adb shell setprop dalvik.vm.execution-mode int:portable 
   $adb shell start

5. Create sdcard for the emulator using mksdcard tool provided with android SDK. E.g.,
   $/mksdcard 1024M <path-to-android-source-root>/sdcard.img
   To use this sdcard pass it as an argument when running emulator from android-source-root:
   $emulator -sdcard sdcard.img

   If you want to avoid passing argument each time create sdcard in following location:
   $/mksdcard 1024M <path-to-android-source-root>/out/target/product/generic/sdcard.img
   
   But with the second option the problem is that sdcard.img is erased in case of 
   "make clean" unlike the first option.

6. Restart emulator

7. Install AbcClientApp on the emulator from droidRacer-related-files repo 
   (https://bitbucket.org/hppulse/droidracer-related-files)

8. Create or push a file (using adb tool) called abc.txt to /mnt/sdcard/Download 
   on the running emulator. File abc.txt should have the following format:

   line 1:process name of app. This is given in manifest file, can be seen when process 
   name is displayed by Eclipse IDE DDMS interface when app is run on emulator; 
   or can be seen in the AndroidManifest.xml extracted as a .txt file using 
   APKParser tool. Download this tool from droidracer-related-files repository.
   usage:$java -jar APKParser.jar APK_FILE_NAME >> TO_SOME_FILE

   line 2: fully qualified package name of app in L format 

   line 3: a class belonging to application package. 
   E.g., fully qualified name of starting Activity obtained from manifest file 
   extracted using APKParser

   line 4: depth of UI events to be generated by UI explorer of droidRacer

   line 5: specify a positive number or 0.
   delays the first event triggered on the test app, proportionate to the positive integer 
   specified. This will be like a waiting period only before first event is triggered 
   and not for each event. Through trial and error get a useful value. Usually 0
   works for most but for a few this may result in BACK press even before screen 
   is loaded. This happens when control reaches DroidRacer's UI event trigger code
   too soon. BACK press gets triggered as none of the other events are active yet.

   line 6: port to communicate with emulator (you can specify any unused port number here)

   line 7: specify a positive integer as limit if the trace generation has to be 
   truncated on hitting the limit. In that case race detection is performed on 
   truncated trace. Trace seen may shoot up the limit by a couple of operations as
   coordination has to be brought between multiple threads to stop trace generation.
   specified number of events will still be triggered before the test run exits.

   E.g:
   com.android.music
   Lcom/android/music/
   com.android.music.MusicBrowserActivity
   4
   0
   9997
   3000

** sample abc.txt can be found on https://bitbucket.org/hppulse/droidracer-related-files
   for each of the app tested by us. Check folder "pldi-2014-tested-apps" in the
   droidracer-related-files repository source root.

** Make sure not to leave any trailing blank space or blank lines in abc.txt. Check
   examples provided in droidracer-related-files repository.


9. Install google services apk (com.google.android.gms.apk) and google play
   apk (com.android.vending.apk) only if you want to test apps those need google 
   services and google play to be installed. These apps may be downloaded from 
   https://play.google.com/ using a suitable plugin to download apk or through
   some sites which make apks available. Uninstall them when testing other apps 
   to avoid unnecessary logs.

10. Download ModelCheckingServer project from https://bitbucket.org/hppulse/droidracer-related-files
   and import it on your eclipse.
   Use ModelCheckingServer to start testing apps using DroidRacer. ModelCheckingServer acts as a 
   server with which the emulator communicates after each testing run. The server performs 
   initializations for each run as specified in abc.sh

   * Inside this project you will find a few script files and src/ABCServer.java . 
   In each of these files change the paths leading to android tools or DroidRacer 
   files, with the path on your machine. Pay care when doing this because not
   changing path for some may not lead to tool crash but may not do anything
   meaningful.

   * backupDatabase folder inside ModelCheckingServer stores the database (.db and .db-journal) 
   file corresponding to each testing run (UI explorer needs this).
   It also stores trace generated (abc_log.txt) for race detection in each run. 
   A sample trace can be found inside ModelCheckingServer.
   Apart from operations, method calls by each tracked thread are logged. 
   Trace along with call stacks are useful in analysing the reported races and 
   also to identify the location of race in the source code. Class/object fields 
   on which races are reported are listed at the end of abc_log.txt files. For 
   each race reported on object-field a pair of memory access operations involved 
   in race are reported as witness in a block above the list of race categories.

   Please refere to our PLDI 2014 paper to know more about the kind and category 
   of races reported.

   * A .db file from any run can be copied back to /data/data/<test-app-process-name>/databases/ 
   folder to continue run from the corresponding exploration run. 
   (The UI explorer backtracks after each run like a model-checker)

   * Summary of each run (races detected, trace length, time taken etc.) are 
   appended to pldi-race.log file in the root of ModelCheckingServer folder.

11. The number of testing runs to be performed needs to be specified in field: traceLimit of 
    ABCServer.java in ModelCheckingServer project.



How to run DroidRacer
---------------------
1. Install the app to be tested and force-stop it before starting ModelCheckingServer outside emulator. 
   ABCServer.java (main class in ModelCheckingServer project) takes three arguments:
   (1) app-process-name to be tested as input,  (2) port to communicate with emulator
   (3) emulator ID (its a number displayed on top of the emulator to identify it in
   case multiple emulators are running simultaneously) 
   E.g., org.tomdroid 9998 5554  #to test Tomdroid app running on emulator-5554
   Also, specify the same port number as specified in abc.txt

2. Before starting DroidRacer run through ModelCheckingServer add 
   <app-process-name, app's main Activity> to intentAppMap HashMap in ABCServer.java inside
   ModelCheckingServer/src . Samples for this can be found in ABCServer.java

3. Make sure configuration file abc.txt is copied to /mnt/sdcard/Download folder of emulator
   and AbcClientApp app is installed. This app is needed to communicate with the 
   ModelCheckingServer.

4. To inspect the trace file abc_log.txt in the middle of testing run issue a pull using adb tool
   from the location /data/data/<process-name-of-app-under-test>/abc_log.txt

5. If you want to only generate trace and not perform race detection then search and comment out 
   Thread.currentThread().abcPrintRacesDetectedToFile();				
   Thread.currentThread().abcComputeMemoryUsedByRaceDetector();
   Thread.currentThread().abcPrintRacesDetectedToFile();
   lines from /frameworks/base/core/java/android/os/ModelCheckingDriver.java and re-build and run

6. Some apps may need initializations like one time logins, accepting an agreement etc.
   You may want to test the app after these initializations and may not want to repeat
   these non-interesting aspects in each run. If the app has the capability to remember
   these initializations even after the app is Force-Quit and restarted, DroidRacer can
   help you avoid these and perform initialization only before the first test run on app.
   For this, comment out lines 12,13,14,15 in abc.sh of ModelCheckingServer project.
   This prevents clearing all data under /data/data/<app-package> and thus does not 
   start testing from scratch across runs.

   Be careful when doing this; in case of some apps this may restart app from a 
   different screen in each run making DroidRacer runs go bad after a few runs 
   due to inconsistencies in initial paths across runs.

7. During the run a few logs get printed on the screen and also stored to 
   abc_log.txt. But during race detection stage DroidRacer does not report any 
   progress till it is completed. The race detection time can vary anywhere
   between a couple of seconds to hours. Do not terminate the emulator in this stage.

8. During a DroidRacer run (before Race Detection phase starts) if 
   "Application Not Responding" dialog is shown click on the "WAIT" button
   so that app is not killed. ANR gets displayed as DroidRacer does file read-write
   from main thread in some places.

9. Read readme.txt in https://bitbucket.org/hppulse/droidracer-related-files
   to reproduce the runs corresponding to results reported in 
   "Race Detection for Android Applications", PLDI 2014 paper.

10. When DroidRacer is running, sometimes UI thread may get blocked for long resulting 
    in ANR (Application Not Responding) dialog to appear on the screen. In such cases
    press on WAIT button in the dialog. This will not affect app exploration or race
    detection. But dialog being on the screen blocks app's UI halting UI exploration.



Understanding DroidRacer implementation
---------------------------------------
1. DroidRacer code is distributed inside /dalvik, /frameworks/base folder and 
   Thread.java, Timer.java and VMThread.java files in /libcore/luni/src/main/java
   directory. We use Thread.java and VMThread.java as interfaces to pass control
   to the native code from Java code.

   frameworks/base/core/java/android/os/ModelCheckingDriver.java is the main 
   java file. In C++ /dalvik/mcd/abc.cpp is the main file for DroidRacer.

   To understand the source code grep for "Android bug-checker", "mcd" inside 
   files modified to implement DroidRacer (basically, files uploaded on 
   droidRacer repository https://bitbucket.org/hppulse/droidracer .

   Method androidBugChecker() inside ModelCheckingDriver.java is an important 
   function for UI explorer. For race detector the set of important functions 
   are listed inside /dalvik/mcd/abc.cpp . Initializations for testing happens 
   inside /dalvik/vm/native/java_lang_reflect_Method.cpp (search for Android bug-checker).
   
   Android component modelling related code is mainly present in 
   /dalvik/vm/mcd/AbcModel.cpp and the instrumentation for it is distributed 
   accross Android Java files like ActivityThread.java, Activity.java,
   ContextImpl.java and so on. To get an idea of the kind of lifecycle actions 
   modeled look at a list of static constants listed in 
   /frameworks/base/core/java/android/os/AbcGlobal.java. This is of use only
   if you know about Android components and their lifecycle.

   Comments in running text can be found for most of the DroidRacer related methods.


2. Data inputs for UI events are initialized in 
   frameworks/base/core/java/android/os/ModelCheckingDriver.java . You can give data
   input of your choice by modifying text in a bunch of methods like 
   initializeTextEmail(), initializeTextPassword() etc.

3. The priority in which UI events are triggered on a screen are specified in 
   method initUIEventPriority() in ModelCheckingDriver.java . Higher the integer
   set for McdDB.COLUMN_EVENT_PRIORITY higher is the priority of the event.
   Priority for BACK press, Menu press and Screen Orientation change is given
   on lines : 5000, 5001, 5002 in branch pldi-2014
   addEventToUnexploredList(BACK_CLICK_EVENT_ID, UI_EVENT, pathNodeID, KEY_PRESS_EVENT_PRIORITY, database);
   addEventToUnexploredList(MENU_CLICK_EVENT_ID, UI_EVENT, pathNodeID, 15, database);
   addEventToUnexploredList(ROTATE_SCREEN_EVENT_ID, UI_EVENT, pathNodeID, KEY_PRESS_EVENT_PRIORITY, database);

   You can modify priority of events to guide the UI exploration in a particular direction.
   E.g., Click Menu button as soon as a new Activity is entered and so on...
   By default higher priority is given to data input events, so that data
   is supplied before navigating from one screen to another.



---------------------------------------------------------------------------------

* Visit http://www.iisc-seal.net/droidracer to know more about DroidRacer.

* For any further queries drop an e-mail to 
pallavi <dot> maiya <at> csa <dot> iisc <dot> ernet <dot> in
    
