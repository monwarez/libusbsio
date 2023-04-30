This test application demonstrates use of libusbsio library in a C project.

Use the 'release' target to link with the library located in the 'bin/platform' directory.

Note that the 'debug' target requires the debugging version of the library to be 
present in bin_debug directory. A debugging version is only available in the full 
source-code distribution of the libusbsio package. Reach out to your NXP representative 
or to NXP MCULink community to find out more information.

Note to Visual Studio users:
- Use the ReleaseS or DebugS targets to link with a static libusbsio.lib library.
- Use Release or Debug targets to link with a DLL loader library libusbsio.dll.lib.
- Default build tools version is set to v140 (Visual Studio 2015) which is compatible
  up to Visual Studio 2019. Change the project properties and use different versions
  of the libraries located in the 'bin' directory when using Visual Studio 2012 or 2013.
