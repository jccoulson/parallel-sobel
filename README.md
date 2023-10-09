# Parallelized Sobel Edge Detection
- The sequential version of this code applies Sobel edge detection to a BMP image and outputs an edge map in BMP format. The Sobel method identifies edges by transforming pixel values based on their surrounding pixels, applying convolution with specific Sobel kernels.

- The MPI version parallelizes the task using multiple nodes. Each node, identified by a rank, processes a segment of the image. After transformation the subsections are combined to form the full edge-detected image.

- The OpenMP version employs compiler directives to distribute the processing workload among multiple threads, speeding up the edge detection operation.

## Dependencies
- MPI library
- OpenMP
- C compiler

## Compiling the Project
- Use `make` to compile all c files
- Use `make clean` to remove files generated in build process

## Images
- `Bears.bmp`: sample image resolution 359x240
- `swan.bmp`: sample image resolution at 960x643
- Can use whatever image you choose at whatever resolution

## Running the Project
- Need list of nodes in `hosts.txt` for MPI
- Sequential:
 - `./seqsobel input.bmp output.bmp`
- OpenMP:
 - `./openmpsobel input.bmp output.bmp`
- MPI:
 - `mpirun -np 4 -hostfile hosts.txt ./mpisobel input.bmp`

## Analysis
- A detailed report analyzing the runtime performance of these implementations can be found in the project directory.
