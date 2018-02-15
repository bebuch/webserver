//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__websocket_client_impl__hpp_INCLUDED_
#define _webservice__websocket_client_impl__hpp_INCLUDED_

#include <webservice/fail.hpp>

#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/string.hpp>
#include <boost/beast/websocket.hpp>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/bind_executor.hpp>

#include <memory>
#include <string>
#include <vector>
#include <thread>


namespace webservice{


	class websocket_client;

	class websocket_client_impl{
	public:
		/// \brief Constructor
		websocket_client_impl(
			websocket_client& self,
			std::string&& host,
			std::uint16_t port,
			std::string&& resource
		)
			: self_(self)
			, resolver_(ioc_)
			, ws_(ioc_)
			, strand_(ws_.get_executor())
			, thread_([
					this,
					host = std::move(host),
					port,
					resource = std::move(resource)
				]{
					// Look up the domain name
					resolver_.async_resolve(
						host,
						std::to_string(port),
						[this, host, resource](
							boost::system::error_code ec,
							boost::asio::ip::tcp::resolver::results_type results
						){
							if(ec){
								on_error(ec);
								return;
							}

							// Make the connection on the IP address we get
							// from a lookup
							boost::asio::async_connect(
								ws_.next_layer(),
								results.begin(),
								results.end(),
								[this, host, resource](
									boost::system::error_code ec, auto
								){
									if(ec){
										on_error(ec);
										return;
									}

									// Perform the websocket handshake
									ws_.async_handshake(host, resource,
										[this](boost::system::error_code ec){
											if(ec){
												on_error(ec);
											}else{
												on_open();
											}
										});
								});
						});

					// restart io_context if it returned by exception
					for(;;){
						try{
							ioc_.run();
							return;
						}catch(...){
							on_exception(std::current_exception());
						}
					}
				}) {}

		/// \brief Destructor
		~websocket_client_impl(){
			ioc_.stop();
			if(thread_.joinable()) thread_.join();
		}


		/// \brief Send a message
		template < typename Data >
		void send(std::shared_ptr< Data > data){
			ws_.text(std::is_same_v< Data, std::string >);
			auto buffer = boost::asio::const_buffer(data->data(), data->size());
			ws_.async_write(
				std::move(buffer),
				boost::asio::bind_executor(
					strand_,
					[this, data = std::move(data)](
						boost::system::error_code ec,
						std::size_t /*bytes_transferred*/
					){
						on_write(ec);
					}));
		}

		/// \brief Close the sessions
		void close(boost::beast::string_view reason){
			ws_.close(reason);
			ioc_.stop();
		}

		/// \brief Read a websocket message
		void do_read(){
			// Set the timer
// 			timer_.expires_after(std::chrono::seconds(15));

			// Read a message into our buffer
			ws_.async_read(
				buffer_,
				boost::asio::bind_executor(
					strand_,
					[this](
						boost::system::error_code ec,
						std::size_t /*bytes_transferred*/
					){
						on_read(ec);
					}));
		}

		/// \brief Called when a websocket message was read
		void on_read(boost::system::error_code ec){
			// Happens when the timer closes the socket
			if(ec == boost::asio::error::operation_aborted){
				return;
			}

			// This indicates that the websocket_session was closed
			if(ec == boost::beast::websocket::error::closed){
				return;
			}

			if(ec){
				on_error(ec);
			}

			// Note that there is activity
// 			activity();

			// Echo the message
			if(ws_.got_text()){
				on_text(buffer_);
			}else{
				on_binary(buffer_);
			}

			// Clear the buffer
			buffer_.consume(buffer_.size());

			// Do another read
			do_read();
		}


		void on_write(boost::system::error_code ec){
			// Happens when the timer closes the socket
			if(ec == boost::asio::error::operation_aborted){
				return;
			}

			if(ec){
				on_error(ec);
				return;
			}
		}


	private:
		/// \brief Called when the sessions starts
		void on_open(){
			try{
				self_.on_open();
			}catch(...){
				on_exception(std::current_exception());
			}

			do_read();
		}

		/// \brief Called when the sessions ends
		void on_close(boost::beast::string_view reason){
			try{
				self_.on_close(reason);
			}catch(...){
				on_exception(std::current_exception());
			}
		}

		/// \brief Called when the session received a text message
		void on_text(boost::beast::multi_buffer& buffer){
			try{
				self_.on_text(buffer);
			}catch(...){
				on_exception(std::current_exception());
			}
		}

		/// \brief Called when the session received a binary message
		void on_binary(boost::beast::multi_buffer& buffer){
			try{
				self_.on_binary(buffer);
			}catch(...){
				on_exception(std::current_exception());
			}
		}

		/// \brief Called when an error occured
		void on_error(boost::system::error_code ec){
			try{
				self_.on_error(ec);
			}catch(...){
				on_exception(std::current_exception());
			}
		}

		/// \brief Called when an exception was thrown
		void on_exception(std::exception_ptr error)noexcept{
			try{
				self_.on_exception(error);
			}catch(...){
				on_exception(std::current_exception());
			}
		}


		/// \brief Pointer to implementation
		websocket_client& self_;

		boost::asio::io_context ioc_;
		boost::asio::ip::tcp::resolver resolver_;
		boost::beast::websocket::stream< boost::asio::ip::tcp::socket > ws_;
		boost::asio::strand< boost::asio::io_context::executor_type > strand_;
		boost::beast::multi_buffer buffer_;

		std::thread thread_;
	};


}


#endif
