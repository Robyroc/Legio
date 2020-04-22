#ifndef STRUCTURE_HANDLER_H
#define STRUCTURE_HANDLER_H

#include <functional>
#include <unordered_map>

template <typename T, typename U>
class StructureHandler
{
    public:
        StructureHandler(std::function<int(T,int*)>, std::function<int(T,int*, int*)>, std::function<int(T*)>);
        void add(T, std::function<int(U, T*)>);
        T translate(T);
        void remove(T);
        void part_of(T, int*);
        void replace(U);
    private:
        std::function<int(T,int*)> attribute_set;
        std::function<int(T,int*, int*)> attribute_get;
        std::function<int(T*)> destroyer;
        std::unordered_map<int, std::pair<std::function<int(U, T*)>, T>> opened;
        int counter;
};

template<typename T, typename U>
StructureHandler<T, U>::StructureHandler(
        std::function<int(T,int*)> setter,
        std::function<int(T,int*, int*)> getter,
        std::function<int(T*)> killer):
    attribute_set(setter),
    attribute_get(getter),
    destroyer(killer) 
{}

template<typename T, typename U>
void StructureHandler<T, U>::add(T added, std::function<int(U, T*)> constr)
{
    int id = counter++;
    std::pair<std::function<int(U, T*)>, T> entrying(constr, added);
    std::pair<int, std::pair<std::function<int(U, T*)>, T>> inserting (id, entrying);
    opened.insert(inserting);

    typename std::unordered_map<int, std::pair<std::function<int(U, T*)>, T>>::iterator res = opened.find(id);
    attribute_set(added, (int *) &(res->first));
    attribute_set(res->second.second, (int *) &(res->first));
}

template <typename T, typename U>
T StructureHandler<T, U>::translate(T input)
{
    int* value;
    int flag;
    attribute_get(input, (int*) &value, &flag);
    if(flag)
    {
        typename std::unordered_map<int, std::pair<std::function<int(U, T*)>, T>>::iterator res = opened.find(*value);
        if(res == opened.end())
            return input;
        else return res->second.second;
    }
    else return input;
}

template <typename T, typename U>
void StructureHandler<T, U>::remove(T item)
{
    int* key;
    int flag;
    attribute_get(item, (int*) &key, &flag);
    if(flag)
    {
        typename std::unordered_map<int, std::pair<std::function<int(U, T*)>, T>>::iterator res = opened.find(*key);
        if(res != opened.end())
            destroyer(&(res->second.second));
        opened.erase(*key);
    }
}

template <typename T, typename U>
void StructureHandler<T, U>::replace(U new_upper)
{
    for(typename std::unordered_map<int, std::pair<std::function<int(U, T*)>, T>>::iterator it = opened.begin(); it != opened.end(); it++)
        destroyer(&(it->second.second));
    for(typename std::unordered_map<int, std::pair<std::function<int(U, T*)>, T>>::iterator it = opened.begin(); it != opened.end(); it++)
    {
        it->second.first(new_upper, &(it->second.second));
        attribute_set(it->second.second, (int *) &(it->first));
    }
}

template <typename T, typename U>
void StructureHandler<T, U>::part_of(T checked, int* result)
{
    int* value;
    attribute_get(checked, (int*) &value, result);
}

#endif