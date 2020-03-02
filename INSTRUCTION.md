# Graphics Playground Getting Started: Instructions

**Requirements:**
- An NVIDIA graphics card. Any card with Compute Capability 2.0 (sm_20) or greater will work. Check your GPU in this [compatibility table](https://developer.nvidia.com/cuda-gpus). 
- Windows 10
- Vulkan 1.2.131.2
- Visual Studio 2019
- CMake
- Git


## Step 1: Setting up your development environment

1. Make sure you are running Windows 10 and that your NVIDIA drivers are up-to-date
2. Install Visual Studio 2019
3. Install Vulkan 1.2.131.2 or higher
	* Download Vulkan [here](https://www.khronos.org/vulkan/).
	* Make sure to run the downloaded installed as administrator so that the installer can set the appropriate environment variables for you.
	* To check that Vulkan is ready for use, go to your Vulkan SDK directory (C:/VulkanSDK/ unless otherwise specified) and run the vkcube.exe example within the Bin directory. If you see a rotating gray cube with the LunarG logo, then you are all set!
	* If not, you may need to make sure your GPU driver supports Vulkan. If need be, download and install a [Vulkan driver](https://developer.nvidia.com/vulkan-driver) from NVIDIA's website.
4. Install CMake
5. Install Git


## Step 2: Fork or download the code

1. Use GitHub to fork this repository into your own GitHub account.
2. Clone from GitHub onto your machine:
	 * Navigate to the directory where you want to keep your files, then clone your fork.
	 * `git clone` the clone URL from your GitHub fork homepage.

* [How to use GitHub](https://guides.github.com/activities/hello-world/)
* [How to use Git](http://git-scm.com/docs/gittutorial)


## Step 3: Build and run

* `src/` contains the source code.
* `external/` contains the binaries and headers for GLFW, glm, tiny_obj, and stb_image.

**CMake note:** Do not change any build settings or add any files to your project directly in Visual Studio. Instead, edit the `src/CMakeLists.txt` file. Any files you add that is outside the `src/GraphicsPlayground` directory must be added here. If you edit it, just rebuild your VS project to make it update itself.


### Windows

1. In Git Bash, navigate to your cloned project directory.
2. Create a `build` directory: `mkdir build`
	* (This "out-of-source" build makes it easy to delete the `build` directory and try again if something goes wrong with the configuration.)
3. Navigate into that directory: `cd build`
4. Open the CMake GUI to configure the project:
	* `cmake-gui ..` or `"C:\Program Files (x86)\cmake\bin\cmake-gui.exe" ..`
	* Make sure that the "Source" directory is like `.../GraphicsPlayground`.
	* Click *Configure*.  Select your version of Visual Studio, Win64
	* Click *Generate*.
5. If generation was successful, there should now be a Visual Studio solution (`.sln`) file in the `build` directory that you just created. Open this (from the command line: `explorer *.sln`)
6. Build. (Note that there are Debug and Release configuration options.)
7. Run. Make sure you run the `GraphicsPlayground` target (not `ALL_BUILD`) by right-clicking it and selecting "Set as StartUp Project".
	* If you have switchable graphics (NVIDIA Optimus), you may need to force your program to run with only the NVIDIA card. In NVIDIA Control Panel, under "Manage 3D Settings," set "Multi-display/Mixed GPU acceleration" to "Single display performance mode".


**Note:** 

While developing, you will want to keep validation layers enabled so that error checking is turned on. The project is set up such that when you are in debug mode, validation layers are enabled, and when you are in release mode, validation layers are disabled. After building the code, you should be able to run the project without any errors. Two windows will open -  one will either be blank or show errors if any (if you're running in debug mode), the other (pending no errors) will show the clouds with a sun and sky background above an ocean-like gradient.


## Other Notes

* Compile GLSL shaders into SPIR-V bytecode:
* **Windows ONLY** Create a compile.bat file with the following contents:

```
C:/VulkanSDK/1.0.17.0/Bin32/glslangValidator.exe -V shader.vert
C:/VulkanSDK/1.0.17.0/Bin32/glslangValidator.exe -V shader.frag
pause
```
Note: Replace the path to glslangValidator.exe with the path to where you installed the Vulkan SDK. Double click the file to run it.


