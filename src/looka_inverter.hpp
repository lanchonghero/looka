#ifndef _LOOKA_INVERTER_HPP
#define _LOOKA_INVERTER_HPP
#include <stdint.h>
#include <unordered_map>
#include <vector>
#include <map>

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

#define USE_UNORDERED_MAP

template<typename KEY, typename ITEM>
class LookaInverter
{
public:
  LookaInverter();
  virtual ~LookaInverter();

  uint32_t Add(const KEY& key, const ITEM& item);
  uint32_t Add(const KEY& key, const std::vector<ITEM>& items);
  uint32_t GetKeys(std::vector<KEY> &keys);
  uint32_t GetItems(const KEY& key, std::vector<ITEM>* &items);

private:
  typedef typename std::vector<ITEM> InvertList_t;
#ifdef USE_UNORDERED_MAP
  typedef typename std::unordered_map<KEY, InvertList_t, typename KEY::Hash> IdxInvert_t;
  typedef typename std::unordered_map<KEY, InvertList_t, typename KEY::Hash>::iterator IdxInvertIt_t;
#else
  typedef typename std::map<KEY, InvertList_t> IdxInvert_t;
  typedef typename std::map<KEY, InvertList_t>::iterator IdxInvertIt_t;
#endif

  IdxInvert_t m_idxInvert;
};

template<typename KEY, typename ITEM>
LookaInverter<KEY, ITEM>::LookaInverter()
{
}

template<typename KEY, typename ITEM>
LookaInverter<KEY, ITEM>::~LookaInverter()
{
}

template<typename KEY, typename ITEM>
uint32_t LookaInverter<KEY, ITEM>::Add(const KEY& key, const ITEM& item)
{
  IdxInvertIt_t it = m_idxInvert.find(key);
  if (it == m_idxInvert.end())
    m_idxInvert.insert(std::make_pair(key, InvertList_t()));

  InvertList_t& ivtList = m_idxInvert[key];
  ivtList.push_back(item);
  return ivtList.size();
}

template<typename KEY, typename ITEM>
uint32_t LookaInverter<KEY, ITEM>::Add(const KEY& key, const std::vector<ITEM>& items)
{
  IdxInvertIt_t it = m_idxInvert.find(key);
  if (it == m_idxInvert.end())
    m_idxInvert.insert(std::make_pair(key, InvertList_t()));

  InvertList_t& ivtList = m_idxInvert[key];
  ivtList.insert(ivtList.end(), items.begin(), items.end()); 
  return ivtList.size();
}

template<typename KEY, typename ITEM>
uint32_t LookaInverter<KEY, ITEM>::GetKeys(std::vector<KEY> &keys)
{
  IdxInvertIt_t it = m_idxInvert.begin();
  for (; it != m_idxInvert.end(); ++it)
    keys.push_back(it->first);
  return keys.size();
}

template<typename KEY, typename ITEM>
uint32_t LookaInverter<KEY, ITEM>::GetItems(const KEY& key, std::vector<ITEM>* &items)
{
  uint32_t size = 0;
  IdxInvertIt_t it = m_idxInvert.find(key);
  if (it != m_idxInvert.end()) {
    items = &(it->second);
    size = items->size();
  }
  return size;
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
template<typename KEY, typename ITEM>
class LookaSimpleInverter
{
public:
  LookaSimpleInverter() {}
  virtual ~LookaSimpleInverter() {}

  uint32_t Add(const KEY& key, const ITEM& item);
  uint32_t Add(const KEY& key, const std::vector<ITEM>& items);
  uint32_t GetKeys(std::vector<KEY> &keys);
  uint32_t GetItems(const KEY& key, std::vector<ITEM>* &items);

private:
  typedef typename std::vector<ITEM> InvertList_t;
  typedef typename std::map<KEY, InvertList_t> IdxInvert_t;
  typedef typename std::map<KEY, InvertList_t>::iterator IdxInvertIt_t;

  IdxInvert_t m_idxInvert;
};

template<typename KEY, typename ITEM>
uint32_t LookaSimpleInverter<KEY, ITEM>::Add(const KEY& key, const ITEM& item)
{
  IdxInvertIt_t it = m_idxInvert.find(key);
  if (it == m_idxInvert.end())
    m_idxInvert.insert(std::make_pair(key, InvertList_t()));

  InvertList_t& ivtList = m_idxInvert[key];
  ivtList.push_back(item);
  return ivtList.size();
}

template<typename KEY, typename ITEM>
uint32_t LookaSimpleInverter<KEY, ITEM>::Add(const KEY& key, const std::vector<ITEM>& items)
{
  IdxInvertIt_t it = m_idxInvert.find(key);
  if (it == m_idxInvert.end())
    m_idxInvert.insert(std::make_pair(key, InvertList_t()));

  InvertList_t& ivtList = m_idxInvert[key];
  ivtList.insert(ivtList.end(), items.begin(), items.end()); 
  return ivtList.size();
}

template<typename KEY, typename ITEM>
uint32_t LookaSimpleInverter<KEY, ITEM>::GetKeys(std::vector<KEY> &keys)
{
  IdxInvertIt_t it = m_idxInvert.begin();
  for (; it != m_idxInvert.end(); ++it)
    keys.push_back(it->first);
  return keys.size();
}

template<typename KEY, typename ITEM>
uint32_t LookaSimpleInverter<KEY, ITEM>::GetItems(const KEY& key, std::vector<ITEM>* &items)
{
  uint32_t size = 0;
  IdxInvertIt_t it = m_idxInvert.find(key);
  if (it != m_idxInvert.end()) {
    items = &(it->second);
    size = items->size();
  }
  return size;
}

#endif //_LOOKA_INVERTER_HPP
