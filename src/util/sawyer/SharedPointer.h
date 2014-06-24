#ifndef Sawyer_SharedPtr_H
#define Sawyer_SharedPtr_H

#include <cstddef>
#include <ostream>
#include <sawyer/Assert.h>
#include <sawyer/Sawyer.h>

namespace Sawyer {

/** Reference-counting smart pointer.
 *
 *  This class is a reference-counting pointer to an object that inherits from @ref SharedObject. Usage is similar to
 *  <code>boost::shared_ptr</code>.
 *
 *  @todo Write documentation for SharedPointer. */
template<class T>
class SharedPointer {
public:
    typedef T Pointee;
private:
    Pointee *pointee_;

    static void aquireOwnership(Pointee *rawPtr);

    // Returns number of owners remaining
    static size_t releaseOwnership(Pointee *rawPtr);

public:
    /** Constructs an empty shared pointer. */
    SharedPointer(): pointee_(NULL) {}

    /** Constructs a new pointer that shares ownership of the pointed-to object with the @p other pointer. The pointed-to
     *  object will only be deleted after both pointers are deleted.
     * @{ */
    SharedPointer(const SharedPointer &other): pointee_(other.pointee_) {
        aquireOwnership(pointee_);
    }
    template<class Y>
    SharedPointer(const SharedPointer<Y> &other): pointee_(get(other)) {
        aquireOwnership(pointee_);
    }
    /** @} */
    
    /** Constructs a shared pointer for an object.
     *
     *  If @p obj is non-null then its reference count is incremented. It is possible to create any number of shared pointers
     *  to the same object using this constructor. The expression "delete obj" must be well formed and must not invoke
     *  undefined behavior. */
    template<class Y>
    explicit SharedPointer(Y *rawPtr): pointee_(rawPtr) {
        if (pointee_!=NULL)
            aquireOwnership(pointee_);
    }

    /** Conditionally deletes the pointed-to object.  The object is deleted when its reference count reaches zero. */
    ~SharedPointer() {
        if (0==releaseOwnership(pointee_))
            delete pointee_;
    }

    /** Assignment. This pointer is caused to point to the same object as @p other, decrementing the reference count for the
     * object originally pointed to by this pointer and incrementing the reference count for the object pointed by @p other.
     * @{ */
    SharedPointer& operator=(const SharedPointer &other) {
        return operator=<T>(other);
    }
    template<class Y>
    SharedPointer& operator=(const SharedPointer<Y> &other) {
        if (pointee_!=get(other)) {
            if (pointee_!=NULL && 0==releaseOwnership(pointee_))
                delete pointee_;
            pointee_ = get(other);
            aquireOwnership(pointee_);
        }
        return *this;
    }
    /** @} */

    /** Reference to the pointed-to object.  An assertion will fail if assertions are enabled and this method is invoked on an
     *  empty pointer. */
    T& operator*() const {
        ASSERT_not_null2(pointee_, "shared pointer points to no object");
        ASSERT_require(ownershipCount(pointee_)>0);
        return *pointee_;
    }

    /** Dereference pointed-to object. The pointed-to object is returned. Returns null for empty pointers. */
    T* operator->() const {
        ASSERT_not_null2(pointee_, "shared pointer points to no object");
        ASSERT_require(ownershipCount(pointee_)>0);
        return pointee_;
    }
    /** @} */

    /** Dynamic cast.
     *
     *  Casts the specified pointer to a new pointer type using dynamic_cast. */
    template<class U>
    SharedPointer<U> dynamicCast() const {
        return SharedPointer<U>(dynamic_cast<U*>(pointee_));
    }
    
    /** Comparison of two pointers.
     *
     *  Comparisons operators compare the underlying pointers to objects.
     *
     *  @{ */
    template<class U>
    bool operator==(const SharedPointer<U> &other) const {
        return pointee_ == get(other);
    }
    template<class U>
    bool operator!=(const SharedPointer<U> &other) const {
        return pointee_ != get(other);
    }
    template<class U>
    bool operator<(const SharedPointer<U> &other) const {
        return pointee_ < get(other);
    }
    template<class U>
    bool operator<=(const SharedPointer<U> &other) const {
        return pointee_ <= get(other);
    }
    template<class U>
    bool operator>(const SharedPointer<U> &other) const {
        return pointee_ > get(other);
    }
    template<class U>
    bool operator>=(const SharedPointer<U> &other) const {
        return pointee_ >= get(other);
    }

    /** Comparison of two pointers.
     *
     *  Compares the underlying pointer with the specified pointer.
     *
     *  @{ */
    template<class U>
    bool operator==(const U *ptr) const {
        return pointee_ == ptr;
    }
    template<class U>
    bool operator!=(const U *ptr) const {
        return pointee_ != ptr;
    }
    template<class U>
    bool operator<(const U *ptr) const {
        return pointee_ < ptr;
    }
    template<class U>
    bool operator<=(const U *ptr) const {
        return pointee_ <= ptr;
    }
    template<class U>
    bool operator>(const U *ptr) const {
        return pointee_ > ptr;
    }
    template<class U>
    bool operator>=(const U *ptr) const {
        return pointee_ >= ptr;
    }
    /** @} */

