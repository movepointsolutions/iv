#ifndef IV_SIMPLE_PTR_H
#define IV_SIMPLE_PTR_H

namespace iv
{

template <class T>
class simple_ptr
{
	T *ptr;
public:
	simple_ptr(T *_ptr) : ptr(_ptr) { }
	T &operator *() { return *ptr; }
	T *operator ->() { return ptr; }
	T *get() const { return ptr; }
	operator T *() const { return ptr; }
};

}

#endif // IV_SIMPLE_PTR_H
