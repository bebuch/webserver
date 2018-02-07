//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webserver
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webserver/server.hpp>

#include "listener.hpp"

#include <thread>


namespace webserver{


	class server_impl{
	public:
		/// \brief Constructor
		///
		/// Run the IO context on all threads.
		server_impl(
			http_request_handler& handler,
			boost::asio::ip::address const address,
			std::uint16_t const port,
			std::uint8_t const thread_count,
			server::exception_handler&& handle_exception
		)
			: handle_exception_(std::move(handle_exception))
			, ioc_{thread_count}
			, listener_(
				handler,
				ioc_,
				boost::asio::ip::tcp::endpoint{address, port})
		{
			// Run the I/O service on the requested number of thread_count
			threads_.reserve(thread_count);
			for(std::size_t i = 0; i < thread_count; ++i){
				threads_.emplace_back([this]{
					// restart io_context if it returned by exception
					for(;;){
						try{
							ioc_.run();
							return;
						}catch(...){
							if(handle_exception_){
								try{
									handle_exception_(std::current_exception());
								}catch(std::exception const& e){
									log_exception(e,
										"server::exception_handler");
								}catch(...){
									log_exception("server::exception_handler");
								}
							}else{
								log_exception(std::current_exception(),
									"server::exception");
							}
						}
					}
				});
			}
		}


		/// \brief Wait until all thread have finished
		~server_impl(){
			stop();
			block();
		}


		/// \copydoc server::block()
		void block(){
			std::lock_guard< std::mutex > lock(mutex);
			for(auto& thread: threads_){
				if(thread.joinable()) thread.join();
			}
		}

		/// \copydoc server::stop()
		void stop(){
			ioc_.stop();
		}


	private:
		/// \brief Protect thread joins
		std::mutex mutex;

		/// \brief Callback that is called if an exception is thrown
		server::exception_handler const handle_exception_;

		/// \brief The worker threads
		std::vector< std::thread > threads_;

		/// \brief The io_context is required for all I/O
		boost::asio::io_context ioc_;

		/// \brief Accepts incoming connections and launches the sessions
		webserver::listener listener_;
	};


	server::server(
		http_request_handler& handler,
		boost::asio::ip::address const address,
		std::uint16_t const port,
		std::uint8_t const thread_count,
		exception_handler handle_exception
	)
		: impl_(std::make_unique< server_impl >(
				handler,
				address,
				port,
				thread_count,
				std::move(handle_exception)
			)) {}

	server::~server() = default;


	void server::block(){
		impl_->block();
	}

	void server::stop(){
		impl_->stop();
	}


}