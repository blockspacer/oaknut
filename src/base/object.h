//
// Copyright © 2018 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//

/**
 * @ingroup base_group
 * @class Object
 * @brief Base class for all reference-counted types.
 */
class Object {
public:
	int _refs;

	Object();
	virtual ~Object();
    
    /**
     Increments the internal reference counter. NB: Not threadsafe, cos we don't have threads.
     */
	void retain();
    
    /**
     Decrements the internal reference counter. If the counter reaches zero the object is moved
     to a queue of objects that will be free()d between frames. NB: Not threadsafe.
     */
	void release();
	
	void* operator new(size_t sz);
    
#ifdef DEBUG
    virtual string debugDescription();
#endif
    
    static void flushAutodeletePool();
    
    template<class ...Ts>
    static Object* createByName(const string& className, Ts...);

};

#if DEBUG
extern void* DBGOBJ;
#endif

/**
 * @ingroup base_group
 * @class sp<T>
 * @brief A smart pointer class that holds a strong reference to an Object-derived type.
 */
template<class T>
class sp {
public:
	T* _obj;
    
    template<class TC>
    TC* as() const {
        return static_cast<TC*>(_obj);
    }
/*#if DEBUG
    void dbgLog(bool retain) {
        if (_obj && _obj==DBGOBJ) {
            app.log("DBGOBJ:%s=%d", retain?"+1":"-1", _obj->_refs);
        }
    }
#else*/
#define dbgLog(x)
//#endif

    sp() : _obj(NULL) {
    }
    sp(T* obj) : _obj(obj) {
		if (obj) {
			obj->retain();
            dbgLog(true);
		}
    }
    sp(const sp<T>& sp) : _obj(sp._obj) { // Copy constructor
		if (_obj) {
			_obj->retain();
            dbgLog(true);
		}
    }
    ~sp() {
		if (_obj) {
			_obj->release();
            dbgLog(false);
		}
    }
    T& operator* () {
        return *(T*)_obj;
    }
    
    T* operator-> () const {
        return (T*)_obj;
    }
    T* operator& () const {
        return (T*)_obj;
    }
	void assign(T* obj) {
		if (obj != _obj) {
			if (_obj) {
				_obj->release();
                dbgLog(false);
			}
			_obj = obj;
			if (_obj) {
				_obj->retain();
                dbgLog(true);
			}
		}
	}
    T* operator= (T* obj) {
		assign(obj);
        return (T*)_obj;
    }
    sp<T>& operator= (const sp<T>& obj) {
		assign(obj._obj);
        return *this;
    }
    
    operator T*() const {
        return (T*)_obj;
    }
    bool isSet() {
        return _obj!=NULL;
    }
    
};

/**
 Declare a type as being dynamically creatable. The type must have a public constructor that takes no arguments.
 */

#define DECLARE_DYNCREATE2(X,NAME) static ClassRegistrar<X> s_classReg##X(NAME)
#define DECLARE_DYNCREATE(X) DECLARE_DYNCREATE2(X,#X)

extern std::map<string, Object* (*)()>* s_classRegister;

template<typename T>
class ClassRegistrar {
private: static Object* createT() {return new T(); }
public:
    ClassRegistrar(const string& className) {
        if (!s_classRegister) {
            s_classRegister = new std::map<string, Object*(*)()>();
        }
        s_classRegister->insert(std::make_pair(className, &createT));
    }
};


