#include <cassert>
#include <cstdio>
#include <cstdlib>
using namespace std;

#include "Testbench.h"

unsigned char header[54] = {
    0x42,          // identity : B
    0x4d,          // identity : M
    0,    0, 0, 0, // file size
    0,    0,       // reserved1
    0,    0,       // reserved2
    54,   0, 0, 0, // RGB data offset
    40,   0, 0, 0, // struct BITMAPINFOHEADER size
    0,    0, 0, 0, // bmp width
    0,    0, 0, 0, // bmp height
    1,    0,       // planes
    24,   0,       // bit per pixel
    0,    0, 0, 0, // compression
    0,    0, 0, 0, // data size
    0,    0, 0, 0, // h resolution
    0,    0, 0, 0, // v resolution
    0,    0, 0, 0, // used colors
    0,    0, 0, 0  // important colors
};

Testbench::Testbench(sc_module_name n) : sc_module(n), output_rgb_raw_data_offset(54) {
  SC_THREAD(feed_data);
  sensitive << i_clk.pos();
  dont_initialize();
  SC_THREAD(fetch_data);
  sensitive << i_clk.pos();
  dont_initialize();
}

Testbench::~Testbench() {
  cout << "Total run time = " << total_run_time << endl;
  cout << "Average latency = " << latency << " cycles" << endl;
  cout << "Average throughput = " << throughput / 1e6 << "M pixel/sec" << endl;
}

int Testbench::read_bmp(string infile_name) {
  FILE *fp_s = NULL; // source file handler
  fp_s = fopen(infile_name.c_str(), "rb");
  if (fp_s == NULL) {
    printf("fopen %s error\n", infile_name.c_str());
    return -1;
  }
  // move offset to 10 to find rgb raw data offset
  fseek(fp_s, 10, SEEK_SET);
  assert(fread(&input_rgb_raw_data_offset, sizeof(unsigned int), 1, fp_s));

  // move offset to 18 to get width & height;
  fseek(fp_s, 18, SEEK_SET);
  assert(fread(&width, sizeof(unsigned int), 1, fp_s));
  assert(fread(&height, sizeof(unsigned int), 1, fp_s));

  // get bit per pixel
  fseek(fp_s, 28, SEEK_SET);
  assert(fread(&bits_per_pixel, sizeof(unsigned short), 1, fp_s));
  bytes_per_pixel = bits_per_pixel / 8;

  // move offset to input_rgb_raw_data_offset to get RGB raw data
  fseek(fp_s, input_rgb_raw_data_offset, SEEK_SET);

  source_bitmap =
      (unsigned char *)malloc((size_t)width * height * bytes_per_pixel);
  if (source_bitmap == NULL) {
    printf("malloc images_s error\n");
    return -1;
  }

  target_bitmap =
      (unsigned char *)malloc((size_t)width * height * bytes_per_pixel);
  if (target_bitmap == NULL) {
    printf("malloc target_bitmap error\n");
    return -1;
  }

  printf("Image width=%d, height=%d\n", width, height);
  assert(fread(source_bitmap, sizeof(unsigned char),
               (size_t)(long)width * height * bytes_per_pixel, fp_s));
  fclose(fp_s);

  return 0;
}

int Testbench::write_bmp(string outfile_name) {
  FILE *fp_t = NULL;      // target file handler
  unsigned int file_size; // file size

  fp_t = fopen(outfile_name.c_str(), "wb");
  if (fp_t == NULL) {
    printf("fopen %s error\n", outfile_name.c_str());
    return -1;
  }

  // file size
  file_size = width * height * bytes_per_pixel + output_rgb_raw_data_offset;
  header[2] = (unsigned char)(file_size & 0x000000ff);
  header[3] = (file_size >> 8) & 0x000000ff;
  header[4] = (file_size >> 16) & 0x000000ff;
  header[5] = (file_size >> 24) & 0x000000ff;

  // width
  header[18] = width & 0x000000ff;
  header[19] = (width >> 8) & 0x000000ff;
  header[20] = (width >> 16) & 0x000000ff;
  header[21] = (width >> 24) & 0x000000ff;

  // height
  header[22] = height & 0x000000ff;
  header[23] = (height >> 8) & 0x000000ff;
  header[24] = (height >> 16) & 0x000000ff;
  header[25] = (height >> 24) & 0x000000ff;

  // bit per pixel
  header[28] = bits_per_pixel;

  // write header
  fwrite(header, sizeof(unsigned char), output_rgb_raw_data_offset, fp_t);

  // write image
  fwrite(target_bitmap, sizeof(unsigned char),
         (size_t)(long)width * height * bytes_per_pixel, fp_t);

  fclose(fp_t);
  return 0;
}

