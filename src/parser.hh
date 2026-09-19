#ifndef PARSER_HH
#define PARSER_HH

#include "lua.hh"
#include "ast.hh"
#include "context.hh"

namespace parser {

void init(lua_State *L);

void parse(
    lua_State *L, ffi::context &ctx, char const *input,
    char const *iend = nullptr, int paridx = -1
);

ast::c_type parse_type(
    lua_State *L, ffi::context &ctx, char const *input,
    char const *iend = nullptr, int paridx = -1
);

ast::c_expr_type parse_number(
    lua_State *L, ffi::context &ctx, ast::c_value &v, char const *input,
    char const *iend = nullptr
);

} /* namespace parser */

#endif /* PARSER_HH */
