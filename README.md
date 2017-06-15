# MeshToSDF Converter

This is a simple console application, which allows to convert model with triangular mesh into signed distance field (SDF) representation. Based on algorithm from "Distance Fields for Rapid Collision Detection in Physically Based Modeling" paper, so only two restrictions for supported models:
* Triangular mesh (as mentioned above)
* Must be specified vertex normals

Application use OpenGL's compute shaders for implementing this algorithm on GPU (and freeGLUT for initializing GL context).

Parameters for converter can be speicified by converter.parm file (example of which in repo). Some words about it's struct:
* **Model path** Path to the model, which should be converted
* **Size factor** Determining max allowed size for SSBOs, which used by app, by this formula: *MAX_SSBO_SIZE for implementation / Size factor*. Recomended value - 2.0.
* **SDF path** Path to file, in which SDF will be saved.
* **Resolution** Resolution of distance field. 0,0,0 - special value, which means maximum allowed resolution for current system (resolution for the axis depends on model's size).
* **Loader type** Type of the loader, which used for loading model from file. At this moment support only *Obj Loader*.
* **Filler value** Value, which used to fill output buffer before SDF generation. Recomended value - 1000000.0.
* **Delta** Algorithm's parameter, which determine "thickness" of distance envelope (see paper for more information).
* **Path to filler shader** Path to filler.glsl
* **Path to modifier shader** Path to modifier.glsl
* **Path to kernel shader** Path to kernel.glsl

**Limitations:**
* Max resolution of SDF depends on size of GPU memory and size factor value.
* Delta and time, spent on convertation, has exponential dependency.
* At this moment supported only text version of Wavefront OBJ format as input.

**Format of output binary file:**
First 4 bytes - amount of elements in file (amount of pairs "point - distance" - amount of elements / 4), then follows float values (4 bytes) in this order: first point x, first point y, first point z, first point distance, second point x, ...

Source code of this application can be used without any limitations, provided "as is" and without any warranties.
