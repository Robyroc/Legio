#ifndef STRUCTURE_HANDLER_H
#define STRUCTURE_HANDLER_H

#include <functional>
#include <unordered_map>

template <typename T, typename U>
class StructureHandler
{
    public:
        StructureHandler(std::function<int(T,int*)>, std::function<int(T,int*, int*)>, std::function<int(T*)>, std::function<int(T, T*)>, int);
        void add_general(T, std::function<int(U, T*)>);
        void add(int, T, std::function<int(U, T*)>);
        T translate(T);
        void remove(T);
        void part_of(T, int*);
        void replace(U);
    private:
        std::function<int(T,int*)> attribute_set;
        std::function<int(T,int*, int*)> attribute_get;
        std::function<int(T*)> destroyer;
        std::function<int(T, T*)> adapt;
        std::unordered_map<int, std::pair<std::function<int(U, T*)>, T>> opened;
        int counter;
        int fake_flag;
};

template<typename T, typename U>
StructureHandler<T, U>::StructureHandler(
        std::function<int(T,int*)> setter,
        std::function<int(T,int*, int*)> getter,
        std::function<int(T*)> killer,
        std::function<int(T, T*)> adapter,
        int flag):
    attribute_set(setter),
    attribute_get(getter),
    destroyer(killer), 
    adapt(adapter),
    fake_flag(flag)
{}

template<typename T, typename U>
void StructureHandler<T, U>::add(int id, T added, std::function<int(U, T*)> constr)
{
    std::pair<std::function<int(U, T*)>, T> entrying(constr, added);
    std::pair<int, std::pair<std::function<int(U, T*)>, T>> inserting (id, entrying);
    opened.insert(inserting);

    typename std::unordered_map<int, std::pair<std::function<int(U, T*)>, T>>::iterator res = opened.find(id);
    attribute_set(added, (int *) &(res->first));
    attribute_set(res->second.second, (int *) &(res->first));
}

template<typename T, typename U>
void StructureHandler<T, U>::add_general(T added, std::function<int(U, T*)> constr)
{
    add(counter++, added, constr);
}

template <typename T, typename U>
T StructureHandler<T, U>::translate(T input)
{
    int value;
    int* pointer = &value;
    int flag = 0;
    attribute_get(input, (int*) &pointer, &flag);
    if(flag || fake_flag)
    {
        typename std::unordered_map<int, std::pair<std::function<int(U, T*)>, T>>::iterator res = opened.find(*pointer);
        if(res == opened.end())
        {
            printf("CANNOT TRANSLATE SOMETHING...\n");
            return input;
        }
        else return res->second.second;
    }
    else return input;
}

template <typename T, typename U>
void StructureHandler<T, U>::remove(T item)
{

    int key;
    int* pointer = &key;
    int flag;
    attribute_get(item, (int*) &pointer, &flag);
    if(flag || fake_flag)
    {
        typename std::unordered_map<int, std::pair<std::function<int(U, T*)>, T>>::iterator res = opened.find(*pointer);
        if(res != opened.end())
            destroyer(&(res->second.second));
        else
            printf("REMOVING SOMETHING NOT PRESENT...\n");
        opened.erase(*pointer);
    }
}

template <typename T, typename U>
void StructureHandler<T, U>::replace(U new_upper)
{
    T temp;
    for(typename std::unordered_map<int, std::pair<std::function<int(U, T*)>, T>>::iterator it = opened.begin(); it != opened.end(); it++)
    {
        temp = it->second.second;
        destroyer(&(it->second.second));
        it->second.first(new_upper, &(it->second.second));
        attribute_set(it->second.second, (int *) &(it->first));
        adapt(temp, &(it->second.second));
    }
}

template <typename T, typename U>
void StructureHandler<T, U>::part_of(T checked, int* result)
{
    int value;
    int* pointer = &value;
    if(!fake_flag)
        attribute_get(checked, (int*) &pointer, result);
    else
    {
        int flag;
        attribute_get(checked, (int*) &pointer, &flag);
        typename std::unordered_map<int, std::pair<std::function<int(U, T*)>, T>>::iterator res = opened.find(*pointer);
        *result = (res != opened.end());
    }
}

#endif