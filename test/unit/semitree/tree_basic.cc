//------------------------------------------------------------------------------
//
// Basic tests for intrusive inorder semitree.
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#include "tree.h"

#include <boost/test/unit_test.hpp>
#include <iterator>

BOOST_AUTO_TEST_SUITE(semitree)

BOOST_AUTO_TEST_SUITE(basic)

// Basic node type check.
BOOST_AUTO_TEST_CASE(node_type) {
  leaf l;
  BOOST_TEST(!l.is_branch());
  branch b;
  BOOST_TEST(b.is_branch());
}

// Test that newly created branch is empty.
BOOST_AUTO_TEST_CASE(branch_empty) {
  branch b;
  BOOST_TEST(b.empty());
  BOOST_TEST(b.begin() == b.end());
}

// Check that sibling iterators work for one node.
BOOST_AUTO_TEST_CASE(sibling_iterator) {
  leaf l;
  sib_it lit1{&l};
  BOOST_TEST(lit1 == lit1);
  sib_it lit2 = l.get_sibling_iterator();
  BOOST_TEST(lit1 == lit2);
  BOOST_TEST(&*lit1 == &l);

  leaf l2;
  sib_it lit3 = l2.get_sibling_iterator();
  BOOST_TEST(lit1 != lit3);

  branch b;
  sib_it bit1{&b};
  BOOST_TEST(bit1 == bit1);
  sib_it bit2{b.begin()};
  sib_it bit3{b.end()};
  BOOST_TEST(bit1 != bit2);
  BOOST_TEST(bit1 != bit3);
  BOOST_TEST(&*bit1 == &b);

  BOOST_TEST(lit1 != bit1);
  BOOST_TEST(lit1 != bit2);

  const branch &cb{b};
  const_sib_it cbit1{&cb};
  const_sib_it cbit2{bit1};
  BOOST_TEST(cbit1 == cbit1);
  BOOST_TEST(cbit1 == cbit2);
  BOOST_TEST(cbit1 == bit1);
  BOOST_TEST(bit2 != cbit2);
  BOOST_TEST(cb.begin() == b.begin());
  BOOST_TEST(b.end() == cb.end());
}

// Check that inorder iterators work for one node.
BOOST_AUTO_TEST_CASE(inorder_iterator) {
  leaf l;
  ino_it lit1{&l, false};
  BOOST_TEST(lit1 == lit1);
  auto lref1 = *lit1;
  BOOST_TEST(&lref1.ref == &l);
  BOOST_TEST(&lit1->ref == &l);
  BOOST_TEST(lref1.visited == false);
  BOOST_TEST(lit1->visited == false);

  ino_it lit2{&l, true};
  auto lref2 = *lit2;
  BOOST_TEST(lref2.visited == true);
  BOOST_TEST(lit1 == lit2);

  leaf l2;
  ino_it lit3{&l2, false};
  BOOST_TEST(lit1 != lit3);
  BOOST_TEST(lit2 != lit3);

  branch b;
  ino_it bit1{&b, false};
  ino_it bit2{&b, true};
  BOOST_TEST(bit1 == bit1);
  BOOST_TEST(bit2 == bit2);
  BOOST_TEST(bit1 != bit2);

  BOOST_TEST(lit1 != bit1);
  BOOST_TEST(lit2 != bit1);
  BOOST_TEST(lit3 != bit1);
  BOOST_TEST(lit1 != bit2);
  BOOST_TEST(lit2 != bit2);
  BOOST_TEST(lit3 != bit2);
}

BOOST_AUTO_TEST_CASE(empty_tree_iterator) {
  tree tr;
  BOOST_TEST(tr.empty());
  BOOST_TEST(tr.inorder_begin() == tr.inorder_end());
}

// Check that newly created node has no parent.
BOOST_AUTO_TEST_CASE(parent) {
  leaf l;
  BOOST_TEST(!l.has_parent());
  branch b;
  BOOST_TEST(!b.has_parent());
}

// Basic tree insertion tests.
BOOST_AUTO_TEST_SUITE(tree_insert)

// Check that left and right same-level iterators tied to each other.
template <typename It> void test_simple_iterators(It lit, It rit) {
  BOOST_TEST(lit != rit);
  auto litold = lit++;
  BOOST_TEST(lit == rit);
  BOOST_TEST(litold == --lit);
  auto ritold = rit--;
  BOOST_TEST(lit == rit);
  BOOST_TEST(ritold == ++rit);
}

// Simple list from two nodes. Check sibling insertion.
BOOST_AUTO_TEST_CASE(sibling) {
  tree tr;
  leaf left;
  leaf right;
  //   root
  // left right
  auto insit = tr.insert(tr.end(), right);
  BOOST_TEST(insit == tr.end());
  tr.insert(right.get_sibling_iterator(), left);
  BOOST_TEST(&left.get_next() == &right);
  BOOST_TEST(&left == &right.get_prev());

  auto lit{left.get_sibling_iterator()};
  auto rit{right.get_sibling_iterator()};
  test_simple_iterators(lit, rit);
}

// Simple list from two nodes. Check leaf inorder insertion.
BOOST_AUTO_TEST_CASE(inorder_leaf) {
  tree tr;
  leaf left;
  leaf right;
  ino_it rit{&right, false};
  //   root
  // left right
  tr.insert(tr.begin(), right);
  tr.insert(rit, left);
  BOOST_TEST(&left.get_next() == &right);
  BOOST_TEST(&left == &right.get_prev());

  ino_it lit{&left, false};
  test_simple_iterators(lit, rit);
}

// Simple list from two nodes. Check leaf+branch inorder insertion.
BOOST_AUTO_TEST_CASE(inorder_branch_list) {
  tree tr;
  leaf l;
  branch b;
  ino_it bit{&b, false};
  // root
  // b  l
  tr.insert(tr.begin(), b);
  tr.insert(bit, l);
  ino_it lit{&l, false};
  test_simple_iterators(lit, bit);
}

// Tree from branch and one leaf child.
BOOST_AUTO_TEST_CASE(inorder_branch_tree) {
  tree tr;
  leaf l;
  branch b;
  ino_it bit{&b, true};
  // root
  //  b
  //  l
  tr.insert(tr.begin(), b);
  tr.insert(bit, l);
  BOOST_TEST(!b.empty());
  BOOST_TEST(b.begin() != b.end());
  BOOST_TEST(&b.get_firstchild() == &l);
  BOOST_TEST(&b.get_lastchild() == &l);
  auto slit{l.get_sibling_iterator()};
  BOOST_TEST(b.begin() == slit);
  BOOST_TEST(--b.end() == slit);
  BOOST_TEST(l.has_parent());
  BOOST_TEST(&l.get_parent() == &b);

  auto nbit{bit};
  ino_it ilit{&l, false};
  BOOST_TEST(--nbit == ilit);
  BOOST_TEST(++nbit == bit);
  std::advance(nbit, -2);
  BOOST_TEST(nbit->ref.is_branch());
  BOOST_TEST(&nbit->ref == &bit->ref);
  BOOST_TEST(!nbit->visited);

  test_simple_iterators(nbit, ilit);
  test_simple_iterators(ilit, bit);
}

BOOST_AUTO_TEST_SUITE_END() // tree_insert

BOOST_AUTO_TEST_SUITE_END() // basic

BOOST_AUTO_TEST_SUITE_END() // semitree
