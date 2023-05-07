#include "System.h"

System::System( sc_module_name n, string input_bmp, string output_bmp ): sc_module( n ), 
	tb("tb"), filter("filter"), clk("clk", CLOCK_PERIOD, SC_NS), rst("rst"), _output_bmp(output_bmp)
{
	tb.i_clk(clk);
	tb.o_rst(rst);
	tb.o_next_row(next_row);
	filter.i_clk(clk);
	filter.i_rst(rst);
	filter.i_next_row(next_row);

#ifndef NATIVE_SYSTEMC
	tb.o_data(rgb);
	tb.i_data(result);
	filter.i_data(rgb);
	filter.o_data(result);
#else
	tb.i_r(result_r);
	tb.i_g(result_g);
	tb.i_b(result_b);
	tb.o_r(data_r);
	tb.o_g(data_g);
	tb.o_b(data_b);

	filter.i_r(data_r);
	filter.i_g(data_g);
	filter.i_b(data_b);
	filter.o_r(result_r);
	filter.o_g(result_g);
	filter.o_b(result_b);
#endif

  	tb.read_bmp(input_bmp);
}

System::~System() {
  tb.write_bmp(_output_bmp);
}
