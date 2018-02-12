//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webserver
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "websocket_service_impl.hpp"


namespace webserver{


	websocket_service::websocket_service()
		: impl_(std::make_unique< websocket_service_impl >(*this)){}


	websocket_service::~websocket_service(){}


	void websocket_service::on_open(std::size_t const /*identifier*/){}

	void websocket_service::on_close(std::size_t const /*identifier*/){}

	void websocket_service::on_text(
		std::size_t const /*identifier*/,
		boost::beast::multi_buffer& /*buffer*/){}

	void websocket_service::on_binary(
		std::size_t const /*identifier*/,
		boost::beast::multi_buffer& /*buffer*/){}


	void websocket_service::send_text(std::string data){
		impl_->send(std::make_shared< std::string >(std::move(data)));
	}

	void websocket_service::send_text(
		std::size_t const identifier,
		std::string data
	){
		impl_->send(
			identifier,
			std::make_shared< std::string >(std::move(data)));
	}

	void websocket_service::send_text(
		std::set< std::size_t > const& identifier,
		std::string data
	){
		impl_->send(
			identifier,
			std::make_shared< std::string >(std::move(data)));
	}

	void websocket_service::send_binary(
		std::vector< std::uint8_t > data
	){
		impl_->send(
			std::make_shared< std::vector< std::uint8_t > >(std::move(data)));
	}

	void websocket_service::send_binary(
		std::size_t const identifier,
		std::vector< std::uint8_t > data
	){
		impl_->send(
			identifier,
			std::make_shared< std::vector< std::uint8_t > >(std::move(data)));
	}

	void websocket_service::send_binary(
		std::set< std::size_t > const& identifier,
		std::vector< std::uint8_t > data
	){
		impl_->send(
			identifier,
			std::make_shared< std::vector< std::uint8_t > >(std::move(data)));
	}


}
