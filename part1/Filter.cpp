#include <cmath>

#ifndef NATIVE_SYSTEMC
#include "stratus_hls.h" 
#endif

#include "Filter.h"

Filter::Filter(sc_module_name n) : sc_module(n) {
#ifndef NATIVE_SYSTEMC
  HLS_FLATTEN_ARRAY(val_r);
  HLS_FLATTEN_ARRAY(val_g);
  HLS_FLATTEN_ARRAY(val_b);
  HLS_FLATTEN_ARRAY(in_buf_r);
  HLS_FLATTEN_ARRAY(in_buf_g);
  HLS_FLATTEN_ARRAY(in_buf_b);
  HLS_FLATTEN_ARRAY(median_buf_r);
  HLS_FLATTEN_ARRAY(median_buf_g);
  HLS_FLATTEN_ARRAY(median_buf_b);
#endif
  SC_THREAD(do_filter);
  sensitive << i_clk.pos();
  dont_initialize();
  reset_signal_is(i_rst, false);

#ifndef NATIVE_SYSTEMC
  i_data.clk_rst(i_clk, i_rst);
  o_data.clk_rst(i_clk, i_rst);
#endif
}

// sobel mask
const int mean_mask[MASK_Y][MASK_X] = {{1, 1, 1}, {1, 2, 1}, {1, 1, 1}};

sc_dt::sc_uint<COL_BITS> Filter::get_median(sc_dt::sc_uint<COL_BITS> buf[MASK_N]) {
  int i;
  unsigned char tmp1;
  unsigned char tmp2;
  for (i = 0; i < MASK_N - 1; i++) {
    tmp1 = buf[i];
    tmp2 = buf[i + 1];
    if (tmp1 > tmp2) {
      buf[i] = tmp2;
      buf[i + 1] = tmp1;
    }
  }
  return buf[MASK_N / 2];
}

#ifndef NATIVE_SYSTEMC
one_pixel Filter::i_function() {
    one_pixel data;
    HLS_DEFINE_PROTOCOL("input");
    data = i_data.get();
    wait();
    return data;
}

bool Filter::i_ctr_function() {
    bool ctr;
    HLS_DEFINE_PROTOCOL("control");
    ctr = i_next_row.read();
    return ctr;
}
#endif

