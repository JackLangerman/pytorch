// define constants like M_PI and C keywords for MSVC
#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#include <math.h>
#endif

#include "ATen/ATen.h"
#include "ATen/CPUGenerator.h"
#include "ATen/CheckGenerator.h"
#include "ATen/Dispatch.h"
#include "ATen/Error.h"
#include "ATen/NativeFunctions.h"
#include "ATen/ScalarType.h"
#include "ATen/Deprecated.h"
#include "TH/THRandom.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace at {
namespace native {
namespace {
void window_function_checks(
    const char* function_name,
    const TensorOptions& options,
    int64_t window_length) {
  AT_CHECK(
      options.layout() != kSparse,
      function_name,
      " is not implemented for sparse types, got: ",
      options.type().toString());
  AT_CHECK(
      at::isFloatingType(options.dtype()),
      function_name,
      " expects floating point dtypes, got: ",
      options.type().toString());
  AT_CHECK(
      window_length > 0,
      function_name,
      " requires positive window_length, got window_length=%lld",
      window_length);
}
} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ arange ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tensor arange(Scalar start, Scalar end, const TensorOptions& options) {
  return native::arange(start, end, /*step=*/1, options);
}

Tensor arange(
    Scalar start,
    Scalar end,
    Scalar step,
    const TensorOptions& options) {
  return options.type()._arange(start, end, step);
}

Tensor& arange_out(Tensor& result, Scalar start, Scalar end) {
  return native::arange_out(result, start, end, /*step=*/1);
}

Tensor& arange_out(Tensor& result, Scalar start, Scalar end, Scalar step) {
  return at::_arange_out(result, start, end, step);
}

Tensor arange(Scalar end, const TensorOptions& options) {
  return options.type()._arange(end);
}

Tensor& arange_out(Tensor& result, Scalar end) {
  return at::_arange_out(result, end);
}

Tensor _dim_arange(const Tensor& like, int64_t dim) {
  return like.type().toScalarType(at::kLong)._arange(like.size(dim));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ empty ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tensor empty(IntList size, const TensorOptions& options) {
  return options.type().tensor(size);
}

Tensor& empty_out(Tensor& result, IntList size) {
  if (result.is_sparse()) {
    result.sparse_raw_resize_(size, size.size(), 0);
  } else {
    result.resize_(size);
  }
  return result;
}

// Temporary type cast operators. These are needed to trace type-casts now since
// Type's are not supported in the IR. Instead, we call down to these
// specialized operators for each datatype.
// TODO: remove when we have Type support in the IR

#define DEFINE_CAST_OP(_1, n, _2)                                \
  Tensor _cast_##n(const Tensor& self, bool non_blocking) {      \
    auto& target_type = self.type().toScalarType(ScalarType::n); \
    if (self.type() == target_type)                              \
      return self;                                               \
    return target_type.copy(self, non_blocking);                 \
  }

AT_FORALL_SCALAR_TYPES(DEFINE_CAST_OP)

#undef DEFINE_CAST_OP

Tensor empty_like(const Tensor& self) {
  return native::empty_like(self, self.options());
}

Tensor empty_like(const Tensor& self, const TensorOptions& options) {
  if (options.layout() == kSparse && self.type().is_sparse()) {
    auto res = options.type().tensor({});
    // resize_as_ requires the same exact type.
    res.sparse_raw_resize_(self.sizes(), self._sparseDims(), self._denseDims());

    return res;
  }
  return native::empty(self.sizes(), options);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ eye ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tensor eye(int64_t n, const TensorOptions& options) {
  return native::eye(n, -1, options);
}

Tensor eye(int64_t n, int64_t m, const TensorOptions& options) {
  auto tensor = options.type().tensor({});
  return at::eye_out(tensor, n, m);
}

Tensor& eye_out_cpu(Tensor& result, int64_t n) {
  return native::eye_out_cpu(result, n, -1);
}

Tensor& eye_out_cpu(Tensor& result, int64_t n, int64_t m) {
  AT_CHECK(n > 0, "n must be greater than 0, got", n);
  if(m <= 0) {
    m = n;
  }

  result.resize_({n, m});
  result.zero_();

  int64_t sz = std::min<int64_t>(n, m);
  AT_DISPATCH_ALL_TYPES(result.type(), "eye", [&]() -> void {
    scalar_t* result_data = result.data<scalar_t>();
    for(int64_t i = 0; i < sz; i++) {
      result_data[i*(result.strides()[0] + result.strides()[1])] = 1;
    }
  });

  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ full ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tensor full(IntList size, Scalar fill_value, const TensorOptions& options) {
  if (options.layout() == kSparse) {
    AT_ERROR("full(...) is not implemented for sparse layout");
  }
  auto result = options.type().tensor(size);
  return result.fill_(fill_value);
}

Tensor& full_out(Tensor& result, IntList size, Scalar fill_value) {
  if (result.is_sparse()) {
    AT_ERROR("full(...) is not implemented for sparse layout");
  }
  result.resize_(size);
  return result.fill_(fill_value);
}

Tensor full_like(const Tensor& self, Scalar fill_value) {
  return native::full_like(self, fill_value, self.options());
}

Tensor full_like(const Tensor& self, Scalar fill_value, const TensorOptions& options) {
  return native::full(self.sizes(), fill_value, options);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ linspace ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tensor linspace(Scalar start, Scalar end, const TensorOptions& options) {
  return native::linspace(start, end, /*steps=*/100, options);
}

Tensor linspace(
    Scalar start,
    Scalar end,
    int64_t steps,
    const TensorOptions& options) {
  return options.type()._linspace(start, end, steps);
}

Tensor& linspace_out(Tensor& result, Scalar start, Scalar end) {
  return native::linspace_out(result, start, end, /*steps=*/100);
}

Tensor& linspace_out(Tensor& result, Scalar start, Scalar end, int64_t steps) {
  return at::_linspace_out(result, start, end, steps);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ logspace ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tensor logspace(Scalar start, Scalar end, const TensorOptions& options) {
  return native::logspace(start, end, /*steps=*/100, options);
}

Tensor logspace(
    Scalar start,
    Scalar end,
    int64_t steps,
    const TensorOptions& options) {
  return options.type()._logspace(start, end, steps);
}

Tensor& logspace_out(Tensor& result, Scalar start, Scalar end) {
  return native::logspace_out(result, start, end, /*steps=*/100);
}

Tensor& logspace_out(Tensor& result, Scalar start, Scalar end, int64_t steps) {
  return at::_logspace_out(result, start, end, steps);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ones ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tensor ones(IntList size, const TensorOptions& options) {
  return native::full(size, /*fill_value=*/1, options);
}

Tensor& ones_out(Tensor& result, IntList size) {
  return native::full_out(result, size, /*fill_value=*/1);
}

Tensor ones_like(const Tensor& self) {
  return native::ones(self.sizes(), self.options());
}

Tensor ones_like(const Tensor& self, const TensorOptions& options) {
  return native::ones(self.sizes(), options);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ rand ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tensor rand(IntList size, const TensorOptions& options) {
  return native::rand(size, nullptr, options);
}

Tensor rand(IntList size, Generator* generator, const TensorOptions& options) {
  auto result = options.type().tensor(size);
  return result.uniform_(0, 1, generator);
}

Tensor& rand_out(Tensor& result, IntList size) {
  return native::rand_out(result, size, nullptr);
}

Tensor& rand_out(Tensor& result, IntList size, Generator* generator) {
  result.resize_(size);
  return result.uniform_(0, 1, generator);
}

Tensor rand_like(const Tensor& self) {
  return native::rand_like(self, self.options());
}

Tensor rand_like(const Tensor& self, const TensorOptions& options) {
  return native::rand(self.sizes(), options);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ randint ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tensor randint(int64_t high, IntList size, const TensorOptions& options) {
  return native::randint(high, size, nullptr, options);
}

Tensor randint(
    int64_t high,
    IntList size,
    Generator* generator,
    const TensorOptions& options) {
  return native::randint(0, high, size, generator, options);
}

Tensor randint(
    int64_t low,
    int64_t high,
    IntList size,
    const TensorOptions& options) {
  return native::randint(low, high, size, nullptr, options);
}

Tensor randint(
    int64_t low,
    int64_t high,
    IntList size,
    Generator* generator,
    const TensorOptions& options) {
  auto result = options.type().tensor(size);
  return result.random_(low, high, generator);
}

Tensor& randint_out(Tensor& result, int64_t high, IntList size) {
  return native::randint_out(result, high, size, nullptr);
}

Tensor& randint_out(
    Tensor& result,
    int64_t high,
    IntList size,
    Generator* generator) {
  result.resize_(size);
  return result.random_(0, high, generator);
}

Tensor& randint_out(Tensor& result, int64_t low, int64_t high, IntList size) {
  return native::randint_out(result, low, high, size, nullptr);
}

Tensor& randint_out(
    Tensor& result,
    int64_t low,
    int64_t high,
    IntList size,
    Generator* generator) {
  result.resize_(size);
  return result.random_(low, high, generator);
}

Tensor randint_like(const Tensor& self, int64_t high) {
  return native::randint_like(self, high, self.options());
}

Tensor randint_like(const Tensor& self, int64_t low, int64_t high) {
  return native::randint_like(self, low, high, self.options());
}

Tensor randint_like(
    const Tensor& self,
    int64_t high,
    const TensorOptions& options) {
  return native::randint(high, self.sizes(), nullptr, options);
}

Tensor randint_like(
    const Tensor& self,
    int64_t low,
    int64_t high,
    const TensorOptions& options) {
  return native::randint(low, high, self.sizes(), nullptr, options);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ randn ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tensor randn(IntList size, const TensorOptions& options) {
  return native::randn(size, nullptr, options);
}

Tensor randn(IntList size, Generator* generator, const TensorOptions& options) {
  auto result = options.type().tensor(size);
  return result.normal_(0, 1, generator);
}

Tensor& randn_out(Tensor& result, IntList size) {
  return native::randn_out(result, size, nullptr);
}

Tensor& randn_out(Tensor& result, IntList size, Generator* generator) {
  result.resize_(size);
  return result.normal_(0, 1, generator);
}

Tensor randn_like(const Tensor& self) {
  return native::randn_like(self, self.options());
}

Tensor randn_like(const Tensor& self, const TensorOptions& options) {
  return native::randn(self.sizes(), nullptr, options);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ randperm ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {
template <typename scalar_t>
void randperm_cpu(Tensor& result, int64_t n, THGenerator* generator) {
  scalar_t *r__data = result.data<scalar_t>();

  result.resize_({n});
  int64_t r__stride_0 = result.stride(0);

  for(int64_t i = 0; i < n; i++) {
    r__data[i*r__stride_0] = static_cast<scalar_t>(i);
  }

  for(int64_t i = 0; i < n - 1; i++)
  {
    int64_t z = THRandom_random(generator) % (n-i);
    scalar_t sav = r__data[i*r__stride_0];
    r__data[i*r__stride_0] = r__data[(z+i)*r__stride_0];
    r__data[(z+i)*r__stride_0] = sav;
  }
}
} // namespace


THGenerator* get_generator(at::Generator* gen) {
  auto default_gen = &at::globalContext().defaultGenerator(at::Backend::CPU);
  auto gen_ = at::check_generator<at::CPUGenerator>(gen, default_gen);
  return gen_->generator;
}

Tensor randperm(int64_t n, const TensorOptions& options) {
  return native::randperm(n, nullptr, options);
}

Tensor randperm(int64_t n, Generator* generator, const TensorOptions& options) {
  auto tensor = options.type().tensor(n);
  return at::randperm_out(tensor, n, generator);
}

Tensor& randperm_out(Tensor& result, int64_t n) {
  return at::randperm_out(result, n, nullptr);
}

Tensor& randperm_out_cpu(Tensor& result, int64_t n, Generator* generator) {
  AT_CHECK(n >= 0, "n must be non-negative, got", n);
  result.resize_({n});
  auto gen = get_generator(generator);
  AT_DISPATCH_ALL_TYPES(result.type(), "randperm", [&]() -> void {
    randperm_cpu<scalar_t>(result, n, gen);
  });

  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ range ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tensor range(Scalar start, Scalar end, const TensorOptions& options) {
  return native::range(start, end, /*step=*/1, options);
}

Tensor range(
    Scalar start,
    Scalar end,
    Scalar step,
    const TensorOptions& options) {
  return options.type()._range(start, end, step);
}

Tensor& range_out(Tensor& result, Scalar start, Scalar end) {
  return native::range_out(result, start, end, 1);
}

Tensor& range_out(Tensor& result, Scalar start, Scalar end, Scalar step) {
  return at::_range_out(result, start, end, step);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ zeros ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tensor zeros(IntList size, const TensorOptions& options) {
  auto result = options.type().tensor(size);
  return result.zero_();
}

Tensor& zeros_out(Tensor& result, IntList size) {
  if (result.is_sparse()) {
    result.sparse_raw_resize_(size, size.size(), 0);
  } else {
    result.resize_(size);
  }
  return result.zero_();
}

Tensor zeros_like(const Tensor& self) {
  return native::zeros_like(self, self.options());
}

Tensor zeros_like(const Tensor& self, const TensorOptions& options) {
  if (options.layout() == kSparse && self.type().is_sparse()) {
    auto res = options.type().tensor({});
    // resize_as_ requires the same exact type.
    res.sparse_raw_resize_(self.sizes(), self._sparseDims(), self._denseDims());
    return res;
  }
  return native::zeros(self.sizes(), options);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~ bartlett_window ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tensor bartlett_window(int64_t window_length, const TensorOptions& options) {
  return native::bartlett_window(window_length, /*periodic=*/true, options);
}

Tensor bartlett_window(
    int64_t window_length,
    bool periodic,
    const TensorOptions& options) {
  window_function_checks("bartlett_window", options, window_length);
  if (window_length == 1) {
    return native::ones({1}, options);
  }
  if (periodic) {
    window_length += 1;
  }
  auto window = native::arange(window_length, options).mul_(2. / static_cast<double>(window_length - 1));
  const int64_t first_half_size = ((window_length - 1) >> 1) + 1;
  window.narrow(0, first_half_size, window_length - first_half_size).mul_(-1).add_(2);
  return periodic ? window.narrow(0, 0, window_length - 1) : window;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~ blackman_window ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tensor blackman_window(int64_t window_length, const TensorOptions& options) {
  return native::blackman_window(window_length, /*periodic=*/true, options);
}

Tensor blackman_window(
    int64_t window_length,
    bool periodic,
    const TensorOptions& options) {
  window_function_checks("blackman_window", options, window_length);
  if (window_length == 1) {
    return native::ones({1}, options);
  }
  if (periodic) {
    window_length += 1;
  }
  // from https://en.wikipedia.org/wiki/Window_function#Blackman_window
  auto window = native::arange(window_length, options).mul_(M_PI / static_cast<double>(window_length - 1));
  window = window.mul(4).cos_().mul_(0.08) - window.mul(2).cos_().mul_(0.5) + 0.42;
  return periodic ? window.narrow(0, 0, window_length - 1) : window;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ hamming_window ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tensor hamming_window(int64_t window_length, const TensorOptions& options) {
  return native::hamming_window(window_length, /*periodic=*/true, options);
}

Tensor hamming_window(
    int64_t window_length,
    bool periodic,
    const TensorOptions& options) {
  return native::hamming_window(
      window_length, periodic, /*alpha=*/0.54, options);
}

Tensor hamming_window(
    int64_t window_length,
    bool periodic,
    double alpha,
    const TensorOptions& options) {
  return native::hamming_window(
      window_length, periodic, alpha, /*beta=*/0.46, options);
}

Tensor hamming_window(
    int64_t window_length,
    bool periodic,
    double alpha,
    double beta,
    const TensorOptions& options) {
  window_function_checks("hamming_window", options, window_length);
  if (window_length == 1) {
    return native::ones({1}, options);
  }
  if (periodic) {
    window_length += 1;
  }
  auto window = native::arange(window_length, options);
  window.mul_(M_PI * 2. / static_cast<double>(window_length - 1)).cos_().mul_(-beta).add_(alpha);
  return periodic ? window.narrow(0, 0, window_length - 1) : window;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ hann_window ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tensor hann_window(int64_t window_length, const TensorOptions& options) {
  return native::hann_window(window_length, /*periodic=*/true, options);
}

Tensor hann_window(
    int64_t window_length,
    bool periodic,
    const TensorOptions& options) {
  window_function_checks("hann_window", options, window_length);
  return native::hamming_window(
      window_length, periodic, /*alpha=*/0.5, /*beta=*/0.5, options);
}
} // namespace native
} // namespace at
