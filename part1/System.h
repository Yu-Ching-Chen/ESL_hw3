#ifndef SYSTEM_H_
#define SYSTEM_H_
#include <systemc>
using namespace sc_core;

#include "Testbench.h"

#ifndef NATIVE_SYSTEMC
#include "Filter_wrap.h"
#include "defines.h"
#else
#include "Filter.h"
#endif

class System: public sc_module
{
public:
	SC_HAS_PROCESS( System );
	System( sc_module_name n, std::string input_bmp, std::string output_bmp );
	~System();
private:
  Testbench tb;
#ifndef NATIVE_SYSTEMC
	Filter_wrapper filter;
#else
	Filter filter;
#endif
	sc_clock clk;
	sc_signal<bool> rst;
	sc_signal<bool> next_row;
#ifndef NATIVE_SYSTEMC
	cynw_p2p< one_pixel > rgb;
	cynw_p2p< one_pixel > result;
#else
	sc_fifo<unsigned char> data_r;
	sc_fifo<unsigned char> data_g;
	sc_fifo<unsigned char> data_b;
	sc_fifo<unsigned char> result_r;
	sc_fifo<unsigned char> result_g;
	sc_fifo<unsigned char> result_b;
#endif

	std::string _output_bmp;
};
#endif
