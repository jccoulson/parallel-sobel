//Jesse Coulson
//CSCI 551 MPI final program
//sequential code provided by Sam Siewert
//https://www.ecst.csuchico.edu/~sbsiewert/csci551/code/sobel/sobel.c
//Program uses MPI to parallelize Sobel edge detection on bmp images


#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mpi.h>

#define MAX_PATH 256
#define FILE_ROOT_PATH "./"

#define True true
#define False false


typedef struct RGB
{
    unsigned char R, G, B;
} RGB;

typedef struct BMPIMAGE
{
    char FILENAME[MAX_PATH];
    unsigned int XSIZE;
    unsigned int YSIZE;
    unsigned char FILLINGBYTE;
    unsigned char *IMAGE_DATA;
} BMPIMAGE;

typedef struct RGBIMAGE
{
    unsigned int XSIZE;
    unsigned int YSIZE;
    RGB *IMAGE_DATA;
} RGBIMAGE;

RGB *SobelEdge(const int xsize, const int ysize, const RGB * const image, int my_rank, int comm_sz);

char bmp_read(unsigned char *image, const int xsize, const int ysize, const char *filename, const bool extension);
BMPIMAGE bmp_file_read(const char *filename, const bool extension);
int bmp_write(const unsigned char *image, const int xsize, const int ysize, const char *filename);
unsigned char *array_to_raw_image(const RGB* InputData, const int xsize, const int ysize);
RGB *raw_image_to_array(const unsigned char *image, const int xsize, const int ysize);
unsigned char bmp_filling_byte_calc(const unsigned int xsize);


int main(void)
{
    int size;
    int comm_sz, my_rank;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    //create mpi rgb type for message passing later
    MPI_Datatype RGB_type;
    //rgb type is made of 3 unsigned char from the struct RGB
    MPI_Type_contiguous(3, MPI_UNSIGNED_CHAR, &RGB_type);
    MPI_Type_commit(&RGB_type);

    char *FilenameString;
    FilenameString = (char*)malloc( MAX_PATH * sizeof(char) );
    //change the filename to want you want without .bmp extension
    //using instead of user input to time speedup of overall program
    strcpy(FilenameString, "bowling");

    //declare bmp struct for all ranks
    BMPIMAGE BMPImage1;
    //read in bmp file infomration to struct for rank 0
    if(my_rank == 0)
    {
      printf("Doing MPI sobel on %s.bmp\n", FilenameString);
      BMPImage1 = bmp_file_read(FilenameString,false);
      free(FilenameString);

      if(BMPImage1.IMAGE_DATA == NULL)
      {
          printf("Image contents error!");
          exit(-1);
      }
      //store size of bmp image
      size = BMPImage1.XSIZE * BMPImage1.YSIZE;
    }
    //broadcast size to all ranks for memory allocation
    MPI_Bcast(&size, 1, MPI_INT, 0, MPI_COMM_WORLD);

    //declare RGBIMAGE struct for all ranks
    RGBIMAGE RGBImage1;

    //malloc memory for 3 values representing rgb for each pixel in image
    RGBImage1.IMAGE_DATA = (RGB*)malloc(size * sizeof(RGB));
    if (RGBImage1.IMAGE_DATA == NULL)
    {
        printf("Memory allocation error!");
        exit(-1);
    }

    //convert the bmp struct to RGB struct for rank 0
    if(my_rank == 0)
    {
      //BMP image sizes and RGB image sizes are the same
      RGBImage1.XSIZE = BMPImage1.XSIZE;
      RGBImage1.YSIZE = BMPImage1.YSIZE;

      if (RGBImage1.IMAGE_DATA == NULL)
      {
          printf("Memory allocation error!");
          exit(-1);
      }
      //store pixel information as RGB array
      RGBImage1.IMAGE_DATA = raw_image_to_array(BMPImage1.IMAGE_DATA, BMPImage1.XSIZE, BMPImage1.YSIZE);
    }

    //broadcast the data of rank 0's RGBImage struct to all ranks
    MPI_Bcast(RGBImage1.IMAGE_DATA, size, RGB_type, 0, MPI_COMM_WORLD);
    MPI_Bcast(&RGBImage1.XSIZE, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&RGBImage1.YSIZE, 1, MPI_INT, 0, MPI_COMM_WORLD);

    //alocate memory for slice for each process
    RGB* myslice = (RGB*) malloc((sizeof(RGB)*size)/comm_sz);


    //each slice points to different portion of divided up RGB array
    myslice = RGBImage1.IMAGE_DATA+((size / comm_sz) * my_rank);

    //need a var to receive all arrays with MPI_Gather
    RGB* temp_reciever = NULL;
    if (my_rank == 0)
    {

      //need to allocate enough memory of all RGB arrays put togethor
      temp_reciever = (RGB*) malloc(sizeof(RGB)*size* sizeof(unsigned char));
    }

    //do sobel algorithm on each process' slice
    //each slice has same width as original image, but the height is divided by num procs
    RGB* temp = SobelEdge(RGBImage1.XSIZE, RGBImage1.YSIZE/comm_sz, myslice, my_rank, comm_sz);



    //gather all the RGB* temp slices into rank 0's temp_reciever
    MPI_Gather(temp, (size)/comm_sz, RGB_type, temp_reciever, (size)/comm_sz, RGB_type, 0,
               MPI_COMM_WORLD);

    //have rank 0 write out completed sobel transformation with temp_reciever
    if(my_rank == 0)
    {
      bmp_write(array_to_raw_image(temp_reciever, RGBImage1.XSIZE, RGBImage1.YSIZE), BMPImage1.XSIZE, BMPImage1.YSIZE, "MPISobelTransformOutput");
    }


    MPI_Finalize();
    return 0;
};



