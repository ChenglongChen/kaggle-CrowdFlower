/* * * * *
 *  AzMemTempl.hpp 
 *  Copyright (C) 2011-2014 Rie Johnson
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * * * * */

#ifndef _AZ_MEM_TEMPL_HPP_
#define _AZ_MEM_TEMPL_HPP_

#include "AzException.hpp"

#define myMIN(x,y) (((x) < (y)) ? (x) : (y))
#define myMAX(x,y) (((x) > (y)) ? (x) : (y))


/*-----  templates for memory handling  ---*/
/*--------------------------------------------------------------*/
/*             expandable  components                           */
/* AzBaseArray    Yes     anything that can be copied by "="   */
/* AzObjArray     Yes     objects; realloc uses "transfer_from */
/* AzObjPtrArray  Yes     ptr to object                        */
/*--------------------------------------------------------------*/

/*-----------------------------------------------------*/
template <class Int>
class AzMemTools {
protected:
  inline static void check_overflow(Int num, const char *eyec, const char *errmsg) {
    if (num < 0) {
      throw new AzException(eyec, errmsg, "check_overflow failed: likely to be integer overflow"); 
    }
  }
public:
  template <class T>
  static void alloc(T **ptr, Int num, const char *eyec="AzMemTools::alloc", 
                    const char *errmsg="") {
    *ptr = NULL; 
    check_overflow(num, eyec, errmsg); 
    if (num == 0) return; 

    try {
      *ptr = new T[num]; 
    }
    catch (std::bad_alloc &ba) {
      throw new AzException(AzAllocError, eyec, errmsg, ba.what(), num);       
    }
    if (*ptr == NULL) throw new AzException(AzAllocError, eyec, errmsg);
  }
  
  template <class T>
  static void free(T **ptr) {
    if (*ptr == NULL) return; 
    delete [] (*ptr); 
    *ptr = NULL; 
  }
  
  /*---  use this for base type or anything that can be copied by =  ---*/
  template <class T>
  static void realloc_base(T **ptr, Int old_num, Int new_num, 
  const char *eyec="AzMemTools::realloc_base", 
                           const char *errmsg="") {
    check_overflow(new_num, eyec, errmsg);        
    T *new_ptr = NULL;  
    if (new_num > 0) {
      try {
        new_ptr = new T[new_num]; 
      }
      catch (std::bad_alloc &ba) {
        throw new AzException(AzAllocError, eyec, errmsg, ba.what()); 
      }
      if (new_ptr == NULL) throw new AzException(AzAllocError, eyec, errmsg); 
      Int cpy_num = myMIN(old_num, new_num); 
      if (cpy_num > 0 && *ptr != NULL) {
        Int ix; 
        for (ix = 0; ix < cpy_num; ++ix) {
          *(new_ptr+ix) = *(*ptr+ix); 
        }
      }
    }
    delete [] *ptr;   
    *ptr = new_ptr;  
  }

  /*---  use this for the classes that cannot be copied by = ---*/
  /*---  "transfer_from" function must be defined  ---*/
  template <class T>
  static void realloc_obj(T **ptr, Int old_num, Int new_num, 
                          const char *eyec="AzMemTools::realloc_obj", 
                          const char *errmsg="") {
    check_overflow(new_num, eyec, errmsg); 
    T *new_ptr = NULL;  
    if (new_num > 0) {
      try {
        new_ptr = new T[new_num]; 
      }
      catch (std::bad_alloc &ba) {
        throw new AzException(AzAllocError, eyec, errmsg, ba.what()); 
      }
      if (new_ptr == NULL) throw new AzException(AzAllocError, eyec, errmsg); 
      Int cpy_num = myMIN(old_num, new_num); 
      if (cpy_num > 0 && *ptr != NULL) {        
        Int ix; 
        try {
          for (ix = 0; ix < cpy_num; ++ix) new_ptr[ix].transfer_from(&((*ptr)[ix])); 
        }
        catch (AzException *e) {
          delete [] new_ptr; 
          throw e; 
        }
      }
    }
    delete [] *ptr;   
    *ptr = new_ptr;  
  }
}; 

