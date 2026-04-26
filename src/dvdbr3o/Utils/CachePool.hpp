#pragma once

#include <ankerl/unordered_dense.h>

namespace dvdbr3o {
template<typename KeyT, typename ValueT>
class CachePool : public ankerl::unordered_dense::map<KeyT, ValueT> {
public:
	using ankerl::unordered_dense::map<KeyT, ValueT>::map;
};
}  // namespace dvdbr3o