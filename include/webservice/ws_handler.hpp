//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__ws_handler__hpp_INCLUDED_
#define _webservice__ws_handler__hpp_INCLUDED_

#include "basic_ws_handler.hpp"


namespace webservice{


	class ws_handler
		: public basic_ws_handler< std::string, std::vector< std::uint8_t > >
	{
		using basic_ws_handler::basic_ws_handler;
	};


}


#endif