    /** Boolean complement.
     *
     *  This operator allows shared pointers to be used in situations like this:
     *
     * @code
     *  SharedPointer<Object> obj = ...;
     *  if (!obj) ...
     * @endcode */
    bool operator!() const { return pointee_==NULL; }

    /** Print a shared pointer.
     *
     *  Printing a shared pointer is the same as printing the pointee's address. */
    friend std::ostream& operator<<(std::ostream &out, const SharedPointer &ptr) {
        out <<get(ptr);
        return out;
    }
    
    // The following trickery is to allow things like "if (x)" to work but without having an implicit
    // conversion to bool which would cause no end of other problems.
private:
    typedef void(SharedPointer::*unspecified_bool)() const;
    void this_type_does_not_support_comparisons() const {}
public:
    /** Type for Boolean context.
     *
     *  Implicit conversion to a type that can be used in a boolean context such as an <code>if</code> or <code>while</code>
     *  statement.  For instance:
     *
     *  @code
     *   SharedPointer<Object> x = ...;
     *   if (x) {
     *      //this is reached
     *   }
     *  @endcode */
    operator unspecified_bool() const {
        return pointee_ ? &SharedPointer::this_type_does_not_support_comparisons : 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                                  Functions
    // These are functions because we don't want to add any pointer methods that could be confused with methods on the
    // pointee. For instance, if the pointee has an "ownershipCount" method then errors could be easily be introduced. For
    // instance, these two lines both compile but do entirely different things:
    //     object->ownershipCount()
    //     object.ownershipCount();
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /** Obtain the pointed-to object.  The pointed-to object is returned. Returns null for empty pointers. */
    friend Pointee* get(const SharedPointer &ptr) {
        return ptr.pointee_;
    }

    /** Returns the pointed-to object's reference count. Returns zero for empty pointers. */
    friend size_t ownershipCount(const SharedPointer &ptr) {
        return ptr.ownershipCount(ptr.pointee_);
    }
private:
    static size_t ownershipCount(Pointee *rawPtr);
};



/** Make pointer point to nothing.
 *
 *  Clears the pointer so it points to no object.  A cleared pointer is like a default-constructed pointer. */
template<class T>
void clear(SharedPointer<T> &ptr) {
    ptr = SharedPointer<T>();
}
    
/** Base class for reference counted objects.
 *
 *  Any reference counted object should inherit from this class, which provides a default constructor, virtual destructor, and
 *  a private reference count data member. */
class SAWYER_EXPORT SharedObject {
    template<class U> friend class SharedPointer;
    mutable size_t nrefs_;
public:
    /** Default constructor.  Initializes the reference count to zero. */
    SharedObject(): nrefs_(0) {}

    /** Virtual destructor. Verifies that the reference count is zero. */
    virtual ~SharedObject() {
        ASSERT_require(nrefs_==0);
    }
};

/** Creates SharedPointer from this.
 *
 *  If an object inherits from SharedObject then it has a @ref sharedFromThis method that allows creation of a
 *  reference-counting pointer from <code>this</code> object. The template parameter, @p T, is the type of object that the
 *  shared pointer will point to, and is typically the type being derived from SharedObject:
 *
 * @code
 *  class MyClass: public SharedFromThis<MyClass> { ... };
 * @endcode
 *
 * @todo SharedFromThis should not inherit from SharedObject because SharedFromThis this can be added at any level of the class
 * hierarchy, but SharedObject should only be added to base classes. */
template<class T>
class SharedFromThis: public SharedObject {
public:
    /** Create a shared pointer from <code>this</code>.
     *
     *  Returns a shared pointer that points to this object. */
    SharedPointer<T> sharedFromThis() {
        T *derived = dynamic_cast<T*>(this);
        ASSERT_not_null(derived);
        return SharedPointer<T>(derived);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Implementations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
inline size_t SharedPointer<T>::ownershipCount(T *rawPtr) {
    return rawPtr==NULL ? 0 : rawPtr->SharedObject::nrefs_;
}

template<class T>
inline void SharedPointer<T>::aquireOwnership(Pointee *rawPtr) {
    if (rawPtr!=NULL)
        ++rawPtr->SharedObject::nrefs_;
}

template<class T>
inline size_t SharedPointer<T>::releaseOwnership(Pointee *rawPtr) {
    if (rawPtr!=NULL) {
        ASSERT_require2(rawPtr->SharedObject::nrefs_>0, "pointee must be owned");
        return --rawPtr->SharedObject::nrefs_;
    } else {
        return 0;
    }
}

} // namespace
#endif