/* Minimal Luau host that runs a single cffi-luau test file.
 *
 * The original test suite ran each .lua file through a standalone Lua
 * interpreter that loaded the FFI via require()/package.cpath. Luau has no
 * native module loader and a deliberately reduced standard library (no
 * package, no os.getenv/os.exit, no dofile), so instead we embed Luau here,
 * statically link cffi in, and provide the small set of globals the tests
 * expect:
 *
 *   - cffi              -> global factory table (from luaopen_cffi)
 *   - load_testlib(ffi) -> ffi.load(<testlib path>) using argv[2]
 *   - skip_test()       -> raises a sentinel error; we exit 77 (CTest skip)
 *
 * Usage: cffi_test_host <test.lua> [<testlib-shared-lib-path>]
 * Exit codes: 0 = pass, 77 = skipped, anything else = failure.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <iterator>

#include <lua.h>
#include <lualib.h>
#include <luacode.h>

extern "C" int luaopen_cffi(lua_State *L);

static const char SKIP_SENTINEL[] = "__CFFI_SKIP_TEST__";
static std::string g_testlib_path;

static int l_load_testlib(lua_State *L) {
    if (g_testlib_path.empty()) {
        luaL_error(L, "%s", SKIP_SENTINEL);
    }
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "load");
    lua_pushlstring(L, g_testlib_path.data(), g_testlib_path.size());
    lua_call(L, 1, 1);
    return 1;
}

static int l_skip_test(lua_State *L) {
    luaL_error(L, "%s", SKIP_SENTINEL);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <test.lua> [testlib]\n", argv[0]);
        return 2;
    }
    if (argc >= 3) {
        g_testlib_path = argv[2];
    }

    std::ifstream in(argv[1], std::ios::binary);
    if (!in) {
        std::fprintf(stderr, "cannot open test file: %s\n", argv[1]);
        return 2;
    }
    std::string src(
        (std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()
    );

    lua_State *L = luaL_newstate();
    if (!L) {
        std::fprintf(stderr, "failed to create Luau state\n");
        return 1;
    }
    luaL_openlibs(L);

    /* expose the cffi factory as a global */
    luaopen_cffi(L);              /* pushes the cffi factory table */
    lua_setglobal(L, "cffi");

    /* test environment globals (NB: we intentionally do not sandbox, so the
     * global table stays writable) */
    lua_pushcfunction(L, l_load_testlib, "load_testlib");
    lua_setglobal(L, "load_testlib");
    lua_pushcfunction(L, l_skip_test, "skip_test");
    lua_setglobal(L, "skip_test");

    /* compile + load the Luau source */
    size_t bclen = 0;
    char *bc = luau_compile(src.data(), src.size(), nullptr, &bclen);
    if (!bc) {
        std::fprintf(stderr, "luau_compile: out of memory\n");
        lua_close(L);
        return 1;
    }
    int loadres = luau_load(L, "@test", bc, bclen, 0);
    std::free(bc);
    if (loadres != 0) {
        std::fprintf(stderr, "load error: %s\n", lua_tostring(L, -1));
        lua_close(L);
        return 1;
    }

    int callres = lua_pcall(L, 0, 0, 0);
    int rc = 0;
    if (callres != 0) {
        const char *err = lua_tostring(L, -1);
        if (err && std::strstr(err, SKIP_SENTINEL)) {
            rc = 77; /* CTest treats this as "skipped" */
        } else {
            std::fprintf(stderr, "ERROR: %s\n", err ? err : "(no message)");
            rc = 1;
        }
    }
    lua_close(L);
    return rc;
}
