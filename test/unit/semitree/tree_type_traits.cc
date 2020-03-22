//------------------------------------------------------------------------------
//
// Type traits tests for various semitree classes.
//
// These tests are compile-time only and consist of static assertions.
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#include "tree.h"

#include <type_traits>
#include <utility>

// Sibling iterators {{
static_assert(std::is_trivially_copyable_v<sib_it>);
static_assert(std::is_trivially_copyable_v<const_sib_it>);

// Just to be sure. Iterators cannot be trivially default constructible
// since they are setting pointer to null in default ctor.
static_assert(std::is_nothrow_default_constructible_v<sib_it>);
static_assert(std::is_trivially_copy_constructible_v<sib_it>);
static_assert(std::is_trivially_move_constructible_v<sib_it>);
static_assert(std::is_trivially_copy_assignable_v<sib_it>);
static_assert(std::is_trivially_move_assignable_v<sib_it>);

static_assert(std::is_nothrow_default_constructible_v<const_sib_it>);
static_assert(std::is_trivially_copy_constructible_v<const_sib_it>);
static_assert(std::is_trivially_move_constructible_v<const_sib_it>);
static_assert(std::is_trivially_copy_assignable_v<const_sib_it>);
static_assert(std::is_trivially_move_assignable_v<const_sib_it>);

// is_nothrow_convertible presents only in c++20...
static_assert(std::is_convertible_v<sib_it, const_sib_it>);
static_assert(std::is_nothrow_constructible_v<const_sib_it, sib_it>);
static_assert(!std::is_convertible_v<const_sib_it, sib_it>);
static_assert(!std::is_constructible_v<sib_it, const_sib_it>);

static_assert(std::is_same_v<typename sib_it::value_type, node>);
static_assert(std::is_same_v<typename sib_it::pointer, node *>);
static_assert(std::is_same_v<typename sib_it::reference, node &>);
static_assert(std::is_same_v<typename const_sib_it::value_type, const node>);
static_assert(std::is_same_v<typename const_sib_it::pointer, const node *>);
static_assert(std::is_same_v<typename const_sib_it::reference, const node &>);
// }} sibling iterators

// Inorder iterators {{
static_assert(std::is_trivially_copyable_v<ino_it>);
static_assert(std::is_trivially_copyable_v<const_ino_it>);

// Just to be sure. Iterators cannot be trivially default constructible
// since they are setting pointer to null in default ctor.
static_assert(std::is_nothrow_default_constructible_v<ino_it>);
static_assert(std::is_trivially_copy_constructible_v<ino_it>);
static_assert(std::is_trivially_move_constructible_v<ino_it>);
static_assert(std::is_trivially_copy_assignable_v<ino_it>);
static_assert(std::is_trivially_move_assignable_v<ino_it>);

static_assert(std::is_nothrow_default_constructible_v<const_ino_it>);
static_assert(std::is_trivially_copy_constructible_v<const_ino_it>);
static_assert(std::is_trivially_move_constructible_v<const_ino_it>);
static_assert(std::is_trivially_copy_assignable_v<const_ino_it>);
static_assert(std::is_trivially_move_assignable_v<const_ino_it>);

// is_nothrow_convertible presents only in c++20...
static_assert(std::is_convertible_v<ino_it, const_ino_it>);
static_assert(std::is_nothrow_constructible_v<const_ino_it, ino_it>);
static_assert(!std::is_convertible_v<const_ino_it, ino_it>);
static_assert(!std::is_constructible_v<ino_it, const_ino_it>);

static_assert(std::is_same_v<decltype(std::declval<ino_it>()->ref), node &>);
static_assert(
    std::is_same_v<decltype(std::declval<const_ino_it>()->ref), const node &>);
// }} inorder iterators

// Iterator interoperation {{
static_assert(std::is_nothrow_constructible_v<ino_it, sib_it, bool>);
static_assert(
    std::is_nothrow_constructible_v<const_ino_it, const_sib_it, bool>);
static_assert(std::is_nothrow_constructible_v<const_ino_it, sib_it, bool>);
// }} iterator interoperation
