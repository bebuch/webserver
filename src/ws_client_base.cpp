//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webservice/ws_client_base.hpp>

#include <boost/make_unique.hpp>

#include "ws_client_base_impl.hpp"


namespace webservice{


	class ws_client_base_impl;


	ws_client_base::ws_client_base(
		std::string host,
		std::string port,
		std::string resource,
		boost::optional< std::chrono::milliseconds > websocket_ping_time,
		std::size_t max_read_message_size
	)
		: impl_(boost::make_unique< ws_client_base_impl >(
			*this, std::move(host), std::move(port), std::move(resource),
			websocket_ping_time, max_read_message_size)) {}

	ws_client_base::~ws_client_base(){
		impl_->send("client shutdown");
		impl_->stop();
		impl_->block();
	}


	void ws_client_base::connect(){
		impl_->connect();
	}

	bool ws_client_base::is_connected()const{
		return impl_->is_connected();
	}


	void ws_client_base::send_text(shared_const_buffer buffer){
		impl_->send(std::make_tuple(text_tag{}, std::move(buffer)));
	}

	void ws_client_base::send_binary(shared_const_buffer buffer){
		impl_->send(std::make_tuple(binary_tag{}, std::move(buffer)));
	}


	void ws_client_base::close(boost::beast::string_view reason){
		impl_->send(boost::beast::websocket::close_reason(reason));
	}

	void ws_client_base::block()noexcept{
		impl_->block();
	}

	void ws_client_base::stop()noexcept{
		impl_->stop();
	}


	void ws_client_base::on_open(){}

	void ws_client_base::on_close(){}

	void ws_client_base::on_text(
		boost::beast::multi_buffer const& /*buffer*/){}

	void ws_client_base::on_binary(
		boost::beast::multi_buffer const& /*buffer*/){}

	void ws_client_base::on_error(
		ws_client_location /*location*/,
		boost::system::error_code /*ec*/){}

	void ws_client_base::on_exception(std::exception_ptr /*error*/)noexcept{}


}
