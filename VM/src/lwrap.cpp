#include <csetjmp>
#include <cstdio>

#include "luawrap.h"
#include "lualib.h"
#include "lstate.h"

// use POSIX versions of setjmp/longjmp if possible: they don't save/restore signal mask and are therefore faster
#if defined(__linux__) || defined(__APPLE__)
#define LUAU_SETJMP(buf) _setjmp(buf)
#define LUAU_LONGJMP(buf, code) _longjmp(buf, code)
#else
#define LUAU_SETJMP(buf) setjmp(buf)
#define LUAU_LONGJMP(buf, code) longjmp(buf, code)
#endif

struct lua_jmpbuf
{
    lua_jmpbuf* volatile prev;
    volatile int status;
    jmp_buf buf;
};

inline void luaD_enter(lua_State* L, lua_jmpbuf* jb)
{
    jb->prev = L->global->errorjmp;
    jb->status = 0;
    L->global->errorjmp = jb;
}

inline void luaD_exit(lua_State* L, lua_jmpbuf* jb)
{
    L->global->errorjmp = jb->prev;
    L->global->unwinding = jb->status;
}

LUA_API int luaW_getstatus(lua_State* L)
{
    return L->global->unwinding;
}

LUA_API void luaW_assertconf_log() {
    Luau::assertHandler() = [](const char* expression, const char* file, int line, const char* function) {
        fprintf(stderr, "LUAU ASSERT FAILED: %s\n", expression);
        fprintf(stderr, "  at %s:%d in %s\n", file, line, function);

        return 0; // continue executing (probably will have corruption, but may give more info)
    };
}

LUA_API void luaW_assertconf_dump() {
    Luau::assertHandler() = [](const char* expression, const char* file, int line, const char* function) {
        fprintf(stderr, "LUAU ASSERT FAILED: %s\n", expression);
        fprintf(stderr, "  at %s:%d in %s\n", file, line, function);

        // hard crash from here so the JVM will reconstruct the jvm side of the stacktrace.
        // not sure if there is a better way to achieve this, but it works :-)
        int *p = NULL;
        *p = 42;

        return 1;
    };
}

LUA_API lua_State* luaW_newstate(lua_Alloc f)
{
    return f != nullptr ? lua_newstate(f, nullptr) : luaL_newstate();
}

LUA_API lua_State* luaW_newthread(lua_State* L) {
    lua_jmpbuf jb;
    lua_State* ret = nullptr;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
         ret = lua_newthread(L);
    luaD_exit(L, &jb);
    return ret;
}

LUA_API void luaW_resetthread(lua_State* L) {
    lua_jmpbuf jb;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        lua_resetthread(L);
    luaD_exit(L, &jb);
}

LUA_API int luaW_equal(lua_State* L, int idx1, int idx2) {
    lua_jmpbuf jb;
    int ret = 0;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        ret = lua_equal(L, idx1, idx2);
    luaD_exit(L, &jb);
    return ret;
}

LUA_API int luaW_lessthan(lua_State* L, int idx1, int idx2) {
    lua_jmpbuf jb;
    int ret = 0;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        ret = luaW_lessthan(L, idx1, idx2);
    luaD_exit(L, &jb);
    return ret;
}

LUA_API const char* luaW_tolstring(lua_State* L, int idx, size_t* len) {
    lua_jmpbuf jb;
    const char* ret = nullptr;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        ret = lua_tolstring(L, idx, len);
    luaD_exit(L, &jb);
    return ret;
}

LUA_API int luaW_objlen(lua_State* L, int idx) {
    lua_jmpbuf jb;
    int ret = 0;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        ret = lua_objlen(L, idx);
    luaD_exit(L, &jb);
    return ret;
}

LUA_API void luaW_pushlstring(lua_State* L, const char* s, size_t l) {
    lua_jmpbuf jb;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        lua_pushlstring(L, s, l);
    luaD_exit(L, &jb);
}

LUA_API void luaW_pushcclosurek(lua_State* L, lua_CFunction fn, const char* debugname, int nup, lua_Continuation cont) {
    lua_jmpbuf jb;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0) {
        lua_pushcclosurek(L, fn, debugname, nup, cont);

        // Marker for java function
        (L->top - 1)->value.gc->cl.isC = 2;
    }
    luaD_exit(L, &jb);
}

LUA_API void* luaW_newuserdatatagged(lua_State* L, size_t sz, int tag) {
    lua_jmpbuf jb;
    void* ret = nullptr;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
         ret = lua_newuserdatatagged(L, sz, tag);
    luaD_exit(L, &jb);
    return ret;
}

LUA_API void* luaW_newuserdatataggedwithmetatable(lua_State* L, size_t sz, int tag) {
    lua_jmpbuf jb;
    void* ret = nullptr;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
         ret = lua_newuserdatataggedwithmetatable(L, sz, tag);
    luaD_exit(L, &jb);
    return ret;
}

LUA_API void* luaW_newuserdatadtor(lua_State* L, size_t sz, void (*dtor)(void*)) {
    lua_jmpbuf jb;
    void* ret = nullptr;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
         ret = lua_newuserdatadtor(L, sz, dtor);
    luaD_exit(L, &jb);
    return ret;
}

LUA_API void* luaW_newbuffer(lua_State* L, size_t sz) {
    lua_jmpbuf jb;
    void* ret = nullptr;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
         ret = lua_newbuffer(L, sz);
    luaD_exit(L, &jb);
    return ret;
}

