//多线程安全的MAP类
#ifndef SAFEMAP_H
#define SAFEMAP_H
#include <boost/thread.hpp>
template<typename Key,typename Value>
class SafeMap
{
	typedef typename std::map<Key,Value>::iterator iterator;
	typedef std::pair<Key,Value> value_type;
	std::map<Key,Value> m_map;
	boost::mutex m_mut;
public:
	SafeMap(){}
	void insert(const value_type& v)
	{
		boost::lock_guard<boost::mutex> lock(m_mut);
		m_map.insert(v);
	}

	typename std::map<Key,Value>::size_type count(const Key& key)const
	{
		return m_map.count(key);
	}

	iterator find(const Key& key)
	{
		//boost::lock_guard<boost::mutex> lock(m_mut);
		return m_map.find(key);
	}

	Value& operator[](const Key& key)
	{
		return m_map[key];
	}
};

template<typename Key,typename Value>
void map_insert_safe(int threadId,std::map<Key,Value>& map,Value& v)
{
	static boost::mutex mut;
	boost::lock_guard<boost::mutex> lock(mut);	map.insert(std::make_pair(threadId,v));
}
#endif
