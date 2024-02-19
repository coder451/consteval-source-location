// consteval-source-location.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <source_location>
#include <string_view>
#include <cstddef>
#include <format>
#include <vector>
#include <mutex>
#include <type_traits>
#include <concepts>

// Silly int wrapper for a special traceable
struct int_wrapper {
  int n{ 0 };
};

// Formatter for output using std::format etc
template <>
struct std::formatter<int_wrapper> {
  // for debugging only
  formatter() { std::cout << "formatter<int_wrapper>()\n"; }

  constexpr auto parse(std::format_parse_context& ctx) {
    return ctx.begin();
  }

  auto format(const int_wrapper& v, std::format_context& ctx) const {
    return std::format_to(ctx.out(), "{}", v.n);
  }
};


// Traceable concept
template <typename T> concept traceable = std::integral<T> || std::same_as<int_wrapper, T>;



static consteval int line(const std::source_location& loc = std::source_location::current()) {
  return loc.line();
}

static consteval const char* file_name(const std::source_location& loc = std::source_location::current()) {
  return loc.file_name();
}

void my_func(const char* f = file_name(), int l = line()) {
  std::cout << "my_func\n at " << f << ": " << l << std::endl;
}

void func2(const std::source_location& loc = std::source_location::current()) {
  std::cout << "func2 \n at " << loc.file_name() << ": " << loc.line() << std::endl;
}

template <size_t N, typename... Ts>
struct bbtrace_simple {
  bbtrace_simple(
    const char (&format_string)[N],
    Ts&&... ts,
    const std::source_location& loc =
    std::source_location::current()
  ) {
    std::cout << "At " << loc.file_name() << ":" << loc.line() << ":";
    std::cout << std::vformat(format_string, std::make_format_args(ts...));
    std::cout << std::endl;
  }
};

template <size_t N, typename... Ts>
bbtrace_simple(const char (&fs)[N], Ts&&...) -> bbtrace_simple<N, Ts...>;

struct bb;
struct blackbox_system {
  blackbox_system(size_t callsites_max):
    callsites_mutex{},
    callsites(callsites_max)
  {

  }

  void add_callsite(bb* callsite) {
    // Record the callsite
    // Note when the vector capacity had to be increased, because we try to avoid the blackbox system
    // allocating memory
    std::scoped_lock<std::mutex> lock(callsites_mutex);
    size_t c = callsites.capacity();
    callsites.push_back(callsite);
    if (callsites.capacity() > c) {
      auto result = std::format("INFO: Call site vector reallocated from {} to {}\n", c, callsites.capacity());
    }
  }
  // Counts all trace object instances, or equivalently, all trace call sites
  // The callsite vector is pre-allocated to avoid memory allocation during execution
  std::mutex callsites_mutex;
  std::vector<bb*> callsites;

};

struct bb {
  template<size_t N>
  bb(
    blackbox_system& sys_p,
    const char (&format_string_p)[N],
    const std::source_location& loc_p = std::source_location::current()
  ):
    sys(sys_p),
    loc(loc_p),
    format_string(format_string_p),
    call_count(0)
  {
    sys.add_callsite(this);
  }

  void trace(traceable auto ... ts) {
    // Compile-time check on whether the supplied types are compatible
    // with tracing


    // On the first call, record the types.
    // This is a bb instance variable, so there is one for each callsite.
    {
      std::scoped_lock lock(call_count_mutex);
      if (call_count == 0) {

      }
      ++call_count;
    }

    std::cout << "At " << loc.file_name() << ":" << loc.line() << ": ";
    std::cout << std::vformat(format_string, std::make_format_args(ts...));
    std::cout << std::endl;
  }

  blackbox_system& sys;
  const char *format_string;
  std::source_location loc;
  std::size_t call_count;
  std::mutex call_count_mutex;
};

/*
template <size_t N, typename... Ts>
struct X {
  X(
    const char (format_string_p)[N], 
    Ts... ts, 
    const std::source_location& loc_p = std::source_location::current()
  ) {

  }
};
template <size_t N, typename... Ts>
X(const char(&fs)[N], Ts&&...) -> X<N, Ts...>;
*/

struct untraceable {};

int main() {
  my_func();
  func2();
  bbtrace_simple("Write these A{} B{} C{}", 41, 42, 43);

  blackbox_system sys(100);

  {static bb* x = new bb(sys, "Write these D{} E{} F{}");
  x->trace(100, 101, 102);}

  {static bb* x = new bb(sys, "Write these G{} H{} I{}");
  x->trace(41, 42, 43);}

  /*
  untraceable me;
  {static bb* x = new bb(sys, "Write this error {}");
  x->trace(me); }
  */

  // This works because the output formatter has been extended
  // for int_wrapper.:wq

  int_wrapper n{ 0 };
  {static bb* x = new bb(sys, "Write this good boy {}");
  x->trace(n); }

}

