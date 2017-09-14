/*
Userspace Virtual Filesystem

Copyright (C) 2015 Sebastian Herbord. All rights reserved.

This file is part of usvfs.

usvfs is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

usvfs is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with usvfs. If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once
#include <cstdint>
#include <scoped_allocator>

namespace usvfs {
namespace shared {

// template <typename T>
// using OffsetPtrT = bi::offset_ptr<T, std::int32_t, std::uint64_t>;
// typedef OffsetPtrT<void> VoidPointerT;

// important: the windows shared memory mechanism, unlike other impelementations
// automatically removes the SHM object when there are no more "subscribers".
// MO currently depends on that feature!

// managed_windows_shared_memory apparently doesn't support sharing between
// 64bit and 32bit processes
// typedef bi::basic_managed_windows_shared_memory<char, bi::rbtree_best_fit<bi::mutex_family, VoidPointerT, 8>,
//                                                bi::iset_index>
//    managed_windows_shared_memory;

// typedef managed_windows_shared_memory SharedMemoryT;

// typedef SharedMemoryT::segment_manager SegmentManagerT;
// TODO: Specialize SegmentManagerT
typedef std::scoped_allocator_adaptor<std::allocator<void>> VoidAllocatorT;
typedef VoidAllocatorT::rebind<char>::other CharAllocatorT;

typedef std::basic_string<char, std::char_traits<char>, CharAllocatorT> StringT;

// FIXME:
 typedef void* SharedMemoryT;

} // namespace shared
} // namespace usvfs
