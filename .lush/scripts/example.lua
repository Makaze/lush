--[[
Copyright (c) 2024, Lance Borden
All rights reserved.

This software is licensed under the BSD 3-Clause License.
You may obtain a copy of the license at:
https://opensource.org/licenses/BSD-3-Clause

Redistribution and use in source and binary forms, with or without
modification, are permitted under the conditions stated in the BSD 3-Clause
License.

THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTIES,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
]]

-- This file is a demo of all of the commands Lunar Shell has in its Lua API
-- the Lua file itself is the executable unit so all code is run line by line

-- All Lua specific tools still work here too. Functions, loops, conditionals,
-- tables, Lua's standard library, etc are all available to integrate with Lunar Shell

print("Welcome to Lunar Shell scripting")

-- exec can be used to run any command line prompt
-- this method is much more native than using os.execute and is the recommended function
-- it also returns a boolean on if the command executed successfully
if lush.exec('echo "hello world"\n') then
	print("echo worked properly")
end

-- debug mode can be used to log execution of commands
lush.debug(true) -- enters debug
lush.exec('echo "echo in debug mode"')
lush.debug(false) -- exits debug

-- getcwd returns the current working directory
local cwd = lush.getcwd()
print(cwd)

-- exec also supports piping
-- this comment will get grepped since it says hello
lush.exec("cd " .. os.getenv("HOME") .. "/.lush/scripts")
lush.exec('cat "example.lua" | grep "hello" | sort | uniq')
lush.exec("cd " .. cwd)
