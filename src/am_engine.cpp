#include "amulet.h"

static void init_reserved_refs(lua_State *L);
static void open_stdlualibs(lua_State *L);
static void init_package_searcher(lua_State *L);
static bool run_embedded_scripts(lua_State *L, bool worker);

lua_State *am_init_engine(bool worker) {
    lua_State *L = luaL_newstate();
    init_reserved_refs(L);
    open_stdlualibs(L);
    init_package_searcher(L);
    am_init_traceback_func(L);
    am_open_logging_module(L);
    am_open_math_module(L);
    am_open_time_module(L);
    am_open_buffer_module(L);
    if (!worker) {
        am_init_param_name_map(L);
        am_init_actions();
        am_open_window_module(L);
        am_open_scene_module(L);
        am_open_program_module(L);
        am_open_texture2d_module(L);
        am_open_framebuffer_module(L);
        am_open_depthbuffer_module(L);
        am_open_transforms_module(L);
        am_open_renderer_module(L);
        am_open_audio_module(L);
    }
    if (!run_embedded_scripts(L, worker)) {
        lua_close(L);
        return NULL;
    }
    am_set_globals_metatable(L);
    return L;
}

void am_destroy_engine(lua_State *L) {
    // Audio must be destroyed before closing the lua state, because
    // closing the lua state will destroy the root audio node.
    am_destroy_audio();
    am_destroy_windows(L);
    lua_close(L);
    am_reset_log_cache();
}

static void init_reserved_refs(lua_State *L) {
    int i, j;
    do {
        lua_pushboolean(L, 1);
        i = luaL_ref(L, LUA_REGISTRYINDEX);
    } while (i < AM_RESERVED_REFS_START - 1);
    if (i != AM_RESERVED_REFS_START - 1) {
        am_abort("Internal Error: AM_RESERVED_REFS_START too low\n");
    }
    for (i = AM_RESERVED_REFS_START; i < AM_RESERVED_REFS_END; i++) {
        lua_pushboolean(L, 1);
        j = luaL_ref(L, LUA_REGISTRYINDEX);
        if (i != j) {
            am_abort("Internal Error: non-contiguous refs\n");
        }
        j++;
    }
}

static void open_stdlualibs(lua_State *L) {
    am_requiref(L, "base",      luaopen_base);
    am_requiref(L, "package",   luaopen_package);
    am_requiref(L, "math",      luaopen_math);
#ifndef AM_LUAJIT
    // luajit opens coroutine in luaopen_base
    am_requiref(L, "coroutine", luaopen_coroutine);
#endif
    am_requiref(L, "string",    luaopen_string);
    am_requiref(L, "table",     luaopen_table);
    am_requiref(L, "os",        luaopen_os);
    am_requiref(L, "debug",     luaopen_debug);

#ifdef AM_LUAJIT
    am_requiref(L, "ffi",       luaopen_ffi);
    am_requiref(L, "jit",       luaopen_jit);
#endif
}

static void init_package_searcher(lua_State *L) {
    lua_getglobal(L, "package");
    lua_newtable(L);
    lua_pushcclosure(L, am_package_searcher, 0);
    lua_rawseti(L, -2, 1);
#ifdef AM_LUAJIT
    lua_setfield(L, -2, "loaders");
#else
    lua_setfield(L, -2, "searchers");
#endif
    lua_pop(L, 1);
}

#define MAX_CHUNKNAME_SIZE 100

static bool run_embedded_script(lua_State *L, const char *filename) {
    char chunkname[MAX_CHUNKNAME_SIZE];
    am_embedded_file_record *rec = am_get_embedded_file(filename);
    if (rec == NULL) {
        am_log0("embedded file '%s' missing", filename);
        return false;
    }
    snprintf(chunkname, MAX_CHUNKNAME_SIZE, "@embedded-%s", filename);
    int r = luaL_loadbuffer(L, (char*)rec->data, rec->len, chunkname);
    if (r == 0) {
        if (!am_call(L, 0, 0)) {
            return false;
        }
    } else {
        const char *msg = lua_tostring(L, -1);
        am_log0("%s", msg);
        return false;
    }
    return true;
}

static bool run_embedded_scripts(lua_State *L, bool worker) {
    return
        run_embedded_script(L, "lua/extra.lua") &&
        run_embedded_script(L, "lua/config.lua") &&
        run_embedded_script(L, "lua/time.lua") &&
        run_embedded_script(L, "lua/input.lua");
}