/*---------------------------------------------------------------------*/
/* For an array of pointers to objects                                 */
/* Faster realloc than array of objects since only pointers are copied */
/*---------------------------------------------------------------------*/
template <class Int>
class AzPMemTools {
  inline static void check_overflow(Int num, const char *eyec, const char *errmsg) {
    if (num < 0) {
      throw new AzException(eyec, errmsg, "check_overflow failed: likely to be integer overflow"); 
    }
  }
public:
  template<class T>
  static void alloc(T ***ptr, Int num, const char *eyec="AzPMemTools::alloc", const char *errmsg="") {
    *ptr = NULL; 
    if (num == 0) return; 
    check_overflow(num, eyec, errmsg); 
    try {
      *ptr = new T *[num]; 
    }
    catch (std::bad_alloc &ba) {
      throw new AzException(AzAllocError, eyec, errmsg, ba.what());       
    }
    if (*ptr == NULL) throw new AzException(AzAllocError, eyec, errmsg); 
    Int ix; 
    for (ix = 0; ix < num; ++ix) (*ptr)[ix] = NULL; 
  }

  template<class T>
  static void free(T ***ptr, Int num) {
    if (*ptr == NULL) return; 
    Int ix; 
    for (ix = 0; ix < num; ++ix) delete (*ptr)[ix]; 
    delete [] (*ptr); 
    *ptr = NULL; 
  }

  template<class T>
  static void realloc(T ***ptr, Int old_num, Int new_num, 
                      const char *eyec="AzPMemTools::realloc", 
                      const char *errmsg="") 
  {
    check_overflow(new_num, eyec, errmsg); 
    T **new_ptr = NULL;  
    if (new_num > 0) {
      try {
        new_ptr = new T *[new_num]; 
      }
      catch (std::bad_alloc &ba) {
        throw new AzException(AzAllocError, eyec, errmsg, ba.what()); 
      }
      if (new_ptr == NULL) throw new AzException(AzAllocError, eyec, errmsg); 
      Int cpy_num = myMIN(old_num, new_num); 
      Int ix; 
      for (ix = 0; ix < cpy_num; ++ix) { 
        *(new_ptr + ix) = *(*ptr + ix); 
        *(*ptr + ix) = NULL;     
      }
      for ( ; ix < new_num; ++ix) *(new_ptr + ix) = NULL; 
    }
    free(ptr, old_num); 
    *ptr = new_ptr;  
  }
}; 

/*---------------------------------------------------------------------*/
/* Array of pointers to objects                                        */
/* Faster than array of objects since realloc copies only the pointers */
/*---------------------------------------------------------------------*/
template<class T, class Int=int>
class AzObjPtrArray  /* expandable array of pointers to objects */
{
protected:
  T **a; 
  Int num; 

public:
  AzObjPtrArray() : a(NULL), num(0) {}
  AzObjPtrArray(Int inp_num, T ***p) : a(NULL), num(0) {
    alloc(p, inp_num); 
  }

  ~AzObjPtrArray() {
    AzPMemTools<Int>::free(&a, num); num = 0; 
  }
  void alloc(T ***p, Int inp_num, 
             const char *eyec="AzObjPtrArrary::alloc", const char *msg="") {
    if (p==NULL) err("null input", eyec, msg); 
    if (a != NULL || num != 0) {
      err("check failed", eyec, msg); 
    }
    num = inp_num; 
    if (num > 0) {
      AzPMemTools<Int>::alloc(&a, num, eyec, msg); 
    }
    *p = a; 
  }
  void realloc(T ***p, Int new_num, 
               const char *eyec="AzObjPtrArrary::realloc", const char *msg="") {
    if (p==NULL || *p!=a) err("sync-check failed", eyec, msg); 
    AzPMemTools<Int>::realloc(&a, num, new_num, eyec, msg); 
    num = new_num;
    *p = a; 
  }
  void free(T ***p, 
            const char *eyec="AzObjPtrArrary::free", const char *msg="") {
    if (p==NULL || *p!=a) {
      err("sync-check failed", eyec, msg); 
    }
    if (a != NULL) {
      AzPMemTools<Int>::free(&a, num); num = 0; 
      *p = a; 
    }
  }
  inline Int size() const { return num; }
  inline T **array() { return a; }
protected:
  void err(const char *s1, const char *s2, const char *s3) {
    throw new AzException(s1, s2, s3); 
  }
};

