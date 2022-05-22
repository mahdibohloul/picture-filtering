#include <iostream>
#include <unistd.h>
#include <fstream>
#include <threads.h>

#define FILE_OUT "parallel.bmp"

using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;

#pragma pack(1)
#pragma once

typedef int LONG;
typedef unsigned short WORD;
typedef unsigned int DWORD;

int const MAX_ROW = 1000, MAX_COL = 1000;

unsigned char picture_input[MAX_ROW][MAX_COL][3];
unsigned char picture_output[MAX_ROW][MAX_COL][3];
bool ok[MAX_ROW][MAX_COL][3];
int const SIZE_ROWS_READ = 8, SIZE_ROWS_FILTER = 100;

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

struct PixelPos {
    int row;
    int col;
};

struct PixelColor {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};

int rows;
int cols;

struct Message {
    int row;
    int end_row;
    unsigned char ****input;
    unsigned char ****output;
    char **file_read_buffer;
    int buffer_size;
};

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

void *getting_pixels_row(void *arg_inp) {
  Message *message = (Message *) arg_inp;
  int start_row = message->row;
  int end_row = message->end_row;
  int extra = cols % 4;
  int count = start_row * 3 * cols + extra * start_row + 1;
  char **file_read_buffer = message->file_read_buffer;
  int end = message->buffer_size;
  for (int i = start_row; i <= end_row; ++i) {
    for (int j = cols - 1; j >= 0; --j) {
      for (int k = 0; k < 3; ++k) {
        picture_input[i][j][k] = (*file_read_buffer)[end - count];
//        picture_output[i][j][k] = picture_input[i][j][k];
        count++;
      }
    }
  }
  pthread_exit(NULL);
}

void *copy_pixel(void *arg_inp) {
  PixelPos *pixel_pos = (PixelPos *) arg_inp;
  cout << "Pixel pos: " << pixel_pos->row << " " << pixel_pos->col << endl;
  for (int i = 0; i < 3; ++i) {
    picture_output[pixel_pos->row][pixel_pos->col][i] = picture_input[pixel_pos->row][pixel_pos->col][i];
  }
  pthread_exit(NULL);
}

void *copy_col(void *row_num) {
  long row = (long) row_num;
  pthread_t *col_threads = new pthread_t[cols];
  PixelPos *pixel_poses = new PixelPos[cols];
  for (int i = 0; i < cols; ++i) {
    pixel_poses[i].row = row;
    pixel_poses[i].col = i;
    bool error = pthread_create(&col_threads[i], NULL, copy_pixel, (void *) &pixel_poses[i]);
    if (error) {
      cout << "Error creating thread" << endl;
      exit(1);
    }
  }

  for (int i = 0; i < cols; ++i) {
    bool error = pthread_join(col_threads[i], NULL);
    if (error) {
      cout << "Error joining thread" << endl;
      exit(1);
    }
  }
  delete[] pixel_poses;
  delete[] col_threads;
  pthread_exit(NULL);
}


void getPixlesFromBMP24(int end, int rows, int cols, char *fileReadBuffer) {
  int count = 1;
  int extra = cols % 4;
  Message *messages = new Message[rows / SIZE_ROWS_FILTER + 1];
  pthread_t *threads_all = new pthread_t[rows / SIZE_ROWS_FILTER + 1];
  for (int i = 0; i < rows; i += SIZE_ROWS_FILTER) {
    messages[i / SIZE_ROWS_FILTER].row = i;
    messages[i / SIZE_ROWS_FILTER].end_row = i + SIZE_ROWS_FILTER - 1;
    messages[i / SIZE_ROWS_FILTER].file_read_buffer = &fileReadBuffer;
    messages[i / SIZE_ROWS_FILTER].buffer_size = end;
    if (messages[i / SIZE_ROWS_FILTER].end_row >= rows)
      messages[i / SIZE_ROWS_FILTER].end_row = rows - 1;
    bool error = pthread_create(&threads_all[i / SIZE_ROWS_FILTER], NULL, getting_pixels_row,
                                (void *) &messages[i / SIZE_ROWS_FILTER]);
    if (error) {
      cout << "Error creating thread" << endl;
      exit(1);
    }
  }

  for (int i = 0; i < rows; i += SIZE_ROWS_FILTER) {
    bool error = pthread_join(threads_all[i / SIZE_ROWS_FILTER], NULL);
    if (error) {
      cout << "Error joining thread" << endl;
      exit(1);
    }
  }

//  pthread_t *row_threads = new pthread_t[rows];
//  for (long i = 0; i < rows; ++i) {
//    bool error = pthread_create(&row_threads[i], NULL, copy_col, (void *) i);
//    if (error) {
//      cout << "HAHA" << endl;
//      cout << "Error creating thread" << endl;
//      exit(1);
//    }
//  }
//
//  for (int i = 0; i < rows; ++i) {
//    bool error = pthread_join(row_threads[i], NULL);
//    if (error) {
//      cout << "Error joining thread" << endl;
//      exit(1);
//    }
//  }
//  delete[] row_threads;
  delete[] messages;
  delete[] threads_all;
}

