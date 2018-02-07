//-----------------------------------------------------------------------------
// Copyright (c) 2018 Benjamin Buch
//
// https://github.com/bebuch/webserver
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <webserver/file_request_handler.hpp>

#include "path_concat.hpp"
#include "mime_type.hpp"

#include <iostream>


namespace webserver{


	void file_request_handler::operator()(
		http_request&& req,
		http_response&& send
	){
		namespace http = boost::beast::http;

		// Make sure we can handle the method
		if(
			req.method() != http::verb::get &&
			req.method() != http::verb::head
		){
			send(bad_request(req, "Unknown HTTP-method"));
			return;
		}

		// Request path must be absolute and not contain "..".
		if(
			req.target().empty() ||
			req.target()[0] != '/' ||
			req.target().find("..") != boost::beast::string_view::npos
		){
			send(bad_request(req, "Illegal request-target"));
			return;
		}

		// Build the path to the requested file
		std::string path = path_concat(doc_root_, req.target());
		if(req.target().back() == '/'){
			path.append("index.html");
		}

		// Attempt to open the file
		boost::beast::error_code ec;
		http::file_body::value_type body;
		body.open(path.c_str(), boost::beast::file_mode::scan, ec);

		// Handle the case where the file doesn't exist
		if(ec == boost::system::errc::no_such_file_or_directory){
			send(not_found(req, req.target()));
			return;
		}

		// Handle an unknown error
		if(ec){
			send(server_error(req, ec.message()));
			return;
		}

		// Cache the size since we need it after the move
		auto const size = body.size();

		// Respond to HEAD request
		if(req.method() == http::verb::head){
			http::response< http::empty_body > res{
				http::status::ok, req.version()};
			res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
			res.set(http::field::content_type, mime_type(path));
			res.content_length(size);
			res.keep_alive(req.keep_alive());
			send(std::move(res));
			return;
		}

		// Respond to GET request
		http::response< http::file_body > res{
			std::piecewise_construct,
			std::make_tuple(std::move(body)),
			std::make_tuple(http::status::ok, req.version())};
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::content_type, mime_type(path));
		res.content_length(size);
		res.keep_alive(req.keep_alive());
		send(std::move(res));
	}



}