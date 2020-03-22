//------------------------------------------------------------------------------
//
// Iteration tests for intrusive inorder semitree.
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#include "tree.h"

#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <iterator>
#include <numeric>

BOOST_AUTO_TEST_SUITE(semitree)

BOOST_AUTO_TEST_SUITE(iteration)

BOOST_AUTO_TEST_CASE(simple_sibling) {
  tree tr;
  leaf l1{1};
  leaf l2{2};
  tr.insert(tr.end(), l1);
  tr.insert(tr.end(), l2);

  auto res =
      std::accumulate(tr.begin(), tr.end(), 0, [](int acc, const node &n) {
        return acc + n.get_data();
      });
  BOOST_TEST(res == 3);

  res = std::accumulate(
      std::reverse_iterator{tr.end()}, std::reverse_iterator{tr.begin()}, 0,
      [](int acc, const node &n) { return acc + n.get_data(); });
  BOOST_TEST(res == 3);

  std::vector<int> ns;
  std::transform(tr.begin(), tr.end(), std::reverse_iterator{tr.end()},
                 std::back_inserter(ns), [](const node &a, const node &b) {
                   return a.get_data() * b.get_data();
                 });
  res = std::accumulate(ns.begin(), ns.end(), 0,
                        [](int acc, int val) { return acc + val; });
  // 1*2 + 2*1.
  BOOST_TEST(res == 4);
}

BOOST_AUTO_TEST_CASE(inorder) {
  tree tr;
  branch root{1};
  leaf l1{2};
  leaf l2{3};
  branch b1{4};
  leaf l3{5};
  branch b2{6};

  //    root{1}
  //   l1{2}  b1{4}
  //        b2{6}  l2{3}
  //        l3{5}
  auto ino_ins_pt = tr.inorder_end();
  auto oldit = tr.insert(ino_ins_pt, root);
  BOOST_TEST(oldit == ino_ins_pt);
  oldit = tr.insert(--ino_ins_pt, l1);
  BOOST_TEST(oldit == ino_ins_pt);
  oldit = tr.insert(ino_ins_pt, b1);
  BOOST_TEST(oldit == ino_ins_pt);
  oldit = tr.insert(--ino_ins_pt, l2);
  BOOST_TEST(oldit == ino_ins_pt);
  oldit = tr.insert(--ino_ins_pt, b2);
  BOOST_TEST(oldit == ino_ins_pt);
  oldit = tr.insert(--ino_ins_pt, l3);
  BOOST_TEST(oldit == ino_ins_pt);

  // Test children of each branch.
  auto sib_sum = [](int acc, const node &n) { return acc + n.get_data(); };
  BOOST_TEST(std::accumulate(root.begin(), root.end(), 0, sib_sum) == 6);
  BOOST_TEST(std::accumulate(b1.begin(), b1.end(), 0, sib_sum) == 9);
  BOOST_TEST(std::accumulate(b2.begin(), b2.end(), 0, sib_sum) == 5);

  // Test inorder traversal.
  auto beg{tr.inorder_begin()};
  auto end{tr.inorder_end()};
  auto rbeg = std::reverse_iterator{end};
  auto rend = std::reverse_iterator{beg};
  auto ino_sum = [](int acc, const auto &desc) {
    return acc + desc.ref.get_data();
  };
  // 1+2+4+6+5+6+3+4+1 == 32.
  BOOST_TEST(std::accumulate(beg, end, 0, ino_sum) == 32);
  BOOST_TEST(std::accumulate(rbeg, rend, 0, ino_sum) == 32);
  std::vector<int> ns;
  auto nsbi = std::back_inserter(ns);
  auto ino_zipmul = [](const auto &d1, const auto &d2) {
    return d1.ref.get_data() * d2.ref.get_data();
  };
  // 1,2,4,6,5,6,3,4,1
  // 1,4,3,6,5,6,4,2,1
  // 1*1+2*4+4*3+6*6+5*5+6*6+3*4+4*2+1*1 == 139
  std::transform(beg, end, rbeg, nsbi, ino_zipmul);
  BOOST_TEST(std::accumulate(ns.begin(), ns.end(), 0) == 139);

  // Test preorder traversal.
  auto pre_sum = [](int acc, const auto &desc) {
    if (desc.visited && desc.ref.is_branch())
      return acc;
    return acc + desc.ref.get_data();
  };
  // 1+2+4+6+5+3 == 21.
  BOOST_TEST(std::accumulate(beg, end, 0, pre_sum) == 21);
  BOOST_TEST(std::accumulate(rbeg, rend, 0, pre_sum) == 21);

  // Unvisited zipmul check to verify order.
  // Actually this is not preorder zip. Skipped nodes return 0,
  // so result will differ from real preorder zip.
  auto unvisited_zipmul = [](const auto &d1, const auto &d2) {
    auto d1val = d1.ref.get_data();
    if (d1.visited && d1.ref.is_branch())
      d1val = 0;
    auto d2val = d2.ref.get_data();
    if (d2.visited && d2.ref.is_branch())
      d2val = 0;
    return d1val * d2val;
  };
  ns.clear();
  // 1,2,4,6,5,0,3,0,0
  // 0,0,3,0,5,6,4,2,1
  // 1*0+2*0+4*3+6*0+5*5+0*6+3*4+0*2+0*1 == 49
  std::transform(beg, end, rbeg, nsbi, unvisited_zipmul);
  BOOST_TEST(std::accumulate(ns.begin(), ns.end(), 0) == 49);

  // Test postorder traversal.
  auto post_sum = [](int acc, const auto &desc) {
    if (!desc.visited && desc.ref.is_branch())
      return acc;
    return acc + desc.ref.get_data();
  };
  // 2+5+6+3+4+1 == 21.
  BOOST_TEST(std::accumulate(beg, end, 0, post_sum) == 21);
  BOOST_TEST(std::accumulate(rbeg, rend, 0, post_sum) == 21);

  // Visited zipmul check to verify order.
  // Actually this is not postorder zip. Skipped nodes return 0,
  // so result will differ from real postorder zip.
  auto visited_zipmul = [](const auto &d1, const auto &d2) {
    auto d1val = d1.ref.get_data();
    if (!d1.visited && d1.ref.is_branch())
      d1val = 0;
    auto d2val = d2.ref.get_data();
    if (!d2.visited && d2.ref.is_branch())
      d2val = 0;
    return d1val * d2val;
  };
  ns.clear();
  // 0,2,0,0,5,6,3,4,1
  // 1,4,3,6,5,0,0,2,0
  // 0*1+2*4+0*3+0*6+5*5+6*0+3*0+4*2+1*0 == 41
  std::transform(beg, end, rbeg, nsbi, visited_zipmul);
  BOOST_TEST(std::accumulate(ns.begin(), ns.end(), 0) == 41);
}

BOOST_AUTO_TEST_SUITE_END() // iteration

BOOST_AUTO_TEST_SUITE_END() // semitree
