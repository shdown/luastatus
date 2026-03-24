local PLUGIN_TABLE = ...
local DT_LL = PLUGIN_TABLE.dbustypes_lowlevel

-- Map with index: returns a new array 'ys' such that #ys == #xs and
--   ys[i] = f(xs[i], i).
local function imap(xs, f)
	local res = {}
	for i = 1, #xs do
		res[i] = f(xs[i], i)
	end
	return res
end

-- This table contains true for all type specifiers that specify a simple type.
local IS_TYPE_SIMPLE = {
	b = true,
	y = true,
	n = true,
	q = true,
	i = true,
	u = true,
	x = true,
	t = true,
	h = true,
	d = true,
	s = true,
	o = true,
	g = true,
	v = true,
}

-- Parses fmt string 'fmt' starting with (1-based) position 'i'.
-- Returns: 'dtype, j', where:
--  * 'dtype' is the parsed D-Bus type object;
--  * 'j' is the (1-based) position of the "leftover" (the part after the parsed prefix).
local function parse_dtype_impl(fmt, i)
	assert(i <= #fmt, 'fmt string terminated too early')
	local c = fmt:sub(i, i)
	if IS_TYPE_SIMPLE[c] then
		return DT_LL.mktype_simple(c), i + 1
	end
	if c == 'a' then
		local elem_dtype
		elem_dtype, i = parse_dtype_impl(fmt, i + 1)
		return DT_LL.mktype_array(elem_dtype), i
	end
	if c == '(' then
		i = i + 1
		local items_dtypes = {}
		while fmt:sub(i, i) ~= ')' do
			local item_dtype
			item_dtype, i = parse_dtype_impl(fmt, i)
			items_dtypes[#items_dtypes + 1] = item_dtype
		end
		return DT_LL.mktype_tuple(items_dtypes), i + 1
	end
	if c == '{' then
		i = i + 1
		local k_dtype, v_dtype
		k_dtype, i = parse_dtype_impl(fmt, i)
		v_dtype, i = parse_dtype_impl(fmt, i)
		assert(fmt:sub(i, i) == '}', 'invalid dict entry type fmt')
		return DT_LL.mktype_dict_entry(k_dtype, v_dtype), i + 1
	end
	error(string.format('invalid type specifier "%s"', c))
end

-- Parses the whole fmt string. Returns the parsed D-Bus type object.
local function parse_dtype(fmt)
	assert(type(fmt) == 'string', 'fmt string must be of string type')

	local dtype, i = parse_dtype_impl(fmt, 1)
	assert(i == #fmt + 1, 'extra data after fmt string')
	return dtype
end

-- Converts a Lua value 'x' into a D-Bus value object of type 'dtype'.
local function mkval_of_dtype(dtype, x)
	local category = dtype:get_category()
	if category == 'simple' then
		return DT_LL.mkval_simple(dtype, x)
	end
	if category == 'array' then
		assert(type(x) == 'table', 'array type requires a table argument')
		local elem_dtype = dtype:get_elem_type()
		local elems = imap(x, function(y, _) return mkval_of_dtype(elem_dtype, y) end)
		return DT_LL.mkval_array(elem_dtype, elems)
	end
	if category == 'tuple' then
		assert(type(x) == 'table', 'tuple type requires a table argument')
		local item_dtypes = dtype:get_item_types()
		assert(#x == #item_dtypes, 'length of given array does not match the # of elements in tuple')
		local items = imap(x, function(y, i) return mkval_of_dtype(item_dtypes[i], y) end)
		return DT_LL.mkval_tuple(items)
	end
	if category == 'dict_entry' then
		assert(type(x) == 'table', 'dict entry type requires a table argument')
		local dtype_k, dtype_v = dtype:get_kv_types()
		assert(#x == 2, 'dict entry type requires array of size 2')
		local k = mkval_of_dtype(dtype_k, x[1])
		local v = mkval_of_dtype(dtype_v, x[2])
		return DT_LL.mkval_dict_entry(k, v)
	end
	error('unexpected type') -- this should never happen
end

-- Converts a Lua value 'x' into a D-Bus value object of type given by fmt string 'fmt'.
local function mkval_from_fmt(fmt, x)
	return mkval_of_dtype(parse_dtype(fmt), x)
end

PLUGIN_TABLE.dbustypes = {
	mkval_from_fmt = mkval_from_fmt,
	mkval_of_dtype = mkval_of_dtype,
	parse_dtype = parse_dtype,
}
