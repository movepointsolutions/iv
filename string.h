#ifndef IV_STRING_H
#define IV_STRING_H

#include <algorithm>
#include <exception>
#include <iostream>
#include <iterator>
#include <memory>
#include <vector>

namespace iv
{

namespace internal
{

struct tree;

struct tree_base : std::enable_shared_from_this<tree_base>
{
	std::shared_ptr<tree> l, r; // left, right
	std::weak_ptr<tree_base> p; // parent

	tree_base(std::shared_ptr<tree> _l, std::shared_ptr<tree> _r);

	void resurrect();
};

struct tree_head : public tree_base
{
	tree_head(std::shared_ptr<tree> root = nullptr) : tree_base(root, NULL) { }
	std::shared_ptr<tree> root()
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

	tree(std::shared_ptr<tree> _l, char _v, std::shared_ptr<tree> _r)
		: tree_base(_l, _r), v(_v), h(std::max(l->height(), r->height()) + 1)
	{
	}

	std::shared_ptr<tree> add_min(char x);
	std::shared_ptr<tree> add_max(char x);
};

tree_base::tree_base(std::shared_ptr<tree> _l, std::shared_ptr<tree> _r)
	: l(_l), r(_r)
{
}

void tree_base::resurrect()
{
	//std::cerr << "Resurrecting " << this << std::endl;
	if (l) {
		l->p = shared_from_this();
		l->resurrect();
	}
	if (r) {
		r->p = shared_from_this();
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

std::shared_ptr<tree> balance(std::shared_ptr<tree> l, char v, std::shared_ptr<tree> r)
{
	if (l->height() > r->height() + 2) {
		if (l->l->height() >= l->r->height()) {
			auto right = std::make_shared<tree>(l->r, v, r);
			return std::make_shared<tree>(l->l, l->v, right);
		} else {
			auto left = std::make_shared<tree>(l->l, l->v, l->r->l);
			auto right = std::make_shared<tree>(l->r->r, v, r);
			return std::make_shared<tree>(left, l->r->v, right);
		}
	} else if (r->height() > l->height() + 2) {
		if (r->r->height() >= r->l->height()) {
			auto left = std::make_shared<tree>(l, v, r->l);
			return std::make_shared<tree>(left, r->v, r->r);
		} else {
			auto right = std::make_shared<tree>(r->l->r, l->v, r->r);
			auto left = std::make_shared<tree>(l, v, r->l->l);
			return std::make_shared<tree>(left, r->l->v, right);
		}
	} else
		return std::make_shared<tree>(l, v, r);
}

std::shared_ptr<tree> tree::add_min(char x)
{
	if (this == nullptr)
		return std::make_shared<tree>(nullptr, x, nullptr);
	else
		return balance(l->add_min(x), v, r);
}

std::shared_ptr<tree> tree::add_max(char x)
{
	if (this == nullptr)
		return std::make_shared<tree>(nullptr, x, nullptr);
	else
		return balance(l, v, r->add_max(x));
}

} // namespace iv::internal

class string_const_iterator : public std::weak_ptr<const internal::tree_base>
{
public:
	using iterator_category = std::bidirectional_iterator_tag;
	using value_type = const internal::tree_base;
	using difference_type = int;
	using pointer = const internal::tree_base *;
	using reference = const internal::tree_base &;

	string_const_iterator(std::weak_ptr<const internal::tree_base> node = std::weak_ptr<const internal::tree_base>())
		: std::weak_ptr<const internal::tree_base>(node)
	{
	}

	/*string_const_iterator(const std::shared_ptr<const internal::tree_base> &node)
		: std::weak_ptr<const internal::tree_base>(node)
	{
	}*/

	/*string_const_iterator(std::weak_ptr<const internal::tree_base> &&node = std::weak_ptr<const internal::tree_base>())
		: std::weak_ptr<const internal::tree_base>(node)
	{
	}*/

	string_const_iterator left()
	{
		auto p = lock();
		if (!p)
			throw std::runtime_error("expired iterator");
		return string_const_iterator(p->l);
	}
	string_const_iterator right()
	{
		auto p = lock();
		if (!p)
			throw std::runtime_error("expired iterator");
		return string_const_iterator(p->r);
	}
	string_const_iterator parent()
	{
		auto p = lock();
		if (!p)
			throw std::runtime_error("expired iterator");
		return string_const_iterator(p->p);
	}

	bool operator ==(const string_const_iterator &other)
	{
		return lock() == other.lock();
	}
	bool operator !=(const string_const_iterator &other)
	{
		return lock() != other.lock();
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
		return static_cast<const internal::tree *>(lock().get())->v;
	}
};

class string
{
	std::shared_ptr<internal::tree_head> head;
	std::vector<std::shared_ptr<internal::tree>> history;
public:
	typedef string_const_iterator const_iterator;

	string() : head(std::make_shared<internal::tree_head>())
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
		history.push_back(head->root());
		head->l = head->root()->add_min(c);
		head->resurrect();
	}
	void push_back(char c)
	{
		history.push_back(head->root());
		head->l = head->root()->add_max(c);
		head->resurrect();
		//std::cout << "!" << head->l << " " << head->root()->c << std::endl;
	}
};

} // namespace iv

#endif // IV_STRING_H
