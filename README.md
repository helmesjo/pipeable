[![Linux & macOS](https://travis-ci.org/helmesjo/pipeable.svg?branch=master)](https://travis-ci.org/helmesjo/pipeable)
[![Windows](https://ci.appveyor.com/api/projects/status/f9vbeaxd8f2tq2hq?svg=true)](https://ci.appveyor.com/project/helmesjo/pipeable)

# pipeable
Just want to invoke some callable object, take the result and forward to another callable, and another, and another..?
A small library I needed that others might find useful.

> _I don't know functional programming, so I probably bring a pretty poor name-game._
> _Got better names at hand? I gladly accept feedback!_
### TL;DR:
```c++
#include <pipeable/pipeable.hpp>
// ...

// Bring "pipe" operator into scope
using pipeable::operator>>=;

struct extract_number
{
  int operator()(string input){ ... }
};

struct number_plus
{
  int operator()(int input){ ... }
};

int main(int argc, char *argv[])
{
  int specialNumber = "my nr 1 hat" >>= extract_number() >>= number_plus(5);
  cout << specialNumber; // output: 6
  
  return 0;
}
```

### Interceptors:
_A special callable capable of "intercepting" the invocation chain and inject custom logic._
_As first argument it will receive a callable representing the downstream pipeline (to be invoked by the interceptor)._

**for_each** example:
```c++
// Pass lambda to "make_interceptor" with first argument as generic. Rest is desired input
auto for_each = pipeable::assembly::make_interceptor(
  [](auto&& downstream, const vector& values){
    for(const int& value : values)
    {
      downstream(value); // will call print_to_stdout()
    }
  });

struct print_to_stdout
{
  void operator()(int val) { cout << val;1 }
};

vector{1, 2, 3} >>= for_each >>= print_to_stdout();
// output: 1 2 3
```
#### Built-in interceptors:
- **[for_each](https://github.com/helmesjo/pipeable/blob/bbe78f033b8b22779e4e371f8c18ef58e9ad7550/include/pipeable/pipeable.hpp#L9-L20)**: Iterate left-hand iterable and forward each individual value to downstream.
- **[visit](https://github.com/helmesjo/pipeable/blob/bbe78f033b8b22779e4e371f8c18ef58e9ad7550/include/pipeable/pipeable.hpp#L22-L27)**: Apply the visitor pattorn (std::visit) to left-hand std::variant<...> and invoke on downstream.
- **[maybe](https://github.com/helmesjo/pipeable/blob/bbe78f033b8b22779e4e371f8c18ef58e9ad7550/include/pipeable/pipeable.hpp#L29-L37)**: Forward left-hand std::optional<T> value to downstream if it exists, else do nothing.

## Build & Install
1. `mkdir build && cd build`
2. `cmake .. && cmake --build .`
    - Run tests: `ctest`
3. `cmake --build . --target install`