void Testbench::feed_data() {
  int x, y, v, u, w, l;        // for loop counter
  unsigned char R, G, B; // color of R, G, B
  int adjustX, adjustY, xBound, yBound;
  int total;

#ifndef NATIVE_SYSTEMC
  o_data.reset();
#endif
  o_rst.write(false);
  wait(2);
  o_next_row.write(true);
  o_rst.write(true);
  wait(1);
  total_start_time = sc_time_stamp();
  for (y = 0; y != height; ++y) {
    for (x = 0; x != width; ++x) {      
      input_time_q.push(sc_time_stamp().to_double());
      adjustX = (MASK_X % 2) ? 1 : 0; // 1
      adjustY = (MASK_Y % 2) ? 1 : 0; // 1
      xBound = MASK_X / 2;            // 1
      yBound = MASK_Y / 2;            // 1

      if (x == 0) {
        v = -yBound;
        u = -xBound;
        // row 1
        for (w = -yBound; w != yBound + adjustY; ++w) { // -1, 0, 1
          for (l = -xBound; l != xBound + adjustX; ++l) { // -1, 0, 1
            if (x + u + l >= 0 && x + u + l < width && y + v + w >= 0 && y + v + w < height) {
              R = *(source_bitmap +
                    bytes_per_pixel * (width * (y + v + w) + (x + u + l)) + 2);
              G = *(source_bitmap +
                    bytes_per_pixel * (width * (y + v + w) + (x + u + l)) + 1);
              B = *(source_bitmap +
                    bytes_per_pixel * (width * (y + v + w) + (x + u + l)) + 0);
            } else {
              R = 0;
              G = 0;
              B = 0;
            }
            one_pixel data;
#ifndef NATIVE_SYSTEMC
            data.range(3 * COL_BITS - 1, 2 * COL_BITS) = R;
            data.range(2 * COL_BITS - 1, COL_BITS) = G;
            data.range(COL_BITS - 1, 0) = B;
            o_data.put(data);
#else
            o_r.write(R);
            o_g.write(G);
            o_b.write(B);
#endif
          }
        }

        ++u;
        l = xBound + adjustX - 1;
        while (u != xBound + adjustX) {
          for (w = -yBound; w != yBound + adjustY; ++w) {
            if (x + u + l >= 0 && x + u + l < width && y + v + w >= 0 && y + v + w < height) {
              R = *(source_bitmap +
                    bytes_per_pixel * (width * (y + v + w) + (x + u + l)) + 2);
              G = *(source_bitmap +
                    bytes_per_pixel * (width * (y + v + w) + (x + u + l)) + 1);
              B = *(source_bitmap +
                    bytes_per_pixel * (width * (y + v + w) + (x + u + l)) + 0);
            } else {
              R = 0;
              G = 0;
              B = 0;
            }
            one_pixel data;
#ifndef NATIVE_SYSTEMC
            data.range(3 * COL_BITS - 1, 2 * COL_BITS) = R;
            data.range(2 * COL_BITS - 1, COL_BITS) = G;
            data.range(COL_BITS - 1, 0) = B;
            o_data.put(data);
#else
            o_r.write(R);
            o_g.write(G);
            o_b.write(B);
#endif
          }
          ++u;
        }

        // row 2
        --u;
        ++v;
        w = yBound + adjustY - 1;
        for (l = -xBound; l != xBound + adjustX; ++l) {
          if (x + u + l >= 0 && x + u + l < width && y + v + w >= 0 && y + v + w < height) {
            R = *(source_bitmap +
                  bytes_per_pixel * (width * (y + v + w) + (x + u + l)) + 2);
            G = *(source_bitmap +
                  bytes_per_pixel * (width * (y + v + w) + (x + u + l)) + 1);
            B = *(source_bitmap +
                  bytes_per_pixel * (width * (y + v + w) + (x + u + l)) + 0);
          } else {
            R = 0;
            G = 0;
            B = 0;
          }
          one_pixel data;
#ifndef NATIVE_SYSTEMC
          data.range(3 * COL_BITS - 1, 2 * COL_BITS) = R;
          data.range(2 * COL_BITS - 1, COL_BITS) = G;
          data.range(COL_BITS - 1, 0) = B;
          o_data.put(data);
#else
          o_r.write(R);
          o_g.write(G);
          o_b.write(B);
#endif
        }

        --u;
        l = -xBound;
        while (u != -(xBound + adjustX)) {
          for (w = -yBound; w != yBound + adjustY; ++w) {
            if (x + u + l >= 0 && x + u + l < width && y + v + w >= 0 && y + v + w < height) {
              R = *(source_bitmap +
                    bytes_per_pixel * (width * (y + v + w) + (x + u + l)) + 2);
              G = *(source_bitmap +
                    bytes_per_pixel * (width * (y + v + w) + (x + u + l)) + 1);
              B = *(source_bitmap +
                    bytes_per_pixel * (width * (y + v + w) + (x + u + l)) + 0);
            } else {
              R = 0;
              G = 0;
              B = 0;
            }
            one_pixel data;
#ifndef NATIVE_SYSTEMC
            data.range(3 * COL_BITS - 1, 2 * COL_BITS) = R;
            data.range(2 * COL_BITS - 1, COL_BITS) = G;
            data.range(COL_BITS - 1, 0) = B;
            o_data.put(data);
#else
            o_r.write(R);
            o_g.write(G);
            o_b.write(B);
#endif
          }
          --u;
                      
        }


        // row 3
        ++u;
        ++v;
        w = yBound + adjustY - 1;
        for (l = -xBound; l != xBound + adjustX; ++l) {
          if (x + u + l >= 0 && x + u + l < width && y + v + w >= 0 && y + v + w < height) {
            R = *(source_bitmap +
                  bytes_per_pixel * (width * (y + v + w) + (x + u + l)) + 2);
            G = *(source_bitmap +
                  bytes_per_pixel * (width * (y + v + w) + (x + u + l)) + 1);
            B = *(source_bitmap +
                  bytes_per_pixel * (width * (y + v + w) + (x + u + l)) + 0);
          } else {
            R = 0;
            G = 0;
            B = 0;
          }
          one_pixel data;
#ifndef NATIVE_SYSTEMC
          data.range(3 * COL_BITS - 1, 2 * COL_BITS) = R;
          data.range(2 * COL_BITS - 1, COL_BITS) = G;
          data.range(COL_BITS - 1, 0) = B;
          o_data.put(data);
#else
          o_r.write(R);
          o_g.write(G);
          o_b.write(B);
#endif
        }

        ++u;
        l = xBound + adjustX - 1;
        while (u != xBound + adjustX) {
          for (w = -yBound; w != yBound + adjustY; ++w) {
            if (x + u + l >= 0 && x + u + l < width && y + v + w >= 0 && y + v + w < height) {
              R = *(source_bitmap +
                    bytes_per_pixel * (width * (y + v + w) + (x + u + l)) + 2);
              G = *(source_bitmap +
                    bytes_per_pixel * (width * (y + v + w) + (x + u + l)) + 1);
              B = *(source_bitmap +
                    bytes_per_pixel * (width * (y + v + w) + (x + u + l)) + 0);
            } else {
              R = 0;
              G = 0;
              B = 0;
            }
            one_pixel data;
#ifndef NATIVE_SYSTEMC
            data.range(3 * COL_BITS - 1, 2 * COL_BITS) = R;
            data.range(2 * COL_BITS - 1, COL_BITS) = G;
            data.range(COL_BITS - 1, 0) = B;
            o_data.put(data);
#else
            o_r.write(R);
            o_g.write(G);
            o_b.write(B);
#endif
          }
          ++u;
        }
      } else {
          u = xBound + adjustX - 1;
          for (v = -yBound; v != yBound + adjustY; ++v) {
            if (v == -yBound) {
              for (w = -yBound; w != yBound + adjustY; ++w) { // -1, 0, 1
                for (l = -xBound; l != xBound + adjustX; ++l) { // -1, 0, 1
                  if (x + u + l >= 0 && x + u + l < width && y + v + w >= 0 && y + v + w < height) {
                    R = *(source_bitmap +
                          bytes_per_pixel * (width * (y + v + w) + (x + u + l)) + 2);
                    G = *(source_bitmap +
                          bytes_per_pixel * (width * (y + v + w) + (x + u + l)) + 1);
                    B = *(source_bitmap +
                          bytes_per_pixel * (width * (y + v + w) + (x + u + l)) + 0);
                  } else {
                    R = 0;
                    G = 0;
                    B = 0;
                  }
                  one_pixel data;
#ifndef NATIVE_SYSTEMC
                  data.range(3 * COL_BITS - 1, 2 * COL_BITS) = R;
                  data.range(2 * COL_BITS - 1, COL_BITS) = G;
                  data.range(COL_BITS - 1, 0) = B;
                  o_data.put(data);
  #else
                  o_r.write(R);
                  o_g.write(G);
                  o_b.write(B);
#endif
              }
            }
          } else {
            w = yBound + adjustY - 1;
            for (l = -xBound; l != xBound + adjustX; ++l) {
              if (x + u + l >= 0 && x + u + l < width && y + v + w >= 0 && y + v + w < height) {
                R = *(source_bitmap +
                      bytes_per_pixel * (width * (y + v + w) + (x + u + l)) + 2);
                G = *(source_bitmap +
                      bytes_per_pixel * (width * (y + v + w) + (x + u + l)) + 1);
                B = *(source_bitmap +
                      bytes_per_pixel * (width * (y + v + w) + (x + u + l)) + 0);
              } else {
                R = 0;
                G = 0;
                B = 0;
              }
              one_pixel data;
#ifndef NATIVE_SYSTEMC
              data.range(3 * COL_BITS - 1, 2 * COL_BITS) = R;
              data.range(2 * COL_BITS - 1, COL_BITS) = G;
              data.range(COL_BITS - 1, 0) = B;
              o_data.put(data);
#else
              o_r.write(R);
              o_g.write(G);
              o_b.write(B);
#endif
            }
          }
        }
      }

      if (x == width - 1) {
        o_next_row.write(true);
      } else {
        o_next_row.write(false);
      }

      // cout << "Now at " << sc_time_stamp() << endl; //print current sc_time
    }
  }
}

