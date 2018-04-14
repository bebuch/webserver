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

#include <boost/lexical_cast.hpp>

#include <thread>
#include <csignal>
#include <cstdlib>


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


struct ws_handler
	: webservice::error_printing_ws_handler< webservice::ws_handler >
{
	int count = 0;

	void on_open(webservice::ws_identifier, std::string const&)override{
		send_text("0");
	}

	void on_close(webservice::ws_identifier, std::string const&)override{
		server()->shutdown();
	}

	void on_text(
		webservice::ws_identifier identifier,
		std::string const&,
		std::string&& /*text*/
	)override{
// 		if(count % 1000 == 0){
// 			std::cout << "\033[1;32mserver pass: '" << text << "'\033[0m"
// 				<< std::endl;
// 		}
		++count;
		send_text(identifier, std::to_string(count));
	}

	void on_binary(
		webservice::ws_identifier,
		std::string const&,
		std::vector< std::uint8_t >&& data
	)override{
		std::string text(data.begin(), data.end());
		std::cout << "\033[1;31mfail: server unexpected binary message '"
			<< text << "'\033[0m\n";
		close("shutdown");
	}
};


struct ws_client
	: webservice::error_printing_ws_client< webservice::ws_client >
{
	using error_printing_ws_client::error_printing_ws_client;

	int count = 0;

	void on_text(std::string&& text)override{
// 		if(count % 1000 == 0){
// 			std::cout << "\033[1;32mclient pass: '" << text << "'\033[0m"
// 				<< std::endl;
// 		}
		++count;
		send_text(text);
	}

	void on_binary(std::vector< std::uint8_t >&& data)override{
		std::string text(data.begin(), data.end());
		std::cout << "\033[1;31mfail: client unexpected binary message '"
			<< text << "'\033[0m\n";
		close("shutdown");
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

	std::srand(std::time(nullptr));

	try{
		using std::make_unique;
		webservice::server server(
			make_unique< request_handler >(),
			make_unique< ws_handler >(),
			make_unique< webservice::error_printing_error_handler >(),
			boost::asio::ip::make_address("127.0.0.1"), 1234, 1);

		constexpr std::size_t client_count = 10;
		std::vector< std::thread > client_threads;
		client_threads.reserve(client_count);
		for(std::size_t i = 0; i < client_count; ++i){
			client_threads.emplace_back([]{
					ws_client client;
					client.async_connect("127.0.0.1", "1234", "/");
					std::this_thread::sleep_for(
						std::chrono::milliseconds(rand() % 100));
				});
		}

		for(auto& thread: client_threads){
			thread.join();
		}

		return 0;
	}catch(std::exception const& e){
		std::cerr << "Exception: " << e.what() << "\n";
		return 1;
	}catch(...){
		std::cerr << "Unknown exception\n";
		return 1;
	}

}