LUA_API int luaW_gettable(lua_State* L, int idx) {
    lua_jmpbuf jb;
    int ret = 0;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        ret = lua_gettable(L, idx);
    luaD_exit(L, &jb);
    return ret;
}

LUA_API int luaW_getfield(lua_State* L, int idx, const char* k) {
    lua_jmpbuf jb;
    int ret = 0;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        ret = lua_getfield(L, idx, k);
    luaD_exit(L, &jb);
    return ret;
}

LUA_API void luaW_createtable(lua_State* L, int narr, int nrec) {
    lua_jmpbuf jb;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        lua_createtable(L, narr, nrec);
    luaD_exit(L, &jb);
}

LUA_API void luaW_settable(lua_State* L, int idx) {
    lua_jmpbuf jb;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        lua_settable(L, idx);
    luaD_exit(L, &jb);
}

LUA_API void luaW_setfield(lua_State* L, int idx, const char* k) {
    lua_jmpbuf jb;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        lua_setfield(L, idx, k);
    luaD_exit(L, &jb);
}

LUA_API void luaW_rawsetfield(lua_State* L, int idx, const char* k) {
    lua_jmpbuf jb;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        lua_rawsetfield(L, idx, k);
    luaD_exit(L, &jb);
}

LUA_API void luaW_rawset(lua_State* L, int idx) {
    lua_jmpbuf jb;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        lua_rawset(L, idx);
    luaD_exit(L, &jb);
}

LUA_API void luaW_rawseti(lua_State* L, int idx, int n) {
    lua_jmpbuf jb;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        lua_rawseti(L, idx, n);
    luaD_exit(L, &jb);
}

LUA_API int luaW_setmetatable(lua_State* L, int objindex) {
    lua_jmpbuf jb;
    int ret = 0;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        ret = lua_setmetatable(L, objindex);
    luaD_exit(L, &jb);
    return ret;
}

LUA_API int luaW_yield(lua_State* L, int nresults) {
    lua_jmpbuf jb;
    int ret = 0;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        ret = lua_yield(L, nresults);
    luaD_exit(L, &jb);
    return ret;
}

LUA_API int luaW_break(lua_State* L) {
    lua_jmpbuf jb;
    int ret = 0;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        ret = lua_break(L);
    luaD_exit(L, &jb);
    return ret;
}

LUA_API int luaW_next(lua_State* L, int idx) {
    lua_jmpbuf jb;
    int ret = 0;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        ret = lua_next(L, idx);
    luaD_exit(L, &jb);
    return ret;
}

LUA_API void luaW_concat(lua_State* L, int n) {
    lua_jmpbuf jb;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        lua_concat(L, n);
    luaD_exit(L, &jb);
}

LUA_API void luaW_setlightuserdataname(lua_State* L, int tag, const char* name) {
    lua_jmpbuf jb;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        lua_setlightuserdataname(L, tag, name);
    luaD_exit(L, &jb);
}

LUA_API void luaW_clonefunction(lua_State* L, int idx) {
    lua_jmpbuf jb;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        lua_clonefunction(L, idx);
    luaD_exit(L, &jb);
}

LUA_API void luaW_cleartable(lua_State* L, int idx) {
    lua_jmpbuf jb;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        lua_cleartable(L, idx);
    luaD_exit(L, &jb);
}

LUA_API void luaW_clonetable(lua_State* L, int idx) {
    lua_jmpbuf jb;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        lua_clonetable(L, idx);
    luaD_exit(L, &jb);
}

LUA_API int luaLW_newmetatable(lua_State* L, const char* tname){
    lua_jmpbuf jb;
    int ret = 0;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        ret = luaL_newmetatable(L, tname);
    luaD_exit(L, &jb);
    return ret;
}

LUA_API const char* luaLW_tolstring(lua_State* L, int idx, size_t* len) {
    lua_jmpbuf jb;
    const char* ret = nullptr;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        ret = luaL_tolstring(L, idx, len);
    luaD_exit(L, &jb);
    return ret;
}

LUA_API const char* luaLW_findtable(lua_State* L, int idx, const char* fname, int szhint) {
    lua_jmpbuf jb;
    const char* ret = nullptr;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        ret = luaL_findtable(L, idx, fname, szhint);
    luaD_exit(L, &jb);
    return ret;
}

LUA_API const char* luaLW_typename(lua_State* L, int idx) {
    lua_jmpbuf jb;
    const char* ret = nullptr;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        ret = luaL_typename(L, idx);
    luaD_exit(L, &jb);
    return ret;
}

LUA_API void luaLW_typeerror(lua_State* L, int narg, const char* tname) {
    lua_jmpbuf jb;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        luaL_typeerror(L, narg, tname);
    luaD_exit(L, &jb);
}

LUA_API void luaLW_argerror(lua_State* L, int narg, const char* extramsg) {
    lua_jmpbuf jb;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        luaL_argerror(L, narg, extramsg);
    luaD_exit(L, &jb);
}

LUA_API int luaLW_checkboolean(lua_State* L, int narg) {
    lua_jmpbuf jb;
    int ret = 0;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        ret = luaL_checkboolean(L, narg);
    luaD_exit(L, &jb);
    return ret;
}

LUA_API void* luaLW_checkudata(lua_State* L, int ud, const char* tname) {
    lua_jmpbuf jb;
    void* ret = nullptr;
    luaD_enter(L, &jb);
    if (LUAU_SETJMP(jb.buf) == 0)
        ret = luaL_checkudata(L, ud, tname);
    luaD_exit(L, &jb);
    return ret;
}