RGB *SobelEdge(const int xsize, const int ysize, const RGB * const image, int my_rank, int comm_sz)
{
    RGB *output;
    output = malloc(xsize * ysize * sizeof(RGB));
    if (output == NULL)
    {
        printf("Memory allocation error!");
        return NULL;
    }

    for(long long int i = 0; i < (xsize * ysize); i++)
    {
        //checking what border for each rank. Rank 0 and
        //last rank, comm_sz-1 need horizontal borders at top and bottom of image
        //all other ranks only need vertical borders at edge of image
        if( (my_rank == 0 &&((i < xsize) || ( (i % xsize) == 0) || ( ((i + 1) % xsize) == 0) ))
          || (my_rank == comm_sz-1 &&( ( (i % xsize) == 0) || ( ((i + 1) % xsize) == 0) || (i >= (xsize*(ysize-1))) ))
        ||((my_rank != comm_sz-1 && my_rank != 0)&&( ( (i % xsize) == 0) || ( ((i + 1) % xsize) == 0) )))
        {
            output[i].R = image[i].R;
            output[i].G = image[i].G;
            output[i].B = image[i].B;
        }
        else
        {
            int Gx = 0, Gy = 0;
            Gx = (
                image[i+xsize-1].R    * (-1) + image[i+xsize].R    * 0 + image[i+xsize+1].R    * 1 +
                image[i-1].R         * (-2) + image[i].R            * 0 + image[i+1].R         * 2 +
                image[i-xsize-1].R    * (-1) + image[i-xsize].R    * 0 + image[i-xsize+1].R    * 1
                );
            Gy = (
                image[i+xsize-1].R    * (-1) + image[i+xsize].R    * (-2) + image[i+xsize+1].R    * (-1) +
                image[i-1].R         *   0  + image[i].R            *   0  + image[i+1].R         *   0  +
                image[i-xsize-1].R    *   1  + image[i-xsize].R    *   2  + image[i-xsize+1].R    *   1
                );
            long double magnitude;
            magnitude = sqrt(pow(Gx, 2) + pow(Gy, 2));

            output[i].R = (magnitude > 255)?255:(magnitude < 0)?0:(unsigned char)magnitude;

            Gx = (
                image[i+xsize-1].G    * (-1) + image[i+xsize].G    * 0 + image[i+xsize+1].G    * 1 +
                image[i-1].G         * (-2) + image[i].G            * 0 + image[i+1].G         * 2 +
                image[i-xsize-1].G    * (-1) + image[i-xsize].G    * 0 + image[i-xsize+1].G    * 1
                );
            Gy = (
                image[i+xsize-1].G    * (-1) + image[i+xsize].G    * (-2) + image[i+xsize+1].G    * (-1) +
                image[i-1].G         *   0  + image[i].G            *   0  + image[i+1].G         *   0  +
                image[i-xsize-1].G    *   1  + image[i-xsize].G    *   2  + image[i-xsize+1].G    *   1
                );
            magnitude = sqrt(pow(Gx, 2) + pow(Gy, 2));

            output[i].G = (magnitude > 255)?255:(magnitude < 0)?0:(unsigned char)magnitude;

            Gx = (
                image[i+xsize-1].B    * (-1) + image[i+xsize].B    * 0 + image[i+xsize+1].B    * 1 +
                image[i-1].B         * (-2) + image[i].B            * 0 + image[i+1].B         * 2 +
                image[i-xsize-1].B    * (-1) + image[i-xsize].B    * 0 + image[i-xsize+1].B    * 1
                );
            Gy = (
                image[i+xsize-1].B    * (-1) + image[i+xsize].B    * (-2) + image[i+xsize+1].B    * (-1) +
                image[i-1].B         *   0  + image[i].B            *   0  + image[i+1].B         *   0  +
                image[i-xsize-1].B    *   1  + image[i-xsize].B    *   2  + image[i-xsize+1].B    *   1
                );
            magnitude = sqrt(pow(Gx, 2) + pow(Gy, 2));

            output[i].B = (magnitude > 255)? 255: (magnitude < 0)?0:(unsigned char)magnitude;
        }
    }
    return output;
}