void writeOutBmp24(char *fileBuffer, const char *nameOfFileToCreate, int bufferSize) {
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
        fileBuffer[bufferSize - count] = picture_output[i][j][k];
        count++;
      }
  }
  write.write(fileBuffer, bufferSize);
}

unsigned char get_mean(int x, int y, int color) {
  int ans = 0, count = 0;
  for (int i = -1; i <= 1; ++i) {
    for (int j = -1; j <= 1; ++j) {
      if (x + 1 < 0 || x + i >= rows)
        continue;
      if (y + 1 < 0 || y + j >= cols)
        continue;
      ans += picture_output[x + i][y + j][color];
      count++;
    }
  }
  ans /= count;
  return (unsigned char) ans;
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

void *filter_horizontal(void *arg) {
  long color = (long) arg;
  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < cols; j++) {
      picture_output[i][j][color] = picture_input[rows - 1 - i][j][color];
    }
  }
  pthread_exit(NULL);
}

void *filter_vertical(void *arg) {
  long color = (long) arg;
  unsigned char temp[rows][cols][3];
  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < cols; j++) {
      temp[i][j][color] = picture_output[i][cols - 1 - j][color];
    }
  }
  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < cols; j++) {
      picture_output[i][j][color] = temp[i][j][color];
    }
  }
  pthread_exit(NULL);
}

void filter_smoothing(int color) {
  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < cols; ++j) {
      if (i == 0 || j == 0 || j == cols - 1 || i == rows - 1)
        continue;
      picture_output[i][j][color] = get_mean(i, j, color);
    }
  }
}

void filter_reflex_color(int color) {
  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < cols; ++j) {
      int temp = abs(255 - picture_output[i][j][color]);
      picture_output[i][j][color] = temp;
    }
  }
}


void add_cross(int color) {
  int cols_mean = cols / 2;
  for (int i = 0; i < rows; ++i) {
    for (int j = -1; j <= 1; ++j) {
      picture_output[i][cols_mean + j][color] = 255;
    }
  }
  int rows_mean = rows / 2;
  for (int i = -1; i <= 1; ++i) {
    for (int j = 0; j < cols; ++j) {
      picture_output[rows_mean + i][j][color] = 255;
    }
  }
}

void *filter_color(void *arg) {
  long color = (long) arg;
  filter_smoothing(color);
  filter_reflex_color(color);
  add_cross(color);
  pthread_exit(NULL);
}

void filter_parallel_horizontal() {
  pthread_t *horizontal_threads = new pthread_t[3];
  for (long i = 0; i < 3; ++i) {
    bool error = pthread_create(&horizontal_threads[i], NULL,
                                filter_horizontal, (void *) i);
    if (error) {
      cout << "Error creating thread" << endl;
      exit(1);
    }
  }
  for (long i = 0; i < 3; ++i) {
    bool error = pthread_join(horizontal_threads[i], NULL);
    if (error) {
      cout << "Error joining thread" << endl;
      exit(1);
    }
  }
}

void filter_parallel_vertical() {
  pthread_t *vertical_threads = new pthread_t[3];
  for (long i = 0; i < 3; ++i) {
    bool error = pthread_create(&vertical_threads[i], NULL,
                                filter_vertical, (void *) i);
    if (error) {
      cout << "Error creating thread" << endl;
      exit(1);
    }
  }
  for (long i = 0; i < 3; ++i) {
    bool error = pthread_join(vertical_threads[i], NULL);
    if (error) {
      cout << "Error joining thread" << endl;
      exit(1);
    }
  }
}

void filter_parallel_color() {
  pthread_t *color_threads = new pthread_t[3];
  for (long i = 0; i < 3; ++i) {
    bool error = pthread_create(&color_threads[i], NULL,
                                filter_color, (void *) i);
    if (error) {
      cout << "Error creating thread" << endl;
      exit(1);
    }
  }
  for (long i = 0; i < 3; ++i) {
    bool error = pthread_join(color_threads[i], NULL);
    if (error) {
      cout << "Error joining thread" << endl;
      exit(1);
    }
  }
}

int main(int argc, char *argv[]) {
  clock_t start_time, end_time;
  start_time = clock();
  char *fileBuffer;
  int bufferSize;
  char *fileName = argv[1];
  if (!fillAndAllocate(fileBuffer, fileName, rows, cols, bufferSize)) {
    cout << "File read error" << endl;
    return 1;
  }
  getPixlesFromBMP24(bufferSize, rows, cols, fileBuffer);
  filter_parallel_horizontal();
  filter_parallel_vertical();
  filter_parallel_color();
  writeOutBmp24(fileBuffer, FILE_OUT, bufferSize);
  end_time = clock();
  cout << "Execution Time: " << (int) (((double) (end_time - start_time) / CLOCKS_PER_SEC) * 1000) << endl;
  return 0;
}