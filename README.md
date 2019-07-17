# CoCoCouroutines

This is my attempt to implement high level assymetric coroutines mechanism written in gnu89 compatible C.

In my work I often encounter complex asynchronous code written in C. Usually, this code implements very complex product logic. Such code normally looks non-continuos with lots of jumps between callbacks, lots of technical details such as where I store the data between jumps, where I return to and so on. As a result, code becomes hardly readable or maintainable.

Assymetric coroutines and coroutines in general are nothing new as a concept. The idea of having suspendable routine that can be resumed from where it stopped, existed almost since the beginning of the age of computing. The great thing about it is that ugly jumpy code becomes continuos. So instead of this:

    function foo() {
       do stuff
       save data somewhere
       async_operation(foo_callback)
    }
    
    function foo_callback(promise) {
       get return value of async_operation from promise
       retrieved data that foo saved
       continue doing stuff
    }

Whe have this:

    coroutine foo() {
       do stuff
       yield async_operation
       continue doing stuff
    }
    
Obviously the second peace of code is much easier to understand. Now you can **actually see the flow** and not all the related junk. There are other advantages of coroutines, one of them is performance boost in many cases, but I will not be dealing with it now. The reason I started CoCo is one and only: **code clarity**.

Today many high level languages implement it in one way or another some sort of coroutines. Regretfully (or not), C, with its minimalistic set of features, is not one of them. There are some great libraries for C that do this kind of suff. I am not attempting to surpass all possible solution, rather concentrait on specific golden set of traits:

1. User's code clarity above all else
2. Portability
   - No CPU specific code (if possible), rather pure C.
   - All platform specific code is encapsulated in minimal interface, that can be implement on new platforms to extend support.
   - Compatible with gnu89 standard (which is used in Linux kernel), i.e. can be used in Kernel.
3. Schedule co-rotines from regular functions. There is no intent to force special `co_routine_main` function to spaw coroutines. If user wan't to schedule a new coroutine, it shall be possible from ANY context.
4. Performance is important, BUT not if it comes in expense of the other traits.

## Status

Currently in active development, not even close to any stable release.

## Usage

Currently, the aim is to make it headers only library. Might change.

```C
#include "co_coroutines.h" /*Primary include*/

co_routine_decl(int, fibonacci, int, x, int, y); /*Declaration*/
co_routine_decl(/*void*/, hello, struct fibonacci_co_obj*, child);

co_yield_rv_t fibonacci(struct fibonacci_co_obj *self) {
    co_routine_begin(self, fibonacci);
    
    /*Simlple infinite fibonacci series generator*/
    
    while (1) {
        int z = _(x) + _(y);
        _(x) = _(y);
        _(y) = z;
        co_yield_return(self, z); /*Return intermediate result*/
    }
   
    co_yield_break();
}

co_yield_rv_t hello(struct hello_co_obj *self) {
    co_routine_begin(self, hello);
    
    printf("I am going to print fibonacci series, just wait 1 second...\n");
    
    co_yield_wait_timeout(self, 1000); /*While waiting, others may run*/
    
    _(child) = co_dork_run(self, _(child), 1, 1); /*Start child coroutine*/
    
    if_co_yield_await(self, _(child)) {
        printf("I've got new number: %d\n", _(child)->rv);
        if (_(child)->rv > 1000) {
            printf("OK, that's big enough for me\n");
            co_yield_break();
        }
        co_yield_await_next(self, _(child)); /*Continue to the next fibonacci number*/
    }
    
   
    co_yield_break();
}
```
