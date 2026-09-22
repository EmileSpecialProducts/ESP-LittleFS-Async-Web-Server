Import("env")
import os
import shutil
from shutil import copyfile

def PostfactoryBuild(source, target, env):
    print("--------- Save firmware.factory.bin before upload -------------------")
    import datetime
    now = datetime.datetime.now()
    print(now)
    from time import gmtime, strftime
    print(strftime("%Y-%m-%d %H:%M:%S GMT", gmtime()))
    pioenv=env.get("PIOENV")
    sourcedir = env.get("PROJECT_BUILD_DIR") + "/" + pioenv
    destination = os.getcwd() + "/firmware" 
    if not os.path.exists(destination):
        os.mkdir(destination)
    destination = (destination + "/" +pioenv ) 
    if not os.path.exists(destination):
        os.mkdir(destination)
    print("destination = " + destination)
    board_mcu = env.get("BOARD_MCU")
    if board_mcu == "esp8266":
        print("CPU is esp8266 " , sourcedir + "/firmware.bin" )
        if os.path.exists(sourcedir + "/firmware.bin"):
            shutil.copyfile(sourcedir + "/firmware.bin", destination + "/firmware.bin")
        else:
            print("Not Found " , sourcedir + "/firmware.bin" )
    else:
        print("CPU is esp32 " , sourcedir + "/firmware.factory.bin" )
        if os.path.exists(sourcedir + "/firmware.factory.bin"):
            shutil.copyfile(sourcedir + "/firmware.factory.bin", destination + "/firmware.factory.bin")
        else:
            print("Not Found " , sourcedir + "/firmware.factory.bin" )
    if os.path.exists(sourcedir + "/littlefs.bin"):
        print("Copying " , sourcedir + "/littlefs.bin" )
        shutil.copyfile(sourcedir + "/littlefs.bin", destination + "/littlefs.bin")
    else:
        print("Not Found " , sourcedir + "/littlefs.bin" )

pioenv=env.get("PIOENV")
source = env.get("PROJECT_BUILD_DIR") + "/" + pioenv
board_mcu = env.get("BOARD_MCU")
print("source ",source)
print("board_mcu",board_mcu)
env.AddPostAction(source+"/littlefs.bin", PostfactoryBuild)
if board_mcu == "esp8266":
    print("AddPostAction = " + source+"/firmware.bin")
    env.AddPostAction(source+"/firmware.bin", PostfactoryBuild)
else:
    print("AddPostAction = " + source+"/firmware.factory.bin")
    # It will not trigger on the firmware.factory.bin so trigger on the firmware.bin 
    env.AddPostAction(source+"/firmware.bin", PostfactoryBuild)

def mytarget_callback(*args, **kwargs):
    print("mytarget_callback")
    pioenv=env.get("PIOENV")
    source = env.get("PROJECT_BUILD_DIR") + "/" + pioenv
    board_mcu = env.get("BOARD_MCU")
    print("pioenv ",pioenv )
    env.Execute("pio run -e " + pioenv +" -t buildfs")
    env.Execute("pio run -e " + pioenv +" -t upload") # using Upload as build will not build the firmware

print( "Adding the target: all")
env.AddCustomTarget(
    name="all",
    dependencies=None,
    actions=mytarget_callback,
    title="Build firmware.factory.bin and littlefs.bin",
    description="This will Build firmware.factory.bin and littlefs.bin"
)
