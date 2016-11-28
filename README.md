# Shooter Demo #

**ShooterDemo** is a small 1st/3rd person shooter which I made in C++11 to investigate the usefulness of  the [Entity-Component-System (ECS)](https://en.wikipedia.org/wiki/Entity_component_system) design pattern when working on an actual game and if it facilitates the use of lock free multi-threading.

## Why this Demo? ##
After more then 10 years of working with OOP design patterns and reading a number of books on OOP (Modern C++ Design, Effective C++, More Effective C++, Effective STL, etc.) I decided to try something a bit less complex and see what are the benefits. I read about the Entity-Component-System design pattern and I liked it's simplicity, but since not everything you read in a book it's actually useful, I decided to give it a try in a small project.

## Design decisions ##

- ***Prefer Data Oriented Design over Object Oriented Design.*** It's been my experience that, in applications dealing with large amounts of data, it's more useful to reason about the data and how it can be processed in an efficient way, rather then think about objects and what design pattern to use. A very interesting talk on this subject is [Mike Acton's "Data-Oriented Design and C++"](https://www.youtube.com/watch?v=rX0ItVEVjHc) talk at CppCon 2014.
- ***Keep data separate from functionality.*** According to ECS, I tried to put all data in Components and all functionality in Systems, with a few exceptions (mainly lib and resource wrapper classes). One notable exception is the SysRenderer class which is a System and also contains data (OpenGL objects), but since this data it's used by all Systems, I didn't see the harm leaving it there.
- ***Prefer Structure of Arrays over Array of Structures.*** Organizing the data this way it's more cache friendly, since each System only iterates over the data that it needs to process and also facilitates the use of lock free multi-threading. 
- ***Avoid memory reallocation.*** Reallocating memory causes a performance hit and also memory fragmentation. It's a heavy price to pay and reallocation should only be used when absolutely necessary!
- ***All functions should have clear Input and Output parameters.*** All functions in Systems are static functions. I found that writing functions this way makes them easier to access from other parts of code (weak dependencies) and easier to use in a different thread without locks, since it's clear what data will be read and what data will be written by the function.
- ***Minimize the number cache misses.*** In my experience cache misses, especially in loops, cause a big performance hit. I tried as much as possible to reason about data locality when working on this project.
- ***Lock free multi-threading.*** Using a Data Oriented Design with data organized as Structure of Arrays in the Scene and having functions with clear input and output parameters, made it easy to reason about how the data can be read/written safely from different threads and I could make the app lock free.
- ***Use cross-platform libraries.*** I used only cross platform libraries, with the ideea that if other people want to port this on different operating systems, they should be able to do this with minimum effort.

## Libraries ##

- **OpenGL 4.5** - rendering
- **[GLEW](http://glew.sourceforge.net/)** - cross-platform OpenGL 4.5 extensions
- **[SDL2](https://www.libsdl.org/)** - cross-platform access to keyboard, mouse and graphic hardware
- **[CTPL](https://github.com/vit-vit/CTPL)** - multi-threading
- **[NanoVG](https://github.com/memononen/nanovg)** - text rendering
- **[GLM](http://glm.g-truc.net/0.9.8/index.html)** - OpenGL and vector math
- **[Minizip](https://github.com/madler/zlib/tree/master/contrib/minizip)** - read the map from a zip archive
- **[Q3Loader (modified)](http://www.flipcode.com/archives/Simple_Quake3_BSP_Loader.shtml)** - loading maps in Quake3 BSP format 
- **[Assimp](https://github.com/assimp/assimp)** - loading 3D models
- **[Recast&Detour](https://github.com/recastnavigation/recastnavigation)** - create a navigation mesh and calculate walking paths and jump down links on it.

## Build ##

**Dependencies:** CMake 3.3

`./cmake .`

The Resources, Libraries (Windows only) and Binaries (Windows only) are automatically downloaded when running the build command.

For Visual Studio, open ShooterDemo.sln, set the ShooterDemo as StartUp Project and set the Working Directory to `$(ProjectDir)/bin` for each Configuration that you are using (Debug, Release, etc.)

## Run (Windows OS)##

**Dependencies:** OpenGL 4.5, [Visual C++ Redistributable for Visual Studio 2015](https://www.microsoft.com/en-us/download/details.aspx?id=48145)

`cd ./bin`

`./ShooterDemo.exe`

## Contrib ##

- **Vlad Catoi** - Adding Linux build support. Tested on Fedora 23 with gcc 5.3.1

## Screenshots ##

![](https://github.com/giulian2003/ShooterDemo/releases/download/v1.0/screenshot.jpg)

![](https://github.com/giulian2003/ShooterDemo/releases/download/v1.0/screenshot2.jpg)
