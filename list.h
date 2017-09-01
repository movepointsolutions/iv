#ifndef IV_LIST_H
#define IV_LIST_H

#include <algorithm>
#include <exception>
#include <iostream>
#include <iterator>
#include <memory>
#include <vector>
#include "simple_ptr.h"

namespace iv
{

namespace internal
{

template <class T>
struct tree;

template <class T>
struct tree_base
{
	tree<T> *l, *r; // left, right
	tree_base *p; // parent

	tree_base(tree<T> *_l, tree<T> *_r) : l(_l), r(_r) { }

	void resurrect();
};

template <class T>
struct tree_head : public tree_base<T>
{
	tree_head(tree<T> *_root = nullptr) : tree_base<T>(_root, nullptr) { }
	tree<T> *root()
	{
		return this->l;
	}
};

template <class T>
struct tree : public tree_base<T>
{
	T v; // contained value
	int h; // height

	int height() const
	{
		if (this == nullptr)
			return 0;
		else
			return h;
	}

	tree(tree *_l, const T &_v, tree *_r)
		: tree_base<T>(_l, _r), v(_v), h(std::max(this->l->height(), this->r->height()) + 1)
	{
		if (this->l)
			this->l->p = this;
		if (this->r)
			this->r->p = this;
	}

	tree *add_min(char x);
	tree *add_max(char x);
protected:
	static tree *balance(tree *l, const T &v, tree *r);
};

template <class T>
void tree_base<T>::resurrect()
{
	//std::cerr << "Resurrecting " << this << std::endl;
	if (l) {
		l->p = this;
		l->resurrect();
	}
	if (r) {
		r->p = this;
		r->resurrect();
	}
}

/*
    let bal l v r =
      let hl = match l with Empty -> 0 | Node {h} -> h in
      let hr = match r with Empty -> 0 | Node {h} -> h in
      if hl > hr + 2 then begin
        match l with
          Empty -> invalid_arg "Set.bal"
        | Node{l=ll; v=lv; r=lr} ->
            if height ll >= height lr then
              create ll lv (create lr v r)
            else begin
              match lr with
                Empty -> invalid_arg "Set.bal"
              | Node{l=lrl; v=lrv; r=lrr}->
                  create (create ll lv lrl) lrv (create lrr v r)
            end
      end else if hr > hl + 2 then begin
        match r with
          Empty -> invalid_arg "Set.bal"
        | Node{l=rl; v=rv; r=rr} ->
            if height rr >= height rl then
              create (create l v rl) rv rr
            else begin
              match rl with
                Empty -> invalid_arg "Set.bal"
              | Node{l=rll; v=rlv; r=rlr} ->
                  create (create l v rll) rlv (create rlr rv rr)
            end
      end else
        Node{l; v; r; h=(if hl >= hr then hl + 1 else hr + 1)}
*/

template <class T>
tree<T> *tree<T>::balance(tree<T> *l, const T &v, tree<T> *r)
{
	if (l->height() > r->height() + 2) {
		if (l->l->height() >= l->r->height()) {
			auto right = new tree(l->r, v, r);
			return new tree(l->l, l->v, right);
		} else {
			auto left = new tree(l->l, l->v, l->r->l);
			auto right = new tree(l->r->r, v, r);
			return new tree(left, l->r->v, right);
		}
	} else if (r->height() > l->height() + 2) {
		if (r->r->height() >= r->l->height()) {
			auto left = new tree(l, v, r->l);
			return new tree(left, r->v, r->r);
		} else {
			auto right = new tree(r->l->r, l->v, r->r);
			auto left = new tree(l, v, r->l->l);
			return new tree(left, r->l->v, right);
		}
	} else
		return new tree(l, v, r);
}

template <class T>
tree<T> *tree<T>::add_min(char x)
{
	if (this == nullptr)
		return new tree(nullptr, x, nullptr);
	else
		return balance(this->l->add_min(x), v, this->r);
}

template <class T>
tree<T> *tree<T>::add_max(char x)
{
	if (this == nullptr)
		return new tree(nullptr, x, nullptr);
	else
		return balance(this->l, v, this->r->add_max(x));
}

} // namespace iv::internal

template <class T>
class list_const_iterator : public simple_ptr<const internal::tree_base<T>>
{
public:
	using iterator_category = std::bidirectional_iterator_tag;
	using value_type = const internal::tree_base<T>;
	using difference_type = int;
	using pointer = const internal::tree_base<T> *;
	using reference = const internal::tree_base<T> &;

	list_const_iterator(const internal::tree_base<T> *node = nullptr)
		: simple_ptr<const internal::tree_base<T>>(node)
	{
	}

	list_const_iterator left()
	{
		return list_const_iterator((*this)->l);
	}
	list_const_iterator right()
	{
		return list_const_iterator((*this)->r);
	}
	list_const_iterator parent()
	{
		return list_const_iterator((*this)->p);
	}

	bool operator ==(const list_const_iterator &other)
	{
		return this->get() == other.get();
	}
	bool operator !=(const list_const_iterator &other)
	{
		return this->get() != other.get();
	}

	operator bool()
	{
		return *this != list_const_iterator();
	}

	list_const_iterator &operator ++()
	{
		if (right()) {
			*this = right();
			while (left())
				*this = left();
		} else {
			while (*this == parent().right())
				*this = parent();
			*this = parent();
		}
		return *this;
	}
	const T &operator *()
	{
		return static_cast<const internal::tree<T> *>(this->get())->v;
	}
};

template <class T>
class list
{
	simple_ptr<internal::tree_head<T>> head;
public:
	typedef list_const_iterator<T> const_iterator;

	list() : head(new internal::tree_head<T>())
	{
	}

	const_iterator begin() const
	{
		const_iterator ret(head);
		while (ret.left())
			ret = ret.left();
		return ret;
	}

	const_iterator end() const
	{
		return const_iterator(head);
	}

	const_iterator root()
	{
		return const_iterator(head->root());
	}

	void push_front(char c)
	{
		head->l = head->root()->add_min(c);
		head->l->p = head;
	}
	void push_back(char c)
	{
		head->l = head->root()->add_max(c);
		head->l->p = head;
		//std::cout << "!" << head->l << " " << head->root()->c << std::endl;
	}
};

} // namespace iv

#endif // IV_LIST_H
