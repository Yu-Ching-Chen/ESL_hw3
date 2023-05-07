#ifndef FILTER_PORT_H
#define FILTER_PORT_H
//
// input port
//
template <class T>
class port_in
{
    public:
        //
        // Member port
        //
        HLS_METAPORT;
        sc_in< T > next_row;
        //
        // Binding function
        //
        template <class C>
        void bind( C& c)
        {
            next_row(c.next_row);
        }
        template <class C>
        void operator() ( C& c) 
        {
            bind(c);    
        }
};

//
// output port
//
template <class T>
class port_out
{
    public:
        HLS_METAPORT;
        sc_out< T > next_row;
};

//
// channel
//
template <class T>
class port
{
    public:
        typedef port_in< T > in;
        typedef port_out< T > out;
        sc_signal< T > next_row;
};
#endif
