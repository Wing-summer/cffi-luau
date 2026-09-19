#ifndef CFFI_CONTEXT_HH
#define CFFI_CONTEXT_HH

#include "ast.hh"

namespace ffi {

struct context {
    ast::decl_store decls;
};

} /* namespace ffi */

#endif /* CFFI_CONTEXT_HH */