void Testbench::fetch_data() {
  unsigned int x, y; // for loop counter
#ifndef NATIVE_SYSTEMC
  one_pixel data;
  i_data.reset();
#endif
  wait(2);
  wait(1);
  for (y = 0; y != height; ++y) {
    for (x = 0; x != width; ++x) {
#ifndef NATIVE_SYSTEMC
      data = i_data.get();
      output_time_q.push(sc_time_stamp().to_double());
      *(target_bitmap + bytes_per_pixel * (width * y + x) + 2) = data.range(3 * COL_BITS - 1, 2 * COL_BITS);
      *(target_bitmap + bytes_per_pixel * (width * y + x) + 1) = data.range(2 * COL_BITS - 1, COL_BITS);
      *(target_bitmap + bytes_per_pixel * (width * y + x) + 0) = data.range(COL_BITS - 1, 0);
#else
      if(i_r.num_available()==0) wait(i_r.data_written_event());
      *(target_bitmap + bytes_per_pixel * (width * y + x) + 2) = i_r.read();
      if(i_g.num_available()==0) wait(i_g.data_written_event());
      *(target_bitmap + bytes_per_pixel * (width * y + x) + 1) = i_g.read();
      if(i_b.num_available()==0) wait(i_b.data_written_event());
      *(target_bitmap + bytes_per_pixel * (width * y + x) + 0) = i_b.read();
#endif
    }
  }  
  total_run_time = sc_time_stamp() - total_start_time;
  latency_and_throughput();
  sc_stop();
}

void Testbench::latency_and_throughput() {
    long cnt = 0;
    double lat = 0;
    double thr = 0;
    double last;
    double tmp1;
    double tmp2;
    while(!input_time_q.empty()) {
        tmp1 = input_time_q.front();
        tmp2 = output_time_q.front();
        input_time_q.pop();
        output_time_q.pop();
        lat = lat + (tmp2 - tmp1) / 1e4;
        if (cnt != 0) thr = thr + 1e12 / (tmp2 - last);
        last = tmp2;
        cnt = cnt + 1;
    }
    latency = lat / (double) cnt;
    throughput = thr / (double) (cnt - 1);
    return;
}
