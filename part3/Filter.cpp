#include <cmath>

#ifndef NATIVE_SYSTEMC
#include "stratus_hls.h" 
#endif

#include "Filter.h"
using namespace sc_dt;

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

      sc_uint<4> l = 0; // median buffer x index
      sc_uint<4> m = 0; // midian buffer y index

      // row 1
      sc_uint<4> i = 0;
      for (sc_uint<4> w = 0; w < MASK_Y; ++w) {
        for (sc_int<5> v = 0; v < MASK_X; ++v) {
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
        for (sc_uint<4> w = 0; w < MASK_Y; ++w) {
          for (sc_int<5> v = 0; v < MASK_X; ++v) {
#ifndef NATIVE_SYSTEMC
            HLS_UNROLL_LOOP(ON);
#endif
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
      for (sc_uint<4> w = 0; w < MASK_Y; ++w) {
        for (sc_int<5> v = 0; v < MASK_X; ++v) {
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
        for (sc_uint<4> w = 0; w < MASK_Y; ++w) {
          for (sc_int<5> v = MASK_X - 1; v >= 0; --v) {
#ifndef NATIVE_SYSTEMC
            HLS_UNROLL_LOOP(ON);
#endif
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
      for (sc_uint<4> w = 0; w < MASK_Y; ++w) {
        for (sc_int<5> v = 0; v < MASK_X; ++v) {
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
        for (sc_uint<4> w = 0; w < MASK_Y; ++w) {
          for (sc_int<5> v = 0; v < MASK_X; ++v) {
#ifndef NATIVE_SYSTEMC
            HLS_UNROLL_LOOP(ON);
#endif
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
      for (sc_uint<4> w = 0; w < MASK_Y; ++w) {
#ifndef NATIVE_SYSTEMC
        HLS_UNROLL_LOOP(ALL);
#endif
        for (sc_int<5> v = 0; v < MASK_X - 1; ++v) {
          median_buf_r[MASK_X * w + v] = median_buf_r[MASK_X * w + (v + 1)];
          median_buf_g[MASK_X * w + v] = median_buf_g[MASK_X * w + (v + 1)];
          median_buf_b[MASK_X * w + v] = median_buf_b[MASK_X * w + (v + 1)];
        }
      }
      sc_uint<4> l = MASK_X - 1; // median buffer x index
      sc_uint<4> m = 0; // midian buffer y index

      for (m = 0; m < MASK_Y; ++m) {
        if (m == 0) {
          for (sc_uint<4> w = 0; w < MASK_Y; ++w) {
            for (sc_int<5> v = 0; v < MASK_X; ++v) {
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
          for (sc_uint<4> w = 0; w < MASK_Y; ++w) {
#ifndef NATIVE_SYSTEMC
            HLS_UNROLL_LOOP(ON);
#endif
            for (sc_int<5> v = 0; v < MASK_X; ++v) {
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
    for (sc_uint<4> w = 0; w < MASK_Y; ++w) {
      for (sc_int<5> v = 0; v < MASK_X; ++v) {
        if (w == 1 && v == 1) {
            mean_r = mean_r + (median_buf_r[MASK_X * w + v] << 1);
            mean_g = mean_g + (median_buf_g[MASK_X * w + v] << 1);
            mean_b = mean_b + (median_buf_b[MASK_X * w + v] << 1);
        } else {
            mean_r = mean_r + median_buf_r[MASK_X * w + v];
            mean_g = mean_g + median_buf_g[MASK_X * w + v];
            mean_b = mean_b + median_buf_b[MASK_X * w + v];
        }
      }
    }

#ifndef NATIVE_SYSTEMC
    {
      one_pixel data;
      HLS_DEFINE_PROTOCOL("output");
      data.range(3 * COL_BITS - 1, 2 * COL_BITS) = ((mean_r << 2) - mean_r) >> 5;
      data.range(2 * COL_BITS - 1, COL_BITS) = ((mean_g << 2) - mean_g) >> 5;
      data.range(COL_BITS - 1, 0) = ((mean_b << 2) - mean_b) >> 5;
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

