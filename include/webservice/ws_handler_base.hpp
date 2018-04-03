//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webservice
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _webservice__ws_handler_base__hpp_INCLUDED_
#define _webservice__ws_handler_base__hpp_INCLUDED_

#include "shared_const_buffer.hpp"
#include "ws_handler_location.hpp"

#include <boost/asio/ip/tcp.hpp>

#include <boost/beast/http.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/string.hpp>

#include <boost/any.hpp>

#include <string>
#include <chrono>


namespace webservice{


	using http_request
		= boost::beast::http::request< boost::beast::http::string_body >;


	template < typename T >
	class sessions;


	class ws_server_session;

	class ws_handler_base{
	public:
		ws_handler_base();

		/// \brief Destructor
		virtual ~ws_handler_base();

		ws_handler_base(ws_handler_base const&) = delete;

		ws_handler_base& operator=(ws_handler_base const&) = delete;


		/// \brief Create a new http_session
		void emplace(boost::asio::ip::tcp::socket&& socket, http_request&& req);


		/// \brief Send a text message to session
		void send_text(
			ws_server_session* session,
			shared_const_buffer buffer);

		/// \brief Send a binary message to session
		void send_binary(
			ws_server_session* session,
			shared_const_buffer buffer);

		/// \brief Shutdown session
		void close(
			ws_server_session* session,
			boost::beast::string_view reason);


		/// \brief Called with a unique identifier when a sessions starts
		///
		/// Default implementation does nothing.
		virtual void on_open(
			ws_server_session* session,
			std::string const& resource);

		/// \brief Called with a unique identifier when a sessions ends
		///
		/// Default implementation does nothing.
		virtual void on_close(
			ws_server_session* session,
			std::string const& resource);

		/// \brief Called when a session received a text message
		///
		/// Default implementation does nothing.
		virtual void on_text(
			ws_server_session* session,
			std::string const& resource,
			boost::beast::multi_buffer&& buffer);

		/// \brief Called when a session received a binary message
		///
		/// Default implementation does nothing.
		virtual void on_binary(
			ws_server_session* session,
			std::string const& resource,
			boost::beast::multi_buffer&& buffer);

		/// \brief Called when an error occured
		///
		/// Default implementation does nothing.
		virtual void on_error(
			ws_server_session* session,
			std::string const& resource,
			ws_handler_location location,
			boost::system::error_code ec);

		/// \brief Called when an exception was thrown
		///
		/// Default implementation does nothing.
		virtual void on_exception(
			ws_server_session* session,
			std::string const& resource,
			std::exception_ptr error)noexcept;


		/// \brief Shutdown hint called by the server
		virtual void shutdown()noexcept;


		/// \brief Set max size of incomming WebSocket messages
		void set_max_read_message_size(std::size_t bytes){
			max_read_message_size_ = bytes;
		}

		/// \brief Max size of incomming WebSocket messages
		std::size_t max_read_message_size()const{
			return max_read_message_size_;
		}


		/// \brief Set session timeout
		void set_ping_time(std::chrono::milliseconds ms){
			ping_time_ = ms;
		}

		/// \brief Session timeout
		std::chrono::milliseconds ping_time()const{
			return ping_time_;
		}


		/// \brief Set the owning server
		virtual void set_server(class server& server);


	protected:
		/// \brief Get reference to server
		class server* server()noexcept;


	private:
		/// \brief Pointer to the server object
		class server* server_ = nullptr;

		/// \brief Max size of incomming http and WebSocket messages
		std::size_t max_read_message_size_{16 * 1024 * 1024};

		/// \brief WebSocket session timeout
		///
		/// After this time without an incomming message a ping is send.
		/// If no message is incomming after a second period of this time, the
		/// session is considerd to be dead and will be closed.
		std::chrono::milliseconds ping_time_{15000};

		/// \brief Pointer to implementation
		std::unique_ptr< sessions< class ws_server_session > > list_;

	};


}


#endif
