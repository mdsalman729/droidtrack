# Dynamic Race Detection tool for Android applications 
#                                                 

This repository is to run our dynamic race detection tool which uses DroidRacer for the front end code instrumentation and implements the fast track algorithm as the back end tool
to carry out dynamic race detection. We have also proposed a few optimizations to reduce the search space in case of the various vector clock reusage rules.

## User Guide
## 

Note- 
The initial steps for downloading the Android source, compiling the Android source and patching the Android source with the Droid instrumentation is mentioned in the readme file of the Droidracer tool and the readme is included as a separate file in this repository. The steps to get the system up and running are the same and we have added files related to the backend race detection in the repository.

1. The race detection tool is in the folder dalvik/vm/rdt/droidtrack.cpp. A majority of the changes are in the dalvik/vm folder. 
2. To run the tool go to the Android top directory ( in my case- /home/salman_workspace/android_backup_droid/android). 
3. If you need to compile the source code before running, then run the makerun.sh script which is located in the top directory.
4. Once the source code has been compiled, you should be able to see the emulator command in the Android top directory. This command is exported by the build process and lets you    run the emulator. Sometimes, the build process has to be repeated to ensure the emulator command is exported. In this cases, just run the script again and it should work.

![mpp.png](https://bitbucket.org/repo/g6K6L4/images/1600596138-mpp.png)
 
5. To run the emulator use the command – 
                                      
```
#!shell
emulator  –partition-size 2047 –memory 2048  –sdcard   sdcard.img

```

6. To test an Android application using the Droid Racer tool, we need to place an application specific file within the Downloads folder of the sdcard.( Refer to the readme-droidRacer.txt file). This file called abc.txt is specifies a few important parameters- 
name of the application in L format.
Main or the starting activity of the application which can be found from the manifest file. In case only the apk is available, then we can extract it using the apk parser tool which is provided in the droidracer tool.
Specimen value of the contents in abc.txt
* com.android.music
* Lcom/android/music/
* com.android.music.MusicBrowserActivity
* 4
* 0
* 9997
* 3000
We need to push the file to the sdcard on the emulator which is done using the command-
 adb push /mnt/sdcard/Download/abc.txt
7. To run the tool, run events on the application such as menu and button clicks.  The events are logged using the adb  logcat command.  Before starting the application, run the adb logcat command and  redirect the output to a txt file using the command-

```
#!shell
adb logcat > a.txt

```
                                      
8. To get the results, we need to run the reuse_script.sh which gives the number of vector clocks reused, the composition of the clocks reused due to the fifo rule and the atom rule. 
9. To trace the data race condition, pull the abc_log.txt which is provided by the droid racer tool using the command-

```
#!shell
 adb pull /data/data/<process-name-of-app-under-test>/abc_log.txt

```
                               
For e.g. , for the tomdroid app we could use the command-

```
#!shell
adb pull /data/data/org.tomdroid/abc_log.txt

```
                               
10. To get the results corresponding to the number of vector clocks reused, use the reuse.sh script which gives information regarding the number of vector clocks reused, the contribution due to each of rules, fifo, atom. 
11. To increase the size of the vector clock, change the parameter TSK_MAX which is a parameter specified in the droidtrack.h file.


## To-do-list 
##
1. Fix the implementation related to the offline analysis tool in droid racer. Our implementation does not need that tool as the race detection is done online but a comparison with droid racer can necessitate that portion to work.
2. The UI explorer tool specified in the documentation has been disabled and the effect of the UI explorer has not been evaluated. This could be used as a future tool.
3. The optimizations have only been carried out for the post messages with delay and the post messages on the same thread (bookkeeping optimization). Further optimizations for the atom rule and the send at front messages can be carried out.
4. A thorough evaluation of the tool for race detection has not been carried out in terms of characterizing the races as cross-posted .etc like the droid racer tool does.
5. Presently, the happens before ordering between the enable and post events has been identified. We need to modify the post message in the handler.java code to send the object reference along with the posted message which can then be utilized in our backend tool to obtain a relation between the enable and post.  The implementation is not complete and we need to analyze the feasibility of such a modification.


---------------------------------------------------------------------------------------------------------
   
**In case of any queries regarding the setup and execution, reach out to mdsalman@vt.edu/md.salman729@gmail.com**