#ifndef IV_STRING_H
#define IV_STRING_H

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

struct tree;

struct tree_base
{
	tree *l, *r; // left, right
	tree_base *p; // parent

	tree_base(tree *_l, tree *_r) : l(_l), r(_r) { }

	void resurrect();
};

struct tree_head : public tree_base
{
	tree_head(tree *_root = nullptr) : tree_base(_root, nullptr) { }
	tree *root()
	{
		return l;
	}
};

struct tree : public tree_base
{
	char v; // contained value
	int h; // height

	int height() const
	{
		if (this == nullptr)
			return 0;
		else
			return h;
	}

	tree(tree *_l, char _v, tree *_r)
		: tree_base(_l, _r), v(_v), h(std::max(l->height(), r->height()) + 1)
	{
	}

	tree *add_min(char x);
	tree *add_max(char x);
};

void tree_base::resurrect()
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

tree *balance(tree *l, char v, tree *r)
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

tree *tree::add_min(char x)
{
	if (this == nullptr)
		return new tree(nullptr, x, nullptr);
	else
		return balance(l->add_min(x), v, r);
}

tree *tree::add_max(char x)
{
	if (this == nullptr)
		return new tree(nullptr, x, nullptr);
	else
		return balance(l, v, r->add_max(x));
}

} // namespace iv::internal

class string_const_iterator : public simple_ptr<const internal::tree_base>
{
public:
	using iterator_category = std::bidirectional_iterator_tag;
	using value_type = const internal::tree_base;
	using difference_type = int;
	using pointer = const internal::tree_base *;
	using reference = const internal::tree_base &;

	string_const_iterator(const internal::tree_base *node = nullptr)
		: simple_ptr<const internal::tree_base>(node)
	{
	}

	string_const_iterator left()
	{
		return string_const_iterator((*this)->l);
	}
	string_const_iterator right()
	{
		return string_const_iterator((*this)->r);
	}
	string_const_iterator parent()
	{
		return string_const_iterator((*this)->p);
	}

	bool operator ==(const string_const_iterator &other)
	{
		return get() == other.get();
	}
	bool operator !=(const string_const_iterator &other)
	{
		return get() != other.get();
	}

	operator bool()
	{
		return *this != string_const_iterator();
	}

	string_const_iterator &operator ++()
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
	char operator *()
	{
		return static_cast<const internal::tree *>(get())->v;
	}
};

class string
{
	simple_ptr<internal::tree_head> head;
public:
	typedef string_const_iterator const_iterator;

	string() : head(new internal::tree_head())
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
		head->resurrect();
	}
	void push_back(char c)
	{
		head->l = head->root()->add_max(c);
		head->resurrect();
		//std::cout << "!" << head->l << " " << head->root()->c << std::endl;
	}
};

} // namespace iv

#endif // IV_STRING_H