void Filter::do_filter() {
  {
#ifndef NATIVE_SYSTEMC
    HLS_DEFINE_PROTOCOL("reset");
    i_data.reset();
    o_data.reset();
#endif
    wait();
  }
  while (true) {
#ifndef NATIVE_SYSTEMC
    if (i_ctr_function()) {
#else
    if (i_next_row.read()) {
#endif

      int l = 0; // median buffer x index
      int m = 0; // midian buffer y index

      // row 1
      int i = 0;
      for (int w = 0; w < MASK_Y; ++w) {
        for (int v = 0; v < MASK_X; ++v) {
#ifndef NATIVE_SYSTEMC
          one_pixel data;
          data = i_function();
          in_buf_r[MASK_X * w + v] = data.range(3 * COL_BITS - 1, 2 * COL_BITS);
          in_buf_g[MASK_X * w + v] = data.range(2 * COL_BITS - 1, COL_BITS);
          in_buf_b[MASK_X * w + v] = data.range(COL_BITS - 1, 0);
#else
          in_buf_r[MASK_X * w + v] = i_r.read();
          in_buf_g[MASK_X * w + v] = i_g.read();
          in_buf_b[MASK_X * w + v] = i_b.read();
#endif
          val_r[MASK_X * w + v] = in_buf_r[MASK_X * w + v];
          val_g[MASK_X * w + v] = in_buf_g[MASK_X * w + v];
          val_b[MASK_X * w + v] = in_buf_b[MASK_X * w + v];
        }
      }

      median_buf_r[MASK_X * m + l] = get_median(val_r);
      median_buf_g[MASK_X * m + l] = get_median(val_g);
      median_buf_b[MASK_X * m + l] = get_median(val_b);
      
      i = i + 1;
      l++;

      while (i < 3) {
        for (int w = 0; w < MASK_Y; ++w) {
          for (int v = 0; v < MASK_X; ++v) {
            if (v == MASK_X - 1) {
#ifndef NATIVE_SYSTEMC
              one_pixel data;
              data = i_function();
              in_buf_r[MASK_X * w + v] = data.range(3 * COL_BITS - 1, 2 * COL_BITS);
              in_buf_g[MASK_X * w + v] = data.range(2 * COL_BITS - 1, COL_BITS);
              in_buf_b[MASK_X * w + v] = data.range(COL_BITS - 1, 0);
#else
              in_buf_r[MASK_X * w + v] = i_r.read();
              in_buf_g[MASK_X * w + v] = i_g.read();
              in_buf_b[MASK_X * w + v] = i_b.read();
#endif
            } else {
              in_buf_r[MASK_X * w + v] = in_buf_r[MASK_X * w + (v + 1)];
              in_buf_g[MASK_X * w + v] = in_buf_g[MASK_X * w + (v + 1)];
              in_buf_b[MASK_X * w + v] = in_buf_b[MASK_X * w + (v + 1)];
            }
            val_r[MASK_X * w + v] = in_buf_r[MASK_X * w + v];
            val_g[MASK_X * w + v] = in_buf_g[MASK_X * w + v];
            val_b[MASK_X * w + v] = in_buf_b[MASK_X * w + v];
          }
        }

        median_buf_r[MASK_X * m + l] = get_median(val_r);
        median_buf_g[MASK_X * m + l] = get_median(val_g);
        median_buf_b[MASK_X * m + l] = get_median(val_b);

        i = i + 1;
        l++;
      }

      m++;
      l--;
      // row 2
      for (int w = 0; w < MASK_Y; ++w) {
        for (int v = 0; v < MASK_X; ++v) {
            if (w == MASK_Y - 1) {
#ifndef NATIVE_SYSTEMC
              one_pixel data;
              data = i_function();
              in_buf_r[MASK_X * w + v] = data.range(3 * COL_BITS - 1, 2 * COL_BITS);
              in_buf_g[MASK_X * w + v] = data.range(2 * COL_BITS - 1, COL_BITS);
              in_buf_b[MASK_X * w + v] = data.range(COL_BITS - 1, 0);
#else
              in_buf_r[MASK_X * w + v] = i_r.read();
              in_buf_g[MASK_X * w + v] = i_g.read();
              in_buf_b[MASK_X * w + v] = i_b.read();
#endif
            } else {
              in_buf_r[MASK_X * w + v] = in_buf_r[MASK_X * (w + 1) + v];
              in_buf_g[MASK_X * w + v] = in_buf_g[MASK_X * (w + 1) + v];
              in_buf_b[MASK_X * w + v] = in_buf_b[MASK_X * (w + 1) + v];
            }
            val_r[MASK_X * w + v] = in_buf_r[MASK_X * w + v];
            val_g[MASK_X * w + v] = in_buf_g[MASK_X * w + v];
            val_b[MASK_X * w + v] = in_buf_b[MASK_X * w + v];
        }
      }

        median_buf_r[MASK_X * m + l] = get_median(val_r);
        median_buf_g[MASK_X * m + l] = get_median(val_g);
        median_buf_b[MASK_X * m + l] = get_median(val_b);

      i = i + 1; // i = 4
      l--;

      while (i < 6) {
        for (int w = 0; w < MASK_Y; ++w) {
          for (int v = MASK_X - 1; v >= 0; --v) {
            if (v == 0) {
#ifndef NATIVE_SYSTEMC
              one_pixel data;
              data = i_function();
              in_buf_r[MASK_X * w + v] = data.range(3 * COL_BITS - 1, 2 * COL_BITS);
              in_buf_g[MASK_X * w + v] = data.range(2 * COL_BITS - 1, COL_BITS);
              in_buf_b[MASK_X * w + v] = data.range(COL_BITS - 1, 0);
#else
              in_buf_r[MASK_X * w + v] = i_r.read();
              in_buf_g[MASK_X * w + v] = i_g.read();
              in_buf_b[MASK_X * w + v] = i_b.read();
#endif
            } else {
              in_buf_r[MASK_X * w + v] = in_buf_r[MASK_X * w + (v - 1)];
              in_buf_g[MASK_X * w + v] = in_buf_g[MASK_X * w + (v - 1)];
              in_buf_b[MASK_X * w + v] = in_buf_b[MASK_X * w + (v - 1)];
            }
            val_r[MASK_X * w + v] = in_buf_r[MASK_X * w + v];
            val_g[MASK_X * w + v] = in_buf_g[MASK_X * w + v];
            val_b[MASK_X * w + v] = in_buf_b[MASK_X * w + v];                
          }
        }

        median_buf_r[MASK_X * m + l] = get_median(val_r);
        median_buf_g[MASK_X * m + l] = get_median(val_g);
        median_buf_b[MASK_X * m + l] = get_median(val_b);

        i = i + 1;
        l--;
      }

      m++;
      l++;

      // row 3
      for (int w = 0; w < MASK_Y; ++w) {
        for (int v = 0; v < MASK_X; ++v) {
            if (w == MASK_Y - 1) {
#ifndef NATIVE_SYSTEMC
              one_pixel data;
              data = i_function();
              in_buf_r[MASK_X * w + v] = data.range(3 * COL_BITS - 1, 2 * COL_BITS);
              in_buf_g[MASK_X * w + v] = data.range(2 * COL_BITS - 1, COL_BITS);
              in_buf_b[MASK_X * w + v] = data.range(COL_BITS - 1, 0);
#else
              in_buf_r[MASK_X * w + v] = i_r.read();
              in_buf_g[MASK_X * w + v] = i_g.read();
              in_buf_b[MASK_X * w + v] = i_b.read();
#endif
            } else {
              in_buf_r[MASK_X * w + v] = in_buf_r[MASK_X * (w + 1) + v];
              in_buf_g[MASK_X * w + v] = in_buf_g[MASK_X * (w + 1) + v];
              in_buf_b[MASK_X * w + v] = in_buf_b[MASK_X * (w + 1) + v];
            }
            val_r[MASK_X * w + v] = in_buf_r[MASK_X * w + v];
            val_g[MASK_X * w + v] = in_buf_g[MASK_X * w + v];
            val_b[MASK_X * w + v] = in_buf_b[MASK_X * w + v];      
        }
      }

      median_buf_r[MASK_X * m + l] = get_median(val_r);
      median_buf_g[MASK_X * m + l] = get_median(val_g);
      median_buf_b[MASK_X * m + l] = get_median(val_b);

      i = i + 1; // i = 7
      l++;

      while (i < 9) {
        for (int w = 0; w < MASK_Y; ++w) {
          for (int v = 0; v < MASK_X; ++v) {
            if (v == MASK_X - 1) {
#ifndef NATIVE_SYSTEMC
              one_pixel data;
              data = i_function();
              in_buf_r[MASK_X * w + v] = data.range(3 * COL_BITS - 1, 2 * COL_BITS);
              in_buf_g[MASK_X * w + v] = data.range(2 * COL_BITS - 1, COL_BITS);
              in_buf_b[MASK_X * w + v] = data.range(COL_BITS - 1, 0);
#else
              in_buf_r[MASK_X * w + v] = i_r.read();
              in_buf_g[MASK_X * w + v] = i_g.read();
              in_buf_b[MASK_X * w + v] = i_b.read();
#endif
            } else {
              in_buf_r[MASK_X * w + v] = in_buf_r[MASK_X * w + (v + 1)];
              in_buf_g[MASK_X * w + v] = in_buf_g[MASK_X * w + (v + 1)];
              in_buf_b[MASK_X * w + v] = in_buf_b[MASK_X * w + (v + 1)];
            }
            val_r[MASK_X * w + v] = in_buf_r[MASK_X * w + v];
            val_g[MASK_X * w + v] = in_buf_g[MASK_X * w + v];
            val_b[MASK_X * w + v] = in_buf_b[MASK_X * w + v];   
          }
        }

        median_buf_r[MASK_X * m + l] = get_median(val_r);
        median_buf_g[MASK_X * m + l] = get_median(val_g);
        median_buf_b[MASK_X * m + l] = get_median(val_b);
        i = i + 1;
        l++;
      }
    } else {
      for (int w = 0; w < MASK_Y; ++w) {
        for (int v = 0; v < MASK_X - 1; ++v) {
          median_buf_r[MASK_X * w + v] = median_buf_r[MASK_X * w + (v + 1)];
          median_buf_g[MASK_X * w + v] = median_buf_g[MASK_X * w + (v + 1)];
          median_buf_b[MASK_X * w + v] = median_buf_b[MASK_X * w + (v + 1)];
        }
      }
      int l = MASK_X - 1; // median buffer x index
      int m = 0; // midian buffer y index

      for (m = 0; m < MASK_Y; ++m) {
        if (m == 0) {
          for (int w = 0; w < MASK_Y; ++w) {
            for (int v = 0; v < MASK_X; ++v) {
#ifndef NATIVE_SYSTEMC
              one_pixel data;
              data = i_function();
              in_buf_r[MASK_X * w + v] = data.range(3 * COL_BITS - 1, 2 * COL_BITS);
              in_buf_g[MASK_X * w + v] = data.range(2 * COL_BITS - 1, COL_BITS);
              in_buf_b[MASK_X * w + v] = data.range(COL_BITS - 1, 0);
#else
              in_buf_r[MASK_X * w + v] = i_r.read();
              in_buf_g[MASK_X * w + v] = i_g.read();
              in_buf_b[MASK_X * w + v] = i_b.read();
#endif
              val_r[MASK_X * w + v] = in_buf_r[MASK_X * w + v];
              val_g[MASK_X * w + v] = in_buf_g[MASK_X * w + v];
              val_b[MASK_X * w + v] = in_buf_b[MASK_X * w + v];   
            }
          }
        } else {
          for (int w = 0; w < MASK_Y; ++w) {
            for (int v = 0; v < MASK_X; ++v) {
                if (w == MASK_Y - 1) {
#ifndef NATIVE_SYSTEMC
                  one_pixel data;
                  data = i_function();
                  in_buf_r[MASK_X * w + v] = data.range(3 * COL_BITS - 1, 2 * COL_BITS);
                  in_buf_g[MASK_X * w + v] = data.range(2 * COL_BITS - 1, COL_BITS);
                  in_buf_b[MASK_X * w + v] = data.range(COL_BITS - 1, 0);
#else
                  in_buf_r[MASK_X * w + v] = i_r.read();
                  in_buf_g[MASK_X * w + v] = i_g.read();
                  in_buf_b[MASK_X * w + v] = i_b.read();
#endif
                } else {
                  in_buf_r[MASK_X * w + v] = in_buf_r[MASK_X * (w + 1) + v];
                  in_buf_g[MASK_X * w + v] = in_buf_g[MASK_X * (w + 1) + v];
                  in_buf_b[MASK_X * w + v] = in_buf_b[MASK_X * (w + 1) + v];
                }
                val_r[MASK_X * w + v] = in_buf_r[MASK_X * w + v];
                val_g[MASK_X * w + v] = in_buf_g[MASK_X * w + v];
                val_b[MASK_X * w + v] = in_buf_b[MASK_X * w + v]; 
            }
          }
        }
        median_buf_r[MASK_X * m + l] = get_median(val_r);
        median_buf_g[MASK_X * m + l] = get_median(val_g);
        median_buf_b[MASK_X * m + l] = get_median(val_b);
      }
    }

    mean_r = 0;
    mean_g = 0;
    mean_b = 0;
    for (int w = 0; w < MASK_Y; ++w) {
      for (int v = 0; v < MASK_X; ++v) {
        mean_r = mean_r + (median_buf_r[MASK_X * w + v] * mean_mask[w][v]);
        mean_g = mean_g + (median_buf_g[MASK_X * w + v] * mean_mask[w][v]);
        mean_b = mean_b + (median_buf_b[MASK_X * w + v] * mean_mask[w][v]);
      }
    }

#ifndef NATIVE_SYSTEMC
    {
      one_pixel data;
      HLS_DEFINE_PROTOCOL("output");
      data.range(3 * COL_BITS - 1, 2 * COL_BITS) = mean_r / 10;
      data.range(2 * COL_BITS - 1, COL_BITS) = mean_g / 10;
      data.range(COL_BITS - 1, 0) = mean_b / 10;
      o_data.put(data);
      wait();
    }
#else
    o_r.write(mean_r / 10);
    o_g.write(mean_g / 10);
    o_b.write(mean_b / 10);
#endif
  }
}

