//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2004-2013. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#define STABLE_VECTOR_ENABLE_INVARIANT_CHECKING
#include <memory>

#include <boost/container/stable_vector.hpp>
#include <boost/container/node_allocator.hpp>

#include "check_equal_containers.hpp"
#include "movable_int.hpp"
#include "expand_bwd_test_allocator.hpp"
#include "expand_bwd_test_template.hpp"
#include "dummy_test_allocator.hpp"
#include "propagate_allocator_test.hpp"
#include "vector_test.hpp"
#include "default_init_test.hpp"
#include "../../intrusive/test/iterator_test.hpp"

using namespace boost::container;

class recursive_vector
{
   public:
   int id_;
   stable_vector<recursive_vector> vector_;
   stable_vector<recursive_vector>::iterator it_;
   stable_vector<recursive_vector>::const_iterator cit_;
   stable_vector<recursive_vector>::reverse_iterator rit_;
   stable_vector<recursive_vector>::const_reverse_iterator crit_;

   recursive_vector (const recursive_vector &o)
      : vector_(o.vector_)
   {}

   recursive_vector &operator=(const recursive_vector &o)
   { vector_ = o.vector_;  return *this; }
};

void recursive_vector_test()//Test for recursive types
{
   stable_vector<recursive_vector> recursive, copy;
   //Test to test both move emulations
   if(!copy.size()){
      copy = recursive;
   }
}

template<class VoidAllocator>
struct GetAllocatorCont
{
   template<class ValueType>
   struct apply
   {
      typedef stable_vector< ValueType
                           , typename allocator_traits<VoidAllocator>
                              ::template portable_rebind_alloc<ValueType>::type
                           > type;
   };
};

template<class VoidAllocator>
int test_cont_variants()
{
   typedef typename GetAllocatorCont<VoidAllocator>::template apply<int>::type MyCont;
   typedef typename GetAllocatorCont<VoidAllocator>::template apply<test::movable_int>::type MyMoveCont;
   typedef typename GetAllocatorCont<VoidAllocator>::template apply<test::movable_and_copyable_int>::type MyCopyMoveCont;
   typedef typename GetAllocatorCont<VoidAllocator>::template apply<test::copyable_int>::type MyCopyCont;

   if(test::vector_test<MyCont>())
      return 1;
   if(test::vector_test<MyMoveCont>())
      return 1;
   if(test::vector_test<MyCopyMoveCont>())
      return 1;
   if(test::vector_test<MyCopyCont>())
      return 1;

   return 0;
}

struct boost_container_stable_vector;

namespace boost { namespace container {   namespace test {

template<>
struct alloc_propagate_base<boost_container_stable_vector>
{
   template <class T, class Allocator>
   struct apply
   {
      typedef boost::container::stable_vector<T, Allocator> type;
   };
};

}}}   //namespace boost::container::test


int main()
{
   recursive_vector_test();
   {
      //Now test move semantics
      stable_vector<recursive_vector> original;
      stable_vector<recursive_vector> move_ctor(boost::move(original));
      stable_vector<recursive_vector> move_assign;
      move_assign = boost::move(move_ctor);
      move_assign.swap(original);
   }

   //Test non-copy-move operations
   {
      stable_vector<test::non_copymovable_int> sv;
      sv.emplace_back();
      sv.resize(10);
      sv.resize(1);
   }

   ////////////////////////////////////
   //    Testing allocator implementations
   ////////////////////////////////////
   //       std:allocator
   if(test_cont_variants< std::allocator<void> >()){
      std::cerr << "test_cont_variants< std::allocator<void> > failed" << std::endl;
      return 1;
   }
   //       boost::container::node_allocator
   if(test_cont_variants< node_allocator<void> >()){
      std::cerr << "test_cont_variants< node_allocator<void> > failed" << std::endl;
      return 1;
   }

   ////////////////////////////////////
   //    Default init test
   ////////////////////////////////////
   if(!test::default_init_test< stable_vector<int, test::default_init_allocator<int> > >()){
      std::cerr << "Default init test failed" << std::endl;
      return 1;
   }

   ////////////////////////////////////
   //    Emplace testing
   ////////////////////////////////////
   const test::EmplaceOptions Options = (test::EmplaceOptions)(test::EMPLACE_BACK | test::EMPLACE_BEFORE);
   if(!boost::container::test::test_emplace
      < stable_vector<test::EmplaceInt>, Options>())
      return 1;

   ////////////////////////////////////
   //    Allocator propagation testing
   ////////////////////////////////////
   if(!boost::container::test::test_propagate_allocator<boost_container_stable_vector>())
      return 1;

   ////////////////////////////////////
   //    Initializer lists testing
   ////////////////////////////////////
   if(!boost::container::test::test_vector_methods_with_initializer_list_as_argument_for
      < boost::container::stable_vector<int> >())
   {
       std::cerr << "test_methods_with_initializer_list_as_argument failed" << std::endl;
       return 1;
   }

   ////////////////////////////////////
   //    Iterator testing
   ////////////////////////////////////
   {
      typedef boost::container::stable_vector<int> cont_int;
      for (std::size_t i = 10; i <= 10000; i *= 10) {
         cont_int a;
         for (int j = 0; j < (int)i; ++j)
            a.push_back((int)j);
         boost::intrusive::test::test_iterator_random< cont_int >(a);
         if (boost::report_errors() != 0) {
            return 1;
         }
      }
   }

#ifndef BOOST_CONTAINER_NO_CXX17_CTAD
   ////////////////////////////////////
   //    Constructor Template Auto Deduction testing
   ////////////////////////////////////
   {
      auto gold = std::vector{ 1, 2, 3 };
      auto test = boost::container::stable_vector(gold.begin(), gold.end());
      if (test.size() != 3) {
         return 1;
      }
      if (!(test[0] == 1 && test[1] == 2 && test[2] == 3)) {
         return 1;
      }
   }
#endif

   ////////////////////////////////////
   //    has_trivial_destructor_after_move testing
   ////////////////////////////////////
   // default allocator
   {
      typedef boost::container::stable_vector<int> cont;
      typedef cont::allocator_type allocator_type;
      typedef boost::container::allocator_traits<allocator_type>::pointer pointer;
      BOOST_CONTAINER_STATIC_ASSERT_MSG(
        !(boost::has_trivial_destructor_after_move<cont>::value !=
          boost::has_trivial_destructor_after_move<allocator_type>::value &&
          boost::has_trivial_destructor_after_move<pointer>::value)
        , "has_trivial_destructor_after_move(default allocator) test failed");
   }
   // std::allocator
   {
      typedef boost::container::stable_vector<int, std::allocator<int> > cont;
      typedef cont::allocator_type allocator_type;
      typedef boost::container::allocator_traits<allocator_type>::pointer pointer;
      BOOST_CONTAINER_STATIC_ASSERT_MSG(
        !(boost::has_trivial_destructor_after_move<cont>::value !=
          boost::has_trivial_destructor_after_move<allocator_type>::value &&
          boost::has_trivial_destructor_after_move<pointer>::value)
        , "has_trivial_destructor_after_move(std::allocator) test failed");
   }

   return 0;
}
