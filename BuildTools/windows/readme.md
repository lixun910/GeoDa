*****************************************************************
Build Instructions for GeoDa.  Current as of GeoDa 1.8.x
*****************************************************************

Overview: We assume the build machine hosts a recently-installed
clean OS.  This build guide contains notes on setting up the compile
environment, obtaining the GeoDa source files and dependent libraries,
compiling libraries and GeoDa, and finally packaging the program
for distribution and installation.

***************************************************************
Building GeoDa for 32-bit and 64-bit Windows 7 or later
***************************************************************

1. Build machine assumptions:

         -- clean Windows 7 32-bit installation for building Windows 32-bit GeoDa
         -- clean Windows 7 64-bit installation for building Windows 32-bit GeoDa
         -- all OS updates applied

2. Install Visual Studio 2010 (VS)

         We assume Visual Studio C++ Express 2010
         For 64-bit Windows only (and VS Express), you must also
         install Microsoft Windows SDK for Windows 7:
         http://www.microsoft.com/en-us/download/details.aspx?id=8279

         Note: follow these instructions exactly to correctly patch Win 7 SDK.
         This was originally from: http://forum.celestialmatters.org/viewtopic.php?t=404

         Here is a little VS2010 blog for those of you who do programming work (Celestia!,...) using the free
         MS VC++ 2010 Express compiler IDE. Quite recently, a SP1 release became available for the VS2010 compilers that is worth implementing. But some care is required during the installation:


    Let's proceed step-by-step starting with the canonical installation of VS2010 Express:


         2.1. Install Visual Studio 2010 Express
         https://www.visualstudio.com/products/visual-studio-express-vs

         2.2. Install Windows SDK 7.1
         https://www.microsoft.com/en-us/download/details.aspx?id=8279

         Unlike the earlier VS2008 Express, 1) + 2) provided a pretty workable compiler environment for BOTH X64 and X86 settings during the past year or so. Yet, a number of remaining quirks led to the said SP1 for all Visual Studio 2010 compiler versions.

         2.3 Install Visual Studio 2010 SP1 (with VS2010 being already installed!):
         https://www.microsoft.com/en-us/download/details.aspx?id=23691

         The release date of the SP1 pack is March 3rd 2011.
         That should have been it, however...

         An issue has been discovered, where Visual C++ Compilers and libraries that are installed with the Windows SDK 7.1 are removed when Visual Studio 2010 Service Pack 1 is installed. This notably refers to the x64 part. Fortunately, on March 30th 2011 an official update to the SDK 7.1 became available that fixes the problem.

         2.4 Install Visual C++ 2010 SP1 Compiler Update for the Windows SDK 7.1
         https://www.microsoft.com/en-us/download/details.aspx?id=4422

         Note: the sequence of installs is unfortunately quite crucial! The recommended one is
         1), 2), 3), 4). PLEASE do read about errors that may occur otherwise in the readme that accompanies the fix 4)!


         2.5 Install Inno
         2.6 Install cmake for windows with command line available.

3. Use Git to check out GeoDa repository: https://github.com/GeoDaCenter/geoda.git

         You can download and use Github Desktop app for Windows https://desktop.github.com/
 
4. Choose "Visual Studio Command Prompt (2010)" from the
   Visual Studio 2010 Express program folder.
   
         cd to C:\path-to-geoda-repo\BuildTools\windows
 
5. Run the correct build script.

         For 32-bit Windows, run build.bat
         For 64-bit Windows, run build64.bat
         Note: The script should do the following:
                  * set up the compile environment for 32-bit compilation
                  * download the source for each library GeoDa depends on and compile and install them
                  * build GeoDa 32-bit (64-bit) with VS against the GeoDa.vs2010.sln solution file
                  * the final output should be an executable called GeoDa.exe

         NOTE: the boost library may not be compiled automatically. In this case,
         try to manually enter the command next to bootstrap.bat in build.bat/build64.bat file.

    (Optional) to build OGR plugins, you need to manually copy the file
    dep/gdal-1.9.2/nmake.opt.withplugins to temp/gdal-1.9.2/,
    then manually enter and execute the commands between "go noplugin" and ":noplugin" section.
 
         5.1. For FileGDB plugin, download ESRI File geodatabase API 1.3 from http://www.esri.com/apps/products/download/
                  Extrac this zip file to C:\FileGDB
                  cd %LIB_NAME%\ogr\ogrsf_frmts\filegdb
                  nmake -f makefile.vc plugin
                  
         You can find the DLL in current directory 
         Note: if there is an error about PGtableResultLayer, try to re-download the gdal-1.9.2 from Gitub
         
         5.2. For OCI plugin, download Oracle instant client 11.2 from 
         http://www.oracle.com/technetwork/topics/winsoft-085727.html
         Extract this file to C:\oracle_base\ohome
         
                  cd %LIB_NAME%\ogr\ogrsf_frmts\oci
                  nmake -f makefile.vc ogr_OCI.dll
         
         
         5.3. For Arc SDE plugin, try to install ArcSDE SDK from ESRI on your machine, 
         you will find the SDK at C:\Program Files\ArcGIS\ArcSDE
                  
                  cd %LIB_NAME%\ogr\ogrsf_frmts\sde
                  nmake -f makefile.vc ogr_SDE.dll
                  
         The built DLLs also need to be manually copied to Debug/Release direcotry for testing 
          , or installer/64bit or installer/32bit directory for packaging installer.

         --GeoDa.exe should be compiled when run build.bat or build64.bat. You can find it at Release/ directory.
         If GeoDa is failed to build using build.bat, try to open GeoDa.vs2010.sln
         in visual studio, and select release/Win32 or release/x64, and select build. 


