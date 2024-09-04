/*
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
*/

#include "lua_api.h"
#include "lush.h"
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// global for checking if debug_mode is toggled
static bool debug_mode = false;

// -- script execution --
void lua_load_script(lua_State *L, const char *script) {
	char script_path[512];
	// check if script is in the current directory
	if (access(script, F_OK) == 0) {
		snprintf(script_path, sizeof(script_path), "%s", script);
	} else {
		const char *home_dir = getenv("HOME");
		if (home_dir != NULL) {
			snprintf(script_path, sizeof(script_path), "%s/.lush/scripts/%s",
					 home_dir, script);

			if (access(script_path, F_OK) != 0) {
				// script not in either location
				fprintf(stderr, "[C] Script not found: %s\n", script);
				return;
			}
		} else {
			// HOME not set
			fprintf(stderr, "[C] HOME directory is not set.\n");
			return;
		}
	}
	// if we got here the file exists
	if (luaL_dofile(L, script_path) != LUA_OK) {
		printf("[C] Error reading script\n");
		luaL_error(L, "Error: %s\n", lua_tostring(L, -1));
		// remove error from stack
		lua_pop(L, 1);
	}
}

// -- C funtions --
static int execute_command(lua_State *L, const char *line) {
	int status = 0;
	char **commands = lush_split_pipes((char *)line);
	char ***args = lush_split_args(commands, &status);
	if (status == -1) {
		fprintf(stderr, "lush: Expected end of quoted string\n");
	} else if (lush_run(L, args, status) == 0) {
		exit(1);
	}

	for (int i = 0; args[i]; i++) {
		free(args[i]);
	}
	free(args);
	free(commands);
	return status;
}

// -- Lua wrappers --
static int l_execute_command(lua_State *L) {
	const char *command = luaL_checkstring(L, 1);
	int status = execute_command(L, command);
	bool rc = status != -1 ? true : false;

	if (debug_mode) {
		if (rc)
			printf("Executed: %s, success\n", command);
		else
			printf("Executed: %s, failed\n", command);
	}

	lua_pushboolean(L, rc);
	return 1;
}

static int l_get_cwd(lua_State *L) {
	char *cwd = getcwd(NULL, 0);
	lua_pushstring(L, cwd);
	free(cwd);
	return 1;
}

static int l_debug(lua_State *L) {
	if (lua_isboolean(L, 1)) {
		debug_mode = lua_toboolean(L, 1);
	}
	return 0;
}

static int l_cd(lua_State *L) {
	bool rc;
	uid_t uid = getuid();
	struct passwd *pw = getpwuid(uid);
	if (!pw) {
		perror("retrieve home dir");
		rc = false;
		lua_pushboolean(L, rc);
		return 1;
	}

	const char *newdir = luaL_checkstring(L, 1);
	if (newdir == NULL) {
		if (chdir(pw->pw_dir) != 0) {
			perror("lush: cd");
		}
	} else {
		char path[PATH_MAX];
		char extended_path[PATH_MAX];
		char *tilda = strchr(newdir, '~');
		if (tilda) {
			strcpy(path, pw->pw_dir);
			strcat(path, tilda + 1);
		} else {
			strcpy(path, newdir);
		}
		char *exp_path = realpath(path, extended_path);
		if (!exp_path) {
			perror("realpath");
			rc = false;
			lua_pushboolean(L, rc);
			return 1;
		}
		if (chdir(exp_path) != 0) {
			perror("lush: cd");
		}
	}
	rc = true;
	lua_pushboolean(L, rc);
	return 1;
}

// -- register Lua functions --

void lua_register_api(lua_State *L) {
	// global table for api functions
	lua_newtable(L);

	lua_pushcfunction(L, l_execute_command);
	lua_setfield(L, -2, "exec");
	lua_pushcfunction(L, l_get_cwd);
	lua_setfield(L, -2, "getcwd");
	lua_pushcfunction(L, l_debug);
	lua_setfield(L, -2, "debug");
	lua_pushcfunction(L, l_cd);
	lua_setfield(L, -2, "cd");
	// set the table as global
	lua_setglobal(L, "lush");
}
