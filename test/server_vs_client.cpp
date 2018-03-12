//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/http
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "error_printing_ws_handler.hpp"
#include "error_printing_error_handler.hpp"
#include "error_printing_request_handler.hpp"
#include "error_printing_ws_client.hpp"

#include <webservice/server.hpp>
#include <webservice/ws_handler.hpp>
#include <webservice/ws_client.hpp>

#include <boost/make_unique.hpp>

#include <thread>
#include <csignal>


enum class state_t{
	init,
	ws_server_open,
	ws_server_close,
	ws_server_text,
	ws_server_binary,
	ws_client_open,
	ws_client_close,
	ws_client_text,
	ws_client_binary,
	exit
};

boost::beast::string_view to_string(state_t state)noexcept{
	switch(state){
		case state_t::init:             return "init";
		case state_t::ws_server_open:   return "ws_server_open";
		case state_t::ws_server_close:  return "ws_server_close";
		case state_t::ws_server_text:   return "ws_server_text";
		case state_t::ws_server_binary: return "ws_server_binary";
		case state_t::ws_client_open:   return "ws_client_open";
		case state_t::ws_client_close:  return "ws_client_close";
		case state_t::ws_client_text:   return "ws_client_text";
		case state_t::ws_client_binary: return "ws_client_binary";
		case state_t::exit:             return "exit";
	}
	return "unknown state";
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

state_t state = state_t::init;

void check(state_t got){
	switch(state){
		case state_t::init:
			pass(state, got);
			state = state_t::ws_server_open;
			break;
		case state_t::ws_server_open:
			pass(state, got);
			state = state_t::ws_client_open;
			break;
		case state_t::ws_client_open:
			pass(state, got);
			state = state_t::ws_client_text;
			break;
		case state_t::ws_client_text:
			pass(state, got);
			state = state_t::ws_server_text;
			break;
		case state_t::ws_server_text:
			pass(state, got);
			state = state_t::ws_client_binary;
			break;
		case state_t::ws_client_binary:
			pass(state, got);
			state = state_t::ws_server_binary;
			break;
		case state_t::ws_server_binary:
			pass(state, got);
			state = state_t::ws_client_close;
			break;
		case state_t::ws_client_close:
			pass(state, got);
			state = state_t::ws_server_close;
			break;
		case state_t::ws_server_close:
			pass(state, got);
			state = state_t::exit;
			break;
		case state_t::exit:
			pass(state, got);
			break;
	}
}


struct request_handler
	: webservice::error_printing_request_handler<
		webservice::http_request_handler >
{
	using error_printing_request_handler::error_printing_request_handler;

	void operator()(
		webservice::http_request&& req,
		webservice::http_response&& send
	)override{
		std::cout << "\033[1;31mfail: unexpected file request '"
			<< req.target() << "'\033[0m\n";
		webservice::http_request_handler::operator()(
			std::move(req), std::move(send));
	}
};


std::string const test_text = "test text values";


struct ws_handler
	: webservice::error_printing_ws_handler< webservice::ws_handler >
{
	void on_open(std::uintptr_t, std::string const&)override{
		check(state_t::ws_server_open);
		send_text(test_text);
	}

	void on_close(std::uintptr_t, std::string const&)override{
		check(state_t::ws_server_close);
	}

	void on_text(
		std::uintptr_t,
		std::string const&,
		std::string&& text
	)override{
		check(state_t::ws_server_text);
		if(test_text == text){
			std::cout << "\033[1;32mserver pass: '"
				<< test_text
				<< "'\033[0m\n";
		}else{
			std::cout << "\033[1;31mfail: server expected '"
				<< test_text << "' but got '" << text
				<< "'\033[0m\n";
		}
		send_binary(std::vector< std::uint8_t >(
			std::begin(test_text), std::end(test_text)));
	}

	void on_binary(
		std::uintptr_t,
		std::string const&,
		std::vector< std::uint8_t >&& data
	)override{
		check(state_t::ws_server_binary);
		std::string text(data.begin(), data.end());
		if(test_text == text){
			std::cout << "\033[1;32mserver pass: '"
				<< test_text
				<< "'\033[0m\n";
		}else{
			std::cout << "\033[1;31mfail: server expected '"
				<< test_text << "' but got '" << text
				<< "'\033[0m\n";
		}
		close("shutdown");
	}
};


struct ws_client
	: webservice::error_printing_ws_client< webservice::ws_client >
{
	using error_printing_ws_client::error_printing_ws_client;

	void on_open()override{
		check(state_t::ws_client_open);
	}

	void on_close()override{
		check(state_t::ws_client_close);
	}

	void on_text(std::string&& text)override{
		check(state_t::ws_client_text);
		if(test_text == text){
			std::cout << "\033[1;32mclient pass: '"
				<< test_text
				<< "'\033[0m\n";
		}else{
			std::cout << "\033[1;31mfail: client expected '"
				<< test_text << "' but got '" << text
				<< "'\033[0m\n";
		}
		send_text(test_text);
	}

	void on_binary(std::vector< std::uint8_t >&& data)override{
		check(state_t::ws_client_binary);
		std::string text(data.begin(), data.end());
		if(test_text == text){
			std::cout << "\033[1;32mclient pass: '"
				<< test_text
				<< "'\033[0m\n";
		}else{
			std::cout << "\033[1;31mfail: client expected '"
				<< test_text << "' but got '" << text
				<< "'\033[0m\n";
		}
		send_binary(std::vector< std::uint8_t >(
			std::begin(test_text), std::end(test_text)));
	}
};


void signal_handler(int signum){
	std::signal(signum, SIG_DFL);
	std::cout << "Signal: " << signum << '\n';
	std::raise(signum);
}


int main(){
	std::signal(SIGSEGV, &signal_handler);
	std::signal(SIGABRT, &signal_handler);
	std::signal(SIGINT, &signal_handler);

	try{
		{
			using boost::make_unique;
			webservice::server server(
				make_unique< request_handler >(),
				make_unique< ws_handler >(),
				make_unique< webservice::error_printing_error_handler >(),
				boost::asio::ip::make_address("127.0.0.1"), 1234, 1);

			check(state_t::init);

			ws_client client("127.0.0.1", "1234", "/");
			client.connect();

			using system_clock = std::chrono::system_clock;
			auto const start = system_clock::now();
			while(
				state != state_t::exit &&
				system_clock::now() < start + std::chrono::seconds(10)
			){
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			server.stop();
		}

		check(state_t::exit);

		return 0;
	}catch(std::exception const& e){
		std::cerr << "Exception: " << e.what() << "\n";
		return 1;
	}catch(...){
		std::cerr << "Unknown exception\n";
		return 1;
	}

}