RGB *raw_image_to_array(const unsigned char *image, const int xsize, const int ysize)
{
    RGB *output;
    output = (RGB*)malloc(xsize * ysize * sizeof(RGB));
    if(output == NULL)
    {
        printf("Memory allocation error!");
        return NULL;
    }
    unsigned char FillingByte;
    FillingByte = bmp_filling_byte_calc(xsize);
    for(int y = 0;y<ysize;y++)
    {
        for(int x = 0;x<xsize;x++)
        {
            output[y*xsize + x].R =
            image[3*(y * xsize + x) + y * FillingByte + 2];
            output[y*xsize + x].G =
            image[3*(y * xsize + x) + y * FillingByte + 1];
            output[y*xsize + x].B =
            image[3*(y * xsize + x) + y * FillingByte + 0];
        }
    }
    return output;
}

//----bmp_read_x_size function----
unsigned long bmp_read_x_size(const char *filename, const bool extension)
{
    char fname_bmp[MAX_PATH];

    if(extension == false)
    {
        sprintf(fname_bmp, "%s.bmp", filename);
    }
    else
    {
        strcpy(fname_bmp,filename);
    }
    FILE *fp;
    fp = fopen(fname_bmp, "rb");
    if (fp == NULL)
    {
        printf("Fail to read file!\n");
        return -1;
    }
    unsigned char header[54];
    fread(header, sizeof(unsigned char), 54, fp);
    unsigned long output;
    output = header[18] + (header[19] << 8) + ( header[20] << 16) + ( header[21] << 24);
    fclose(fp);
    return output;
}

//---- bmp_read_y_size function ----
unsigned long bmp_read_y_size(const char *filename, const bool extension)
{
    char fname_bmp[MAX_PATH];

    if(extension == false)
    {
        sprintf(fname_bmp, "%s.bmp", filename);
    }
    else
    {
        strcpy(fname_bmp,filename);
    }
    FILE *fp;
    fp = fopen(fname_bmp, "rb");
    if (fp == NULL)
    {
        printf("Fail to read file!\n");
        return -1;
    }
    unsigned char header[54];
    fread(header, sizeof(unsigned char), 54, fp);
    unsigned long output;
    output= header[22] + (header[23] << 8) + ( header[24] << 16) + ( header[25] << 24);
    fclose(fp);
    return output;
}

//---- bmp_file_read function ----
char bmp_read(unsigned char *image, const int xsize, const int ysize, const char *filename, const bool extension)
{
    char fname_bmp[MAX_PATH];

    if(extension == false)
    {
        sprintf(fname_bmp, "%s.bmp", filename);
    }
    else
    {
        strcpy(fname_bmp,filename);
    }
    unsigned char filling_bytes;
    filling_bytes = bmp_filling_byte_calc(xsize);
    FILE *fp;
    fp = fopen(fname_bmp, "rb");
    if (fp==NULL)
    {
        printf("Fail to read file!\n");
        return -1;
    }
    unsigned char header[54];
    fread(header, sizeof(unsigned char), 54, fp);
    fread(image, sizeof(unsigned char), (size_t)(long)(xsize * 3 + filling_bytes)*ysize, fp);
    fclose(fp);
    return 0;
}

