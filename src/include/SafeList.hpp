#pragma once

#include <vector>
#include <functional>

// using namespace std;

template <class T>
class SafeList
{
    vector<T> baseList;

public:
    explicit SafeList() = default;

    explicit SafeList(initializer_list<T> pInitializerList)
    {
        baseList = vector<T>(pInitializerList);
    }

    template <class Iterable>
    explicit SafeList(Iterable pIterable) : SafeList(pIterable.begin(), pIterable.end())
    {
    }

    template <class Iterator>
    explicit SafeList(Iterator pBegin, Iterator pEnd)
    {
        baseList = vector(pBegin, pEnd);
    }

    explicit SafeList(size_t pSz)
    {
        baseList = vector<T>(pSz);
    }

    ~SafeList()
    {
        for (size_t i = 0; i < count(); ++i)
        {
            removeAt(i);
        }
    }

    template <typename F>
    T find(function<F(T)> pSieve, F pTarget)
    {
        for (T element : baseList)
        {
            if (pSieve(element) == pTarget)
            {
                return element;
            }
        }

        return nullptr;
    }

    char* data()
    {
        return reinterpret_cast<char*>(baseList.data());
    }

    void insert(size_t pIndex, T pElement)
    {
        baseList.insert(begin() + pIndex, pElement);
    }

    void add(T pElement)
    {
        baseList.push_back(pElement);
    }

    size_t index(T pElement)
    {
        return std::ranges::find(baseList.begin(), baseList.end(), pElement);
    }

    void removeAt(size_t pIdx)
    {
        baseList.erase(baseList.begin() + pIdx);
    }

    void removeElement(T pElement)
    {
        removeAt(index(pElement));
    }

    size_t count()
    {
        return baseList.size();
    }

    T at(size_t pIdx)
    {
        return baseList[pIdx];
    }

    typename vector<T>::iterator begin()
    {
        return baseList.begin();
    }

    typename vector<T>::iterator end()
    {
        return baseList.end();
    }

    T operator[](size_t pIdx)
    {
        return at(pIdx);
    }
};
