// -*- C++ -*-

/*! \file
 * \brief Catch-alls for all memory allocators
 */

#ifndef QDP_ALLOCATOR
#define QDP_ALLOCATOR

#include "qdp_config.h"
#include "qdp_singleton.h"
/*! QDP Allocator
 *  A raw memory allocator for QDP, for particular use 
 *  with OLattice Objects. The pointers returned by allocate
 *  are all allocated with the correct alignment
 *  On normal targets this should be QDP_ALIGNMENT
 *  whereas on other targets (QCDOC) this should be the default
 *  alignment
 */
namespace __QDP__ 
{
  namespace Allocator 
  {

    enum MemoryPoolHint { DEFAULT, FAST };

  } // namespace Allocator

  // Memory movement hints
  namespace Hints
  {

    //! Hint to move a generic object of type T to fast memory.
    /*!
     * \ingroup QDP Memory management hints
     * 
     * This is a catch all function for objects that do not support
     * memory management hints. It does nothing and had better be inlined
     * or it can be detremental to my health
     *
     * \param x   The object for which the hint is meant
     * \param copy Whether to copy the object's slow memory contents to 
     *             its new fast memory home. Default is no.
     */
    template<typename T> 
    inline
    void moveToFastMemoryHint(T& x, bool copy=false) {}

    //! Hint to return a generic object of type T from fast memory.
    /*!
     * \ingroup QDP Memory management hints
     * 
     * This is a catch all function for objects that do not support
     * memory management hints. It does nothing and had better be inlined
     * or it can be detremental to my health
     *
     * \param x   The object for which the hint is meant
     * \param copy Whether to copy the object's fast memory contents to 
     *             back to its slow memory home. Default is no.
     */
    template<typename T>
    inline
    void revertFromFastMemoryHint(T& x, bool copy=false) {}

  } // namespace Hints

} // namespace __QDP__


// Now include the allocator.h-s and turn into singleton as approprite.
#ifdef QDP_USE_TBBPOOL_ALLOCATOR
#include "qdp_pool_allocator.h"
namespace __QDP__ {
 namespace Allocator {
 using theQDPAllocator = SingletonHolder<__QDP__::Allocator::QDPPoolAllocator,
			    __QDP__::CreateStatic,
			    __QDP__::NoDestroy,
			    __QDP__::SingleThreaded>;
 }
}
#else
// Include the default specialisation
#include "qdp_default_allocator.h"
namespace __QDP__ {
 namespace Allocator {
 using theQDPAllocator =  SingletonHolder<__QDP__::Allocator::QDPDefaultAllocator,
			    __QDP__::CreateStatic,
			    __QDP__::NoDestroy,
			    __QDP__::SingleThreaded>;
 }
 }
#endif


#endif
