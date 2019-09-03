# WOWQueueCamper
Wakes you up when queue is ready. 

video tutorial here  https://www.youtube.com/watch?v=xKz3pMvtXlY

# Disclaimer
This software is provided as-is - seriously, it was written on a whim at 2 in the morning. If you have an issue using the software please search your issue in "Issues" and create a new issue if no match can be found. This repository provides a pre-built binary for ease of use, please be wary of running any .exe files distributed from an unofficial source, such as a forum.

# Usage
```
Usage: WOWQueueCamper [options]

Options:

  --rleft arg           x value of farthest left pixel on fullscreen.Requires
                        other r parameters, default 1035
                        
  --rbottom arg         y value of farthest bottom pixel on fullscreen.Requires
                        other r parameters, default 492
                        
  --rwidth arg          width of x values of farthest left pixel to farthest
                        right pixel on fullscreen. Requires other r parameters,
                        default 75
                        
  --rheight arg         height of y values of farthest left pixel to farthest
                        right pixel on fullscreen.Requires other r parameters,
                        default 30
                        
  --rheight arg         height of y values of farthest left pixel to farthest
                        right pixel on fullscreen.Requires other r parameters,
                        default 30
  --alarm_number arg    Amount of players in queue to trigger alarm. default
                        1000
                        
  --start_delay arg     Time before running program to allow time to switch to
                        WOW queue in ms. default 10000
                        
  --check_delay arg     Time between checking queue in ms. default 1000
  
  --alarm_file arg      Absolute path to wav file to play for alarm. default
                        fire truck air horn
                        
  -h [ --help ]         Prints help menu
 ```
  
rleft, rbottom, rwidth, and rheight are used together to set a custom rectangle to find the queue number in. This program defaults to 1920x1080 resolution where WOW is fullscreened. I suggest using an image manipulation program, like https://www.gimp.org/ to figure out what rectangle that you need.

The current binary release is https://github.com/greenbrettmichael/WOWQueueCamper/releases/download/0.2/x64-binaries.zip if you simply want to use the tool and get going!

# Compiling From Source
So you want to compile from source huh? Take this handy guide with you!

You will need to install https://cmake.org/ , https://git-scm.com/ , and a windows c++17 compiler supported by cmake like https://visualstudio.microsoft.com/ . 

If you do not have a user-integrated vcpkg, please run ```.\bootstrap.bat``` in either cmd or powershell. This will set up the dependency toolchain. Warning this takes a short while (7 minutes on my machine) and takes up a bit of space (a little under 2 GB). If you DO have a user-integrated vcpkg, install the dependencies mentioned in bootstrap.bat and edit the toolchain path in build.bat (I am assuming if this is the case you are a developer and know how to do this).

Next run ```.\build.bat``` in either cmd or powershell. The package can be used or distributed from WOWQueueCamper/build/Release . You're all done! Pat yourself on the back or add a new issue if this did not work for you.

# Credit
The default.wav soundfile was created by Daniel Simon and can be found here https://soundbible.com/2192-Fire-Truck-Horn.html . 
This program uses these fine libraries: https://github.com/tesseract-ocr/tesseract , https://github.com/DanBloomberg/leptonica , and https://github.com/boostorg/program_options