/*-----------------------------------------------------*/
template<class T, class Int=int>
class AzBaseArray  /* expandable array of base type that can be copied by memcpy */
{
protected:
  T *a; 
  Int num;
public:
  inline AzBaseArray(Int inp_num, T **p) : a(NULL), num(0) {
    alloc(p, inp_num); 
  }
  AzBaseArray() : a(NULL), num(0) {}
  ~AzBaseArray() {
    AzMemTools<Int>::free(&a); 
  }
  void alloc(T **p, Int inp_num, 
             const char *eyec="AzBaseArrary::alloc", const char *msg="") {
    if (p==NULL) err("null input", eyec, msg);
    if (a != NULL) {
      err("check failed", eyec, msg); 
    }
    num = inp_num; 
    if (num > 0) {
      AzMemTools<Int>::alloc(&a, num, eyec, msg); 
    }
    *p = a; 
  }
  void realloc(T **p, Int new_num, 
             const char *eyec="AzBaseArrary::realloc", const char *msg="") {
    if (p==NULL || *p!=a) err("sync-check failed", eyec, msg);
    AzMemTools<Int>::realloc_base(&a, num, new_num, eyec, msg); 
    num = new_num;
    *p = a; 
  }
  void transfer_from(AzBaseArray<T,Int> *inp, 
                     T **p, T **inp_p, 
                     const char *eyec="AzBaseArray::transfer_from", const char *msg="") 
  {
    if (p==NULL || *p!=a || inp_p==NULL || *inp_p!=inp->a) {
      err("sync-check failed", eyec, msg); 
    }
    AzMemTools<Int>::free(&a); num = 0; /* free this data */
    a = inp->a; inp->a = NULL;     /* transfer data from inp to this */
    num = inp->num; inp->num = 0;  
    *p = a;          /* synch ptr for this */
    *inp_p = inp->a; /* synch ptr for inp */
  }
  void free(T **p, 
            const char *eyec="AzBaseArray::free", const char *msg="") {
    if (p==NULL || *p!=a) err("sync-check failed", eyec, msg); 
    if (a != NULL) {
      AzMemTools<Int>::free(&a); num = 0; 
      *p = a; 
    }
  }
  inline Int size() const { return num; }
  inline T *array() { return a; }
protected:
  void err(const char *s1, const char *s2, const char *s3) {
    throw new AzException(s1, s2, s3); 
  }
};

/*-----------------------------------------------------*/
template<class T, class Int=int>
class AzObjArray  /* expandable array of class objects that cannot be copied by = */
{
public:
  AzObjArray() {
    a = NULL; 
    num = 0; 
  }
  ~AzObjArray() {
    AzMemTools<Int>::free(&a); 
  }
  void alloc(T **p, Int inp_num, 
             const char *eyec="AzObjArrary::alloc", const char *msg="alloc") {
    if (p==NULL) err("null input", eyec, msg);
    if (a != NULL) err("check failed", eyec, msg); 

    num = inp_num; 
    if (num > 0) {
      AzMemTools<Int>::alloc(&a, num, eyec, msg); 
    }
    *p = a; 
  }
  void realloc(T **p, Int new_num, 
               const char *eyec="AzObjArrary::realloc", const char *msg="alloc") {
    if (p==NULL || *p!=a) err("sync-check failed", eyec, msg);
    AzMemTools<Int>::realloc_obj(&a, num, new_num, eyec, msg); 
    num = new_num;
    *p = a; 
  }
  void transfer_from(AzObjArray<T> *inp, 
                     T **p, T **inp_p, 
                     const char *eyec="AzObjArray::transfer_from", const char *msg="") 
  {
    if (p==NULL || *p!=a || inp_p==NULL || *inp_p!=inp->a) {
      err("sync-check failed", eyec, msg); 
    }
    AzMemTools<Int>::free(&a); num = 0; /* free this data */
    a = inp->a; inp->a = NULL;     /* transfer data from inp to this */
    num = inp->num; inp->num = 0;  
    *p = a;          /* synch ptr for this */
    *inp_p = inp->a; /* synch ptr for inp */
  }
  void free(T **p, 
            const char *eyec="AzObjArray::free", const char *msg="") {
    if (p==NULL || *p!=a) err("sync-check failed", eyec, msg); 
    if (a != NULL) {
      AzMemTools<Int>::free(&a); num = 0; 
      *p = a; 
    }
  }
  inline Int size() const { return num; }
  inline T *array() { return a; }
protected:
  T *a; 
  Int num; 
  void err(const char *s1, const char *s2, const char *s3) {
    throw new AzException(s1, s2, s3); 
  }
};
#endif 
