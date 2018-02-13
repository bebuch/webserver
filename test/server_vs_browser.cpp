//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/http
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webservice/file_request_handler.hpp>
#include <webservice/fail.hpp>
#include <webservice/server.hpp>

#include <thread>


enum class state_t{
	init,
	file_request,
	ws_open,
	ws_close,
	ws_text,
	ws_binary,
	exit
};

std::string to_string(state_t state){
	switch(state){
		case state_t::init:         return "init";
		case state_t::file_request: return "file_request";
		case state_t::ws_open:      return "ws_open";
		case state_t::ws_close:     return "ws_close";
		case state_t::ws_text:      return "ws_text";
		case state_t::ws_binary:    return "ws_binary";
		case state_t::exit:         return "exit";
	}
	throw std::logic_error("unknown state");
}

void pass(state_t expect, state_t got){
	if(expect == got){
		std::cout << "\033[1;32mpass: "
			<< to_string(got)
			<< "\033[0m\n";
	}else{
		std::cout << "\033[1;31mfail: expected "
			<< to_string(expect) << " but got " << to_string(got)
			<< "\033[0m\n";
	}
}

void check(state_t got){
	static state_t state = state_t::init;
	switch(state){
		case state_t::init:
			pass(state, got);
			state = state_t::file_request;
			break;
		case state_t::file_request:
			pass(state, got);
			state = state_t::ws_open;
			break;
		case state_t::ws_open:
			pass(state, got);
			state = state_t::ws_close;
			break;
		case state_t::ws_close:
			pass(state, got);
			state = state_t::ws_text;
			break;
		case state_t::ws_text:
			pass(state, got);
			state = state_t::ws_binary;
			break;
		case state_t::ws_binary:
			pass(state, got);
			state = state_t::exit;
			break;
		case state_t::exit:
			pass(state, got);
			break;
	}
}


struct file_request_handler: webservice::file_request_handler{
	using webservice::file_request_handler::file_request_handler;

	void operator()(
		webservice::http_request&& req,
		webservice::http_response&& send
	)override{
		check(state_t::file_request);
		webservice::file_request_handler::operator()(
			std::move(req), std::move(send));
	}
};


struct websocket_service: webservice::websocket_service{
	static char const test_text[17];

	void on_open(std::uintptr_t)override{
		check(state_t::ws_open);
		send_text(test_text);
	}

	void on_close(std::uintptr_t)override{
		check(state_t::ws_close);
	}

	void on_text(
		std::uintptr_t,
		boost::beast::multi_buffer& buffer
	)override{
		check(state_t::ws_text);
		auto const text = boost::beast::buffers_to_string(buffer.data());
		if(test_text == text){
			std::cout << "\033[1;32mpass: '"
				<< test_text
				<< "'\033[0m\n";
		}else{
			std::cout << "\033[1;31mfail: expected '"
				<< test_text << "' but got '" << text
				<< "'\033[0m\n";
		}
		send_binary(std::vector< std::uint8_t >(
			std::begin(test_text), std::end(test_text)));
	}

	void on_binary(
		std::uintptr_t,
		boost::beast::multi_buffer& buffer
	)override{
		check(state_t::ws_binary);
		auto const text = boost::beast::buffers_to_string(buffer.data());
		if(test_text == text){
			std::cout << "\033[1;32mpass: '"
				<< test_text
				<< "'\033[0m\n";
		}else{
			std::cout << "\033[1;31mfail: expected '"
				<< test_text << "' but got '" << text
				<< "'\033[0m\n";
		}
		close("shutdown");
	}
};

char const websocket_service::test_text[17] = "test text values";


int main(){
	try{
		file_request_handler handler("server_vs_browser");
		websocket_service service;
		webservice::server server(handler, service,
			boost::asio::ip::make_address("127.0.0.1"), 1234, 1);

		check(state_t::init);

		std::system("xdg-open http://127.0.0.1:1234");

		std::this_thread::sleep_for(std::chrono::seconds(10));
		server.stop();

		check(state_t::exit);

		return 0;
	}catch(std::exception const& e){
		webservice::log_exception(e, "program");
		return 1;
	}catch(...){
		webservice::log_exception("program");
		return 1;
	}

}
