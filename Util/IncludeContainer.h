#ifndef __INCLUDE_CONTAINER_H_INCLUDED__
#define __INCLUDE_CONTAINER_H_INCLUDED__

/// Абстрактная база, делающая любой производный класс STL-совместимым контейнером.
/**
 *  Для хранения элеменов типа T использует STL контейнер C.
 */
template<class T, template<class, class> class C> class IncludeContainer/*{{{*/
{
public:
  typedef typename C<T, allocator<T> >::iterator iterator;
  typedef typename C<T, allocator<T> >::const_iterator const_iterator;
  typedef typename C<T, allocator<T> >::reverse_iterator reverse_iterator;
  typedef typename C<T, allocator<T> >::const_reverse_iterator const_reverse_iterator;
  
  T& at(std::size_t index) {
	  return included_container_.at (index);
  }
  iterator begin ()
  {
      return included_container_.begin ();
  }
  iterator end ()
  {
      return included_container_.end ();
  }
  const_iterator begin () const 
  {
      return included_container_.begin ();
  }
  const_iterator end () const 
  {
      return included_container_.end ();
  }
  reverse_iterator rbegin ()
  {
      return included_container_.rbegin ();
  }
  reverse_iterator rend ()
  {
      return included_container_.rend ();
  }
  const_reverse_iterator rbegin () const
  {
      return included_container_.rbegin ();
  }
  const_reverse_iterator rend () const
  {
      return included_container_.rend ();
  }
  bool empty () const
  {
      return included_container_.empty ();
  }
  size_t size () const
  {
      return included_container_.size ();
  }
  void clear ()
  {
      included_container_.clear ();
  }
  void push_back (const T& _value)
  {
      included_container_.push_back (_value);
  }
  void insert (iterator _it, const T& _value)
  {
      included_container_.insert (_it, _value);
  }
  void erase (iterator _it)
  {
      included_container_.erase (_it);
  }
  T& front ()
  {
      return included_container_.front ();
  }
  T& back ()
  {
      return included_container_.back ();
  }
  const T& front () const
  {
      return included_container_.front ();
  }
  const T& back () const
  {
      return included_container_.back ();
  }
private:
  C<T, allocator<T> > included_container_;
};/*}}}*/

#endif