BMPIMAGE bmp_file_read(const char *filename, const bool extension)
{
    BMPIMAGE output;
    stpcpy(output.FILENAME, "");
    output.XSIZE = 0;
    output.YSIZE = 0;
    output.IMAGE_DATA = NULL;
    if(filename == NULL)
    {
        printf("Path is null\n");
        return output;
    }
    char fname_bmp[MAX_PATH];
    if(extension == false)
    {
        sprintf(fname_bmp, "%s.bmp", filename);
    }
    else
    {
        strcpy(fname_bmp,filename);
    }
    FILE *fp;
    fp = fopen(fname_bmp, "rb");
    if (fp == NULL)
    {
        printf("Fail to read file!\n");
        return output;
    }
    stpcpy(output.FILENAME, fname_bmp);
    output.XSIZE = (unsigned int)bmp_read_x_size(output.FILENAME,true);
    output.YSIZE = (unsigned int)bmp_read_y_size(output.FILENAME,true);
    if( (output.XSIZE == -1) || (output.YSIZE == -1) )
    {
        printf("Fail to fetch information of image!");
        return output;
    }
    else
    {
        printf("Width of the input image: %d\n",output.XSIZE);
        printf("Height of the input image: %d\n",output.YSIZE);
        printf("Size of the input image(Byte): %lu\n",(size_t)output.XSIZE * output.YSIZE * 3);
        output.FILLINGBYTE = bmp_filling_byte_calc(output.XSIZE);
        output.IMAGE_DATA = (unsigned char*)malloc((output.XSIZE * 3 + output.FILLINGBYTE) * output.YSIZE * sizeof(unsigned char));
        if (output.IMAGE_DATA == NULL)
        {
            printf("Memory allocation error!");
            return output;
        }
        else
        {
            for(int i = 0; i < ((output.XSIZE * 3 + output.FILLINGBYTE) * output.YSIZE);i++)
            {
                output.IMAGE_DATA[i] = 255;
            }
            bmp_read(output.IMAGE_DATA, output.XSIZE, output.YSIZE, output.FILENAME, true);
        }
    }
    return output;
}

//----bmp_write function----
int bmp_write(const unsigned char *image, const int xsize, const int ysize, const char *filename)
{
    unsigned char FillingByte;
    FillingByte = bmp_filling_byte_calc(xsize);
    unsigned char header[54] =
    {
    0x42, 0x4d, 0, 0, 0, 0, 0, 0, 0, 0,
    54, 0, 0, 0, 40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 24, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0
    };
    unsigned long file_size = (long)xsize * (long)ysize * 3 + 54;
    unsigned long width, height;
    char fname_bmp[MAX_PATH];
    header[2] = (unsigned char)(file_size &0x000000ff);
    header[3] = (file_size >> 8) & 0x000000ff;
    header[4] = (file_size >> 16) & 0x000000ff;
    header[5] = (file_size >> 24) & 0x000000ff;

    width = xsize;
    header[18] = width & 0x000000ff;
    header[19] = (width >> 8) &0x000000ff;
    header[20] = (width >> 16) &0x000000ff;
    header[21] = (width >> 24) &0x000000ff;

    height = ysize;
    header[22] = height &0x000000ff;
    header[23] = (height >> 8) &0x000000ff;
    header[24] = (height >> 16) &0x000000ff;
    header[25] = (height >> 24) &0x000000ff;
    sprintf(fname_bmp, "%s.bmp", filename);
    FILE *fp;
    if (!(fp = fopen(fname_bmp, "wb")))
    {
        return -1;
    }
    fwrite(header, sizeof(unsigned char), 54, fp);
    fwrite(image, sizeof(unsigned char), (size_t)(long)(xsize * 3 + FillingByte)*ysize, fp);
    fclose(fp);
    return 0;
}

unsigned char *array_to_raw_image(const RGB* InputData, const int xsize, const int ysize)
{
    unsigned char FillingByte;
    FillingByte = bmp_filling_byte_calc(xsize);
    unsigned char *output;
    output = (unsigned char*)malloc((xsize * 3 + FillingByte) * ysize * sizeof(unsigned char));
    if(output == NULL)
    {
        printf("Memory allocation error!");
        return NULL;
    }
    for(int y = 0;y < ysize;y++)
    {
        for(int x = 0;x < (xsize * 3 + FillingByte);x++)
        {
            output[y * (xsize * 3 + FillingByte) + x] = 0;
        }
    }
    for(int y = 0;y<ysize;y++)
    {
        for(int x = 0;x<xsize;x++)
        {
            output[3*(y * xsize + x) + y * FillingByte + 2]
            = InputData[y*xsize + x].R;
            output[3*(y * xsize + x) + y * FillingByte + 1]
            = InputData[y*xsize + x].G;
            output[3*(y * xsize + x) + y * FillingByte + 0]
            = InputData[y*xsize + x].B;
        }
    }
    return output;
}

unsigned char bmp_filling_byte_calc(const unsigned int xsize)
{

    unsigned char filling_bytes;
    filling_bytes = ( xsize % 4);
    return filling_bytes;
}
