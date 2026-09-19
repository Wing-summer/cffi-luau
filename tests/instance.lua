assert(cffi.cdef == nil)
assert(cffi.typeof == nil)
assert(cffi.load == nil)
assert(type(cffi.new()) == "table")

local a = cffi.new()
local b = cffi.new()

a.cdef [[
    typedef int foo_t;
    int test_add(int, int);
]]

local ok = pcall(function()
    b.typeof("foo_t")
end)
assert(not ok)

b.cdef [[
    typedef double foo_t;
]]

assert(a.sizeof("foo_t") ~= b.sizeof("foo_t"))

local ok_c = pcall(function()
    return b.C.test_add
end)
assert(not ok_c)