6. Package GeoDa for distribution / installation.

         --Make sure you have gdal19.dll and GeoDa.exe successfully compiled.
         --copy the following DLLs into installer/32bit or installer/64bit directory:
                  gdal19.dll
                  GeoDa.exe
         --If you want latest dependent libraries and plugins, also copy these DLLs to the same directory
                  boost_chrono-vc100-mt-1_54.dll 
                  boost_system-vc100-mt-1_54.dll
                  boost_thread-vc100-mt-1_54.dll
                  libeay32.dll
                  libintl.dll
                  libpq.dll
                  ssleay32.dll
                  wxmsw30u_gl_vc_custom.dll
                  wxmsw30u_vc_custom.dll
                  ogr_FileGDB.dll
                  ogr_OCI.dll
                  ogr_SDE.dll
         --make sure the data/ directory exist, otherwise copy it from temp/gdal-1.9.2/data 
         --make sure the SampleData directory exist, and the items described in GeoDa.iss exist inside this directory 
         --double click GeoDa.iss (You need to install Inno Setup Compiler http://www.jrsoftware.org/isdl.php), 
         click run icon. You will find a setup.exe in your local "Document\Inno Setup Examples Output"  directory

***************************************************************
About ArcSDE plugin 32-bit windows
***************************************************************
From the 10.1 deprecation doc…
http://downloads2.esri.com/support/TechArticles/ArcGIS10and101Deprecation_Plan.pdf
 
[Added 8/18/2011]
ArcGIS Server 10.1’s ArcSDE technology component will no longer support 32-bit versions of the SDE command line utilities or 32-bit versions of the C and Java APIs. With the migration of ArcGIS Server to be 64-bit, the SDE command line utilities and the ArcSDE SDK will be 64-bit only.


##############################
# For visual studio 2017 Community
##############################

1. build geos from source

comment the line that requires "ntwin32.mak" in makefile.vc
The nmake command will complains "max()" function is not defined: go to the source and add #include <algorithm>

nmake.opt 
```
#!INCLUDE <ntwin32.mak>
```

```
GEOS_MSVC = 14.0
GEOS_MSC = 1900
```

2.  build freexl from source

nmake will complains round() and lround() "definition of dllimport function not allowed": go to source and comment out the two functions.

3. build sqlite from source

open the project using visual studio 2017;

4. build spatialite from source

nmake will complains rint() error: edit source and comment out the function

5. build CLAPACK from source

use visual studio 2017 to open the project

6. build expat from source

use expat 2.2.7 to build; use expat.sln with visual studio 2017

7. build GDAL from source

nmake will complain _vsnprintf() error: edit the source and comment out the function

```
#define snprintf _snprintf
//#define snprintf _snprintf
```

odbc32.lib path should be updated in nmake64.opt
```
ODBC_DRV_HOME= "C:\Program Files (x86)\Windows Kits\10\Lib\10.0.17763.0\um\x64"
```
Also, since the change of the odbc32.lib, has to add  "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.16.27023\lib\x64\legacy_stdio_definitions.lib" in ODBCLIB
```
ODBCLIB = $(ODBC_DRV_HOME)\odbc32.lib $(ODBC_DRV_HOME)\odbccp32.lib $(ODBC_DRV_HOME)\user32.lib  "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.16.27023\lib\x64\legacy_stdio_definitions.lib"
```

8. build boost from source

only boost 1 64 (Or later) can work with visual studio 2017. Other things remain the same.

9. build json spirit

use visual studio 2017 to reopen the solution

10. wxWidgets 3.1.2

Before build wxWidgets, pause building process and modify two setup.h files: change "wxUSE_POSTSCRIPT 0" to "wxUSE_POSTSCRIPT 1"
Then, build wxWidgets again

11. Others:

zlib needs to be compiled using vs 2017 as well. 

expat.dll needs to be renamed to libexpat.dll