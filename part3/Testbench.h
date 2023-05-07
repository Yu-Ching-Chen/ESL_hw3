#ifndef TESTBENCH_H_
#define TESTBENCH_H_

#include <string>
#include <queue>
using namespace std;

#include <systemc>
using namespace sc_core;

#ifndef NATIVE_SYSTEMC
#include <cynw_p2p.h>
#include "defines.h"
#endif

#include "filter_def.h"

const int WHITE = 255;
const int BLACK = 0;
const int THRESHOLD = 90;

class Testbench : public sc_module {
public:
  sc_in_clk i_clk;
  sc_out<bool> o_rst;
  sc_out<bool> o_next_row;
#ifndef NATIVE_SYSTEMC
	cynw_p2p< one_pixel >::base_out o_data;
	cynw_p2p< one_pixel >::base_in i_data;
#else
  sc_fifo_out<unsigned char> o_r;
  sc_fifo_out<unsigned char> o_g;
  sc_fifo_out<unsigned char> o_b;
  sc_fifo_in<unsigned char> i_r;
  sc_fifo_in<unsigned char> i_g;
  sc_fifo_in<unsigned char> i_b;
#endif

  SC_HAS_PROCESS(Testbench);

  Testbench(sc_module_name n);
  ~Testbench();

  int read_bmp(string infile_name);
  int write_bmp(string outfile_name);

  unsigned int get_width() { return width; }

  unsigned int get_height() { return height; }

  unsigned int get_width_bytes() { return width_bytes; }

  unsigned int get_bytes_per_pixel() { return bytes_per_pixel; }

  unsigned char *get_source_image() { return source_bitmap; }
  unsigned char *get_target_image() { return target_bitmap; }

private:
  unsigned int input_rgb_raw_data_offset;
  const unsigned int output_rgb_raw_data_offset;
  int width;
  int height;
  unsigned int width_bytes;
  unsigned char bits_per_pixel;
  unsigned short bytes_per_pixel;
  unsigned char *source_bitmap;
  unsigned char *target_bitmap;
  queue<double> input_time_q;
  queue<double> output_time_q;

  unsigned int n_txn;
  sc_time max_txn_time;
  sc_time min_txn_time;
  sc_time total_txn_time;
  sc_time total_start_time;
  sc_time total_run_time;
  double latency;
  double throughput;

  void feed_data();
  void fetch_data();
  void latency_and_throughput();
};
#endif
