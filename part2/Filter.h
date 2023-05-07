#ifndef FILTER_H_
#define FILTER_H_
#include <systemc>
using namespace sc_core;

#include "filter_def.h"

#ifndef NATIVE_SYSTEMC
#include <cynw_p2p.h>
#include "defines.h"
#endif

class Filter : public sc_module {
public:
  sc_in_clk i_clk;
  sc_in< bool > i_rst;
  sc_in< bool > i_next_row;
#ifndef NATIVE_SYSTEMC
  cynw_p2p < one_pixel >::in i_data;
  cynw_p2p < one_pixel >::out o_data;
#else
  sc_fifo_in<unsigned char> i_r;
  sc_fifo_in<unsigned char> i_g;
  sc_fifo_in<unsigned char> i_b;
  sc_fifo_out<unsigned char> o_r;
  sc_fifo_out<unsigned char> o_g;
  sc_fifo_out<unsigned char> o_b;
#endif

  SC_HAS_PROCESS( Filter );
  Filter( sc_module_name n );
  ~Filter() = default;

private:
  void do_filter();
#ifndef NATIVE_SYSTEMC
  one_pixel i_function();
  bool i_ctr_function();
#endif
  sc_dt::sc_uint<COL_BITS> get_median(sc_dt::sc_uint<COL_BITS> buf[MASK_N]);
  sc_dt::sc_uint<COL_BITS> val_r[MASK_N];
  sc_dt::sc_uint<COL_BITS> val_g[MASK_N];
  sc_dt::sc_uint<COL_BITS> val_b[MASK_N];
  sc_dt::sc_uint<COL_BITS> in_buf_r[MASK_N];
  sc_dt::sc_uint<COL_BITS> in_buf_g[MASK_N];
  sc_dt::sc_uint<COL_BITS> in_buf_b[MASK_N];
  sc_dt::sc_uint<COL_BITS> median_buf_r[MASK_N];
  sc_dt::sc_uint<COL_BITS> median_buf_g[MASK_N];
  sc_dt::sc_uint<COL_BITS> median_buf_b[MASK_N];
  int mean_r;
  int mean_g;
  int mean_b;
};
#endif
