/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <string>
#include "irrlichttypes.h"

class InputHandler;
class ChatBackend;  /* to avoid having to include chat.h */
struct SubgameSpec;

class Application {
	public:
		void run_game(bool *kill,
				bool random_input,
				InputHandler *input,
				const std::string &map_dir,
				const std::string &playername,
				const std::string &password,
				const std::string &address, // If "", local server is used
				u16 port,
				std::string &error_message,
				ChatBackend &chat_backend,
				bool *reconnect_requested,
				const SubgameSpec &gamespec, // Used for local game
				bool simple_singleplayer_mode);
};

#endif // APPLICATION_H_
