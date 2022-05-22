#include <iostream>
#include <unistd.h>
#include <fstream>

#define FILE_OUT "serial.bmp"

using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;

#pragma pack(1)
#pragma once

typedef int LONG;
typedef unsigned short WORD;
typedef unsigned int DWORD;

typedef struct tagBITMAPFILEHEADER {
    WORD bfType;
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

struct PixelColor {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};

int rows;
int cols;

bool fillAndAllocate(char *&buffer, const char *fileName, int &rows, int &cols, int &bufferSize) {
  std::ifstream file(fileName);

  if (file) {
    file.seekg(0, std::ios::end);
    std::streampos length = file.tellg();
    file.seekg(0, std::ios::beg);

    buffer = new char[length];
    file.read(&buffer[0], length);

    PBITMAPFILEHEADER file_header;
    PBITMAPINFOHEADER info_header;

    file_header = (PBITMAPFILEHEADER) (&buffer[0]);
    info_header = (PBITMAPINFOHEADER) (&buffer[0] + sizeof(BITMAPFILEHEADER));
    rows = info_header->biHeight;
    cols = info_header->biWidth;
    bufferSize = file_header->bfSize;
    return 1;
  } else {
    cout << "File" << fileName << " doesn't exist!" << endl;
    return 0;
  }
}

void getPixlesFromBMP24(int end, int rows, int cols, char *fileReadBuffer, PixelColor **&pixels) {
  int count = 1;
  int extra = cols % 4;
  pixels = new PixelColor *[cols];
  for (int i = 0; i < cols; i++) {
    pixels[i] = new PixelColor[rows];
  }
  for (int i = 0; i < rows; i++) {
    count += extra;
    for (int j = cols - 1; j >= 0; j--)
      for (int k = 0; k < 3; k++) {
        switch (k) {
          case 0:
            pixels[j][i].red = fileReadBuffer[end - count];
            break;
          case 1:
            pixels[j][i].green = fileReadBuffer[end - count];
            break;
          case 2:
            pixels[j][i].blue = fileReadBuffer[end - count];
            break;
        }
        count++;
      }
  }
}

void writeOutBmp24(char *fileBuffer, const char *nameOfFileToCreate, int bufferSize, PixelColor **input) {
  std::ofstream write(nameOfFileToCreate);
  if (!write) {
    cout << "Failed to write " << nameOfFileToCreate << endl;
    return;
  }
  int count = 1;
  int extra = cols % 4;
  for (int i = 0; i < rows; i++) {
    count += extra;
    for (int j = cols - 1; j >= 0; j--)
      for (int k = 0; k < 3; k++) {
        switch (k) {
          case 0:
            fileBuffer[bufferSize - count] = input[j][i].red;
            break;
          case 1:
            fileBuffer[bufferSize - count] = input[j][i].green;
            break;
          case 2:
            fileBuffer[bufferSize - count] = input[j][i].blue;
            break;
        }
        count++;
      }
  }
  write.write(fileBuffer, bufferSize);
}

PixelColor get_mean(PixelColor **&input, int x, int y) {
  PixelColor mean;
  int green_sum = 0, red_sum = 0, blue_sum = 0, count = 0;
  for (int i = -1; i <= 1; ++i) {
    for (int j = -1; j <= 1; ++j) {
      if (x + 1 < 0 || x + i >= rows)
        continue;
      if (y + 1 < 0 || y + j >= cols)
        continue;
      green_sum += input[x + i][y + j].green;
      red_sum += input[x + i][y + j].red;
      blue_sum += input[x + i][y + j].blue;
      count++;
    }
  }
  green_sum /= count;
  red_sum /= count;
  blue_sum /= count;
  mean.green = (unsigned char) green_sum;
  mean.red = (unsigned char) red_sum;
  mean.blue = (unsigned char) blue_sum;
  return mean;
}

PixelColor get_mean_all(PixelColor **&input) {
  PixelColor mean;
  int green_sum = 0, red_sum = 0, blue_sum = 0, count = 0;
  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < cols; ++j) {
      green_sum += (int) input[i][j].green;
      red_sum += (int) input[i][j].red;
      blue_sum += (int) input[i][j].blue;
    }
  }
  count = rows * cols;
  green_sum /= count;
  red_sum /= count;
  blue_sum /= count;
  mean.green = green_sum;
  mean.red = red_sum;
  mean.blue = blue_sum;
  return mean;
}

void delete_pixels(PixelColor **&input) {
  for (int i = 0; i < cols; i++) {
    delete[] input[i];
  }
  delete[] input;
}

void filter_horizontal(PixelColor **&input) {
  PixelColor **output = new PixelColor *[cols];
  for (int i = 0; i < cols; i++) {
    output[i] = new PixelColor[rows];
  }
  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < cols; j++) {
      output[i][j] = input[rows - 1 - i][j];
    }
  }
  delete_pixels(input);
  input = output;
}

void filter_vertical(PixelColor **&input) {
  PixelColor **output = new PixelColor *[cols];
  for (int i = 0; i < cols; i++) {
    output[i] = new PixelColor[rows];
  }
  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < cols; j++) {
      output[i][j] = input[i][cols - 1 - j];
    }
  }
  delete_pixels(input);
  input = output;
}

void filter_smoothing(PixelColor **&pixel) {
  PixelColor **output = new PixelColor *[cols];
  for (int i = 0; i < cols; i++) {
    output[i] = new PixelColor[rows];
  }
  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < cols; ++j) {
      if (i == 0 || j == 0 || j == cols - 1 || i == rows - 1)
        continue;
      output[i][j] = get_mean(pixel, i, j);
    }
  }
  delete_pixels(pixel);
  pixel = output;
}

void filter_reflex_color(PixelColor **&input) {
  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < cols; ++j) {
      int red = abs(255 - input[i][j].red);
      int green = abs(255 - input[i][j].green);
      int blue = abs(255 - input[i][j].blue);
      input[i][j].red = (unsigned char) red;
      input[i][j].green = (unsigned char) green;
      input[i][j].blue = (unsigned char) blue;
    }
  }
}


void add_cross(PixelColor **&input) {
  int cols_mean = cols / 2;
  for (int i = 0; i < rows; ++i) {
    for (int j = -1; j <= 1; ++j) {
      input[i][cols_mean + j].red = 255;
      input[i][cols_mean + j].green = 255;
      input[i][cols_mean + j].blue = 255;
    }
  }
  int rows_mean = rows / 2;
  for (int i = -1; i <= 1; ++i) {
    for (int j = 0; j < cols; ++j) {
      input[rows_mean + i][j].red = 255;
      input[rows_mean + i][j].green = 255;
      input[rows_mean + i][j].blue = 255;
    }
  }
}

int main(int argc, char *argv[]) {
  clock_t start_time, end_time;
  start_time = clock();
  char *fileBuffer;
  int bufferSize;
  char *fileName = argv[1];
  PixelColor **picture_input;
  if (!fillAndAllocate(fileBuffer, fileName, rows, cols, bufferSize)) {
    cout << "File read error" << endl;
    return 1;
  }

  getPixlesFromBMP24(bufferSize, rows, cols, fileBuffer, picture_input);

  filter_horizontal(picture_input);
  filter_vertical(picture_input);
  filter_smoothing(picture_input);
  filter_reflex_color(picture_input);
  add_cross(picture_input);
  writeOutBmp24(fileBuffer, FILE_OUT, bufferSize, picture_input);
  end_time = clock();
  cout << "Execution Time: " << (int) (((double) (end_time - start_time) / CLOCKS_PER_SEC) * 1000) << endl;
  return 0;
}