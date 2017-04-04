/*
* File:   tlos.hpp
* Author: Barath Kannan
* Implementation of thread-local variables with object-scoped access.
* Created on 13 February 2017 7:46 PM
*/

#ifndef BK_CONQ_TLOS_HPP
#define BK_CONQ_TLOS_HPP

#include <mutex>
#include <vector>
#include <set>
#include <thread>

namespace bk_conq{
namespace details{
template <typename T, typename OWNER = void>
class tlos {
public:
    tlos(std::function<T()> defaultvalfunc = nullptr, std::function<void(T&&)> returnfunc = nullptr) :
        _defaultvalfunc(defaultvalfunc),
        _returnfunc(returnfunc),
        _myid(_id++),
        _myindx(get_index(_myid))
    {}

    static size_t get_index(size_t myid) {
        std::lock_guard<std::mutex> lock(_m);
        size_t myindx;
        //check if a previous owner returned their index
        if (!_available.empty()) {
            //use that index if available
            myindx = _available.back();
            _owners[myindx] = myid;
            _available.pop_back();
        }
        else {
            //otherwise generate a new one
            myindx = _owners.size();
            _owners.push_back(myid);
        }
        return myindx;
    }

    virtual ~tlos() {
        std::lock_guard<std::mutex> lock(_m);
        //invoke the returner for all thread local boxes corresponding to this index
        for (returner* ret : _thread_locals) {
            std::vector<box>& vec = (*ret)();
            //if the size is not greater than the index, the thread-local object was never accessed
            //and hence it was never initialized
            if (_myindx < vec.size()) {
                auto& box_instance = vec.at(_myindx);
                //check if we've used that box
                if (box_instance.owner_id == _myid) {
                    if (box_instance.returnfunc) box_instance.returnfunc(std::move(box_instance.value));
                    //mark as unused
                    box_instance.owner_id = 0;
                }
            }
        }
        //relinquish the index and mark it as available for others
        _available.push_back(_myindx);
        //mark the index as unused
        _owners[_myindx] = 0;
        //if this is the last tlos instance of this type, clear some of the static space
        if (_available.size() == _owners.size()) {
            _available.clear();
            _owners.clear();
        }
    }

    //Returns a reference to the thread local variable corresponding to the tlos object
    T& get() {
        auto& vec = get_returner();
        //resize if accessed by a class with a higher counter
        if (vec.size() <= _myindx) {
            std::lock_guard<std::mutex> lock(_m);
            //check again now that we're sync'd
            if (vec.size() <= _myindx) {
                vec.resize(_myindx + 1);
            }
        }
        //retrieve the item owned by this object instance
        auto& ret = vec[_myindx];
        //if the owner is not set, need to apply the default value
        if (!ret.owner_id) {
            //if a default value function has been provided, use that to assign the initial value
            if (_defaultvalfunc) ret.value = _defaultvalfunc();
            //reconfigure the id and returner
            //the returnerfunc is only invoked if the thread is joined and the thread_local vector has its dtor invoked
            //it is not necessary to invoke it here as the previous owner has already been destroyed
            ret.owner_id = _myid;
            ret.returnfunc = _returnfunc;
        }
        return ret.value;
    }

    //manually invokes the returning function for the calling thread
    //return value indicates that the returner was successfully invoked
    bool relinquish() {
        std::lock_guard<std::mutex> lock(_m);
        auto& vec = get_returner();
        //if the index is not valid yet, the object was never assigned from this thread in the first place
        if (vec.size() <= _myindx) return false;
        auto& ret = vec[_myindx];
        //we only want to invoke the returner if we are the owner
        if (ret.owner_id != _myid) return false;
        //check if it has been initialized
        if (!ret.returnfunc) return false;
        ret.returnfunc(std::move(ret.value));
        //reset the owner id so if get() is ever called after this point, it will call the defaultvalfunc again
        ret.owner_id = 0;
        return true;
    }

private:
    struct box {
        size_t owner_id;
        T value;
        std::function<void(T&&)> returnfunc{ nullptr };
    };

    class returner {
    private:
        std::vector<box> _v;
    public:
        returner() {
            std::lock_guard<std::mutex> lock(_m);
            _thread_locals.insert(this);
        }

        std::vector<box>& get() {
            return _v;
        }

        std::vector<box>& operator()() {
            return get();
        }

        //This thread local object's dtor invokes the returnfunc for all thread-local items
        //whose owning object has not already gone out of scope and who have defined a _returnfunc.
        //It also removes the returner from the tracked set of thread local returners
        ~returner() {
            std::lock_guard<std::mutex> lock(_m);
            //owners size will only be less if it was flushed by a tlos dtor
            for (size_t i = 0; i < _v.size() && i < _owners.size(); ++i) {
                auto& current_id = _v.at(i);
                //check if a returnfunc has been defined, and
                //check if the object who owns the item is still valid
                if (current_id.returnfunc && current_id.owner_id && _owners[i] == current_id.owner_id) {
                    current_id.returnfunc(std::move(current_id.value));
                }
            }
            _thread_locals.erase(this);
        }
    };

    std::vector<box>& get_returner() {
        //the vector of thread_local variables
        thread_local returner vec;
        return vec();
    }

    //defines a function to assign the initial value to the retrieved value, when it is first retrieved in a unique thread in a unique object
    const std::function<T()> _defaultvalfunc{ nullptr };

    //defines a function to notify the owning object that a thread owning a thread-local instance has gone out of scope
    const std::function<void(T&&)> _returnfunc{ nullptr };

    //identifies the index of this objects U item in the thread
    const size_t _myindx;

    //uniquely identifies this object (prefer using this to pointer as another object of the same type can be given the same address)
    const size_t _myid;

    //counter for assigning the objects unique id, starting at 1
    static std::atomic<size_t> _id;

    //vector of released box indexes
    static std::vector<size_t> _available;

    //maps box indexes to their owners id
    static std::vector<size_t> _owners;

    //provides global access to the thread local vectors
    static std::set<returner*> _thread_locals;

    static std::mutex _m;
};

template <typename T, typename OWNER>
std::atomic<size_t> tlos<T, OWNER>::_id = 1;

template <typename T, typename OWNER>
std::vector<size_t> tlos<T, OWNER>::_available;

template <typename T, typename OWNER>
std::vector<size_t> tlos<T, OWNER>::_owners;

template <typename T, typename OWNER>
std::set<typename tlos<T, OWNER>::returner*> tlos<T, OWNER>::_thread_locals;

template <typename T, typename OWNER>
std::mutex tlos<T, OWNER>::_m;

}//namespace details
}//namespace bk_conq

#endif // BK_CONQ_TLOS_HPP

