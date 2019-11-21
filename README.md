[![Cpp Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17) [![Linux & macOS](https://travis-ci.org/helmesjo/pipeable.svg?branch=master)](https://travis-ci.org/helmesjo/pipeable) [![Windows](https://ci.appveyor.com/api/projects/status/f9vbeaxd8f2tq2hq?svg=true)](https://ci.appveyor.com/project/helmesjo/pipeable)

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
  int operator()(string input);
};

struct number_plus
{
  number_plus(int val);
  int operator()(int input);
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
  void operator()(int val) { cout << val; }
};

vector{1, 2, 3} >>= for_each >>= print_to_stdout();
// output: 1 2 3
```
#### Built-in interceptors:
- **[for_each](https://github.com/helmesjo/pipeable/blob/bbe78f033b8b22779e4e371f8c18ef58e9ad7550/include/pipeable/pipeable.hpp#L9-L20)**: Iterate left-hand iterable and forward each individual value to downstream.
- **[visit](https://github.com/helmesjo/pipeable/blob/bbe78f033b8b22779e4e371f8c18ef58e9ad7550/include/pipeable/pipeable.hpp#L22-L27)**: Apply the visitor pattorn (std::visit) to left-hand std::variant<...> and invoke on downstream.
- **[unpack](https://github.com/helmesjo/pipeable/blob/cc76b0ff42b36bd9021b3afad8c1b3979c6cef25/include/pipeable/pipeable.hpp#L29-L34)**: Unpack left-hand tuple and pass elements as individual arguments to downstream.
- **[maybe](https://github.com/helmesjo/pipeable/blob/cc76b0ff42b36bd9021b3afad8c1b3979c6cef25/include/pipeable/pipeable.hpp#L36-L44)**: Forward left-hand optional value to downstream if it exists, else do nothing.
### Data Generator:
_A callable storing other callables to-be-invoked whenever new data is generated (observer pattern)._
```c++
#include <pipeable/data_generator.hpp>

struct print_to_stdout
{
  void operator()(int val) { cout << val; }
};

data_generator<int> myGenerator;
print_to_stdout receiver;
myGenerator += &receiver;   // Register receiver

myGenerator(1);             // output: 1
1 >>= myGenerator;          // output: 1

myGenerator -= &receiver;   // Deregister receiver
myGenerator(1);             // No output

```
### Data Source:
_An iterable type to be "pulled" for data until no more exists._
```c++
#include <pipeable/data_source.hpp>

struct int_source final : public data_source<int>
{
    std::optional<int> next() override
    {
        return current_ < 100 ? std::optional<int>{current_++} : std::nullopt;
    }
    int current_ = 0;
};

struct print_to_stdout
{
  void operator()(int val) { cout << val; }
};

int_source mySource;

mySource >>= for_each >>= print_to_stdout();
// output: 0 ... 99

```

# Build & Install
1. `mkdir build && cd build`
2. `cmake .. && cmake --build .`
    - Run tests: `ctest`
3. `cmake --build . --target install`
