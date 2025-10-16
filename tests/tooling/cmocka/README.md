Cmocka Quick Start
==================

The main page links to the API documentation and a slideshow. Neither of these, in my quick perusal, actually give you an example of building a project with Cmocka. I really appreciate it when there's some kind of quickstart skeleton project you can just roll with, so I've taken their [first example from the API](https://api.cmocka.org/) and done just that.

The first thing you'll want to do to follow along here is download cmocka. I grabbed the [source release of 1.1.5](https://git.cryptomilk.org/projects/cmocka.git/).


```
sudo apt install cmake doxygen
tar -xzvf cmocka-1.1.8.tar.gz
cd cmocka-1.1.8
rm -rf build
mkdir build
cd build
cmake ..
make
sudo make install

```


Next, make yourself a `test.c` file with the following contents:

```
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>/* A test case that does nothing and succeeds. */
static void null_test_success(void **state) {
    (void) state; /* unused */
}
int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(null_test_success),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}

```

and build your test file:

```
# gcc -I cmocka-1.1.8/include/ -L cmocka-1.1.8/build/src/ test.c -lcmocka -o a.out
gcc -I /usr/local/include -L /usr/local/lib test.c -lcmocka -o a.out
```

This will have produced a `a.out` binary. Run it!

```
$ ./a.out
[==========] Running 1 test(s).
[ RUN      ] null_test_success
[       OK ] null_test_success
[==========] 1 test(s) run.
[  PASSED  ] 1 test(s).

```

And there you go. If you want to write unit tests for real C code, the only difference is you need to `#include` your headers for the source you're testing, and compile the source files you're testing along with the tests.