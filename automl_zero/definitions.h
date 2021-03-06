// Copyright 2020 The Google Research Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// TODO(ereal):
// -Rename namespace to amlz.
// -Clean up optimized training (leave only optimized code).
// -Clean up functional cache choice (leave only functional cache branch).
// -Simplify train budgets and remove interface.
// -Have only py3 tests.
// -Renumber proto fields.
// -Address or remove all TODOs.
// -Consider removing test_util files.
// -Apply linter.
// -Apply build cleaner.
// -Rename "component_functions" to "component functions" everywhere.
// -Add more comments, link to paper.

#ifndef THIRD_PARTY_GOOGLE_RESEARCH_GOOGLE_RESEARCH_AUTOML_ZERO_DEFINITIONS_H_
#define THIRD_PARTY_GOOGLE_RESEARCH_GOOGLE_RESEARCH_AUTOML_ZERO_DEFINITIONS_H_

#include <sched.h>

#include <atomic>
#include <cstdint>
#include <limits>
#include <sstream>
#include <string>
#include <thread>  // NOLINT(build/c++11)

#include "base/integral_types.h"
#include "definitions.proto.h"
#include "google/protobuf/text_format.h"
#include "absl/flags/flag.h"
#include "third_party/eigen3/Eigen/Core"
#include "util/hash/mix.h"
#include "util/intops/safe_int_ops.h"

////////////////////////////////////////////////////////////////////////////////
// Conventions.
////////////////////////////////////////////////////////////////////////////////

// F = template argument for the size of the features and all tensor coords.

////////////////////////////////////////////////////////////////////////////////
// Preprocessor directives.
////////////////////////////////////////////////////////////////////////////////

// These allow defining compile-time flags. They can be used to evolve larger
// component_functions without forcing the small-component_function evolution to
// be slow.

// NOTE: if you specify any of these in the command line and you want to analyze
// the results in Colab, you must specify the same values when you use
// adhoc_import.

#ifndef MAX_SCALAR_ADDRESSES
  #define MAX_SCALAR_ADDRESSES 20
#endif
#ifndef MAX_VECTOR_ADDRESSES
  #define MAX_VECTOR_ADDRESSES 20
#endif
#ifndef MAX_MATRIX_ADDRESSES
  #define MAX_MATRIX_ADDRESSES 20
#endif

namespace brain {
namespace evolution {
namespace amlz {

////////////////////////////////////////////////////////////////////////////////
// Types.
////////////////////////////////////////////////////////////////////////////////

// IntegerT is the preferred type for all integers. Use this unless there is a
// reason not to. Reasons could be the demands of external interfaces or
// speed/space considerations.
// Must be castable to RandomSeedT.
typedef int64_t IntegerT;  // A generic integer.

typedef float ProbabilityT;

typedef std::atomic_llong AtomicIntegerT;

// Type for seeding random generators.
// Must be castable from RandomSeedT.
typedef uint32 RandomSeedT;

// Index for the coordinates of the activations for any rank > 0.
typedef int FeatureIndexT;

typedef double Scalar;

template <FeatureIndexT F>
using Vector = ::Eigen::Matrix<double, F, 1>;

template <FeatureIndexT F>
using Matrix = ::Eigen::Matrix<double, F, F, ::Eigen::RowMajor>;

enum Choice2T : IntegerT {
  kChoice0of2 = 0,
  kChoice1of2 = 1,
};

enum Choice3T : IntegerT {
  kChoice0of3 = 0,
  kChoice1of3 = 1,
  kChoice2of3 = 2,
};

////////////////////////////////////////////////////////////////////////////////
// Constants.
////////////////////////////////////////////////////////////////////////////////

// Useful constant to represent an "infinity" but is only about ~1000x
// the largest value we would use (to prevent numeric overflows).
constexpr IntegerT kUnlimitedTime = 100000000000000000;  // About 3 years.

constexpr IntegerT kNanosPerSecond = 1000000000;
constexpr IntegerT kNanosPerMicro = 1000;

// TODO(ereal): remove?
constexpr FeatureIndexT kFirstFeaturesIndex = 0;

const double kPi = 3.14159265359;
const double kE = 2.71828182846;

// Useful constant to represent an "infinity" but is only about ~1000x
// the largest value we would use (to prevent numeric overflows).
constexpr IntegerT kUnlimitedIndividuals = 1000000000000000;  // Quadrillion.

// Fitness bounds.
constexpr double kMinFitness = 0.0;
constexpr double kMaxFitness = 1.0;

////////////////////////////////////////////////////////////////////////////////
// Memory-related definitions.
////////////////////////////////////////////////////////////////////////////////

// Specifies an address within one of the typed memories (scalar, vector, etc).
typedef uint16_t AddressT;

// Scalar addresses.
// <scalar output branch>.
constexpr AddressT kLabelsScalarAddress = 0;
constexpr AddressT kPredictionsScalarAddress = 1;
constexpr AddressT kFirstOutScalarAddress = 1;
constexpr AddressT kMaxScalarAddresses = MAX_SCALAR_ADDRESSES;

// Vector addresses.
constexpr AddressT kFeaturesVectorAddress = 0;
constexpr AddressT kFirstOutVectorAddress = 1;
// <vector output branch>.
constexpr AddressT kLabelsVectorAddress = 1;
constexpr AddressT kPredictionsVectorAddress = 2;
constexpr AddressT kMaxVectorAddresses = MAX_VECTOR_ADDRESSES;

// Matrix addresses.
constexpr AddressT kFirstOutMatrixAddress = 0;
constexpr AddressT kMaxMatrixAddresses = MAX_MATRIX_ADDRESSES;

template <FeatureIndexT F>
std::string VectorToString(const Vector<F>& value) {
  std::ostringstream message;
  message << "[";
  for (IntegerT i = 0; i < F; ++i) {
    message << value(i) << ", ";
  }
  message << "]";
  return message.str();
}
template <FeatureIndexT F>
std::string ToString(const Vector<F>& value) {
  return VectorToString<F>(value);
}

template <FeatureIndexT F>
std::string MatrixToString(const Matrix<F>& value) {
  std::ostringstream message;
  message << "\n[";
  for (IntegerT i = 0; i < F; ++i) {
    message << "[";
    for (IntegerT j = 0; j < F; ++j) {
      message << value(i, j) << ", ";
    }
    message << "],\n";
  }
  message << "]\n";
  return message.str();
}
template <FeatureIndexT F>
std::string ToString(const Matrix<F>& value) {
  return MatrixToString<F>(value);
}

////////////////////////////////////////////////////////////////////////////////
// Instruction-related definitions.
////////////////////////////////////////////////////////////////////////////////

// TODO(ereal): kept to avoid affecting generated random numbers. Remove.
typedef uint16_t DeprecatedOpIndexT;

inline std::vector<Op> ConvertToOps(const std::vector<IntegerT>& values) {
  std::vector<Op> converted_values;
  converted_values.reserve(values.size());
  for (const IntegerT value : values) {
    CHECK(Op_IsValid(value));
    converted_values.push_back(static_cast<Op>(value));
  }
  return converted_values;
}

////////////////////////////////////////////////////////////////////////////////
// Algorithm-related definitions.
////////////////////////////////////////////////////////////////////////////////

// The index of an instruction within the Algorithm.
typedef uint16_t InstructionIndexT;

////////////////////////////////////////////////////////////////////////////////
// Commonly used methods.
////////////////////////////////////////////////////////////////////////////////

// Cast safely between integer types. Do not overload. Can specialize.
template<typename InT, typename OutT>
OutT SafeCast(const InT value) {  // TODO(ereal): can remove?
  OutT result;
  CHECK(util_intops::SafeAdd(value, 0, &result));
  return result;
}

// Convenience methods to parse protos.
template <class ProtoT>
ProtoT ParseSerialized(const std::string& str) {
  ProtoT proto;
  CHECK(proto.ParseFromString(str));
  return proto;
}
template <class ProtoT>
ProtoT ParseTextFormat(const std::string& str) {
  ProtoT proto;
  CHECK(proto2::TextFormat::ParseFromString(str, &proto));
  return proto;
}

// Convenience methods to parse initializer list arguments.
template<typename NumericT>
NumericT PositiveOrDie(const NumericT value) {
  CHECK_GT(value, NumericT()) << "Found non-positive." << std::endl;
  return value;
}
template<typename PointerT>
PointerT NotNullOrDie(PointerT value) {
  CHECK(value != nullptr) << "Found null." << std::endl;
  return value;
}
template<typename ContainerT>  // Also works for strings.
const ContainerT& NonEmptyOrDie(const ContainerT& value) {
  CHECK(!value.empty()) << "Found empty." << std::endl;
  return value;
}
template<typename ContainerT>  // Also works for strings.
ContainerT& NonEmptyOrDie(ContainerT& value) {
  CHECK(!value.empty()) << "Found empty." << std::endl;
  return value;
}
template<typename ContainerT>  // Also works for strings.
ContainerT* NonEmptyOrDie(ContainerT* value) {
  CHECK(!value->empty()) << "Found empty." << std::endl;
  return value;
}
template<typename ContainerT>  // Also works for strings.
const ContainerT& SizeLessThanOrDie(
    const ContainerT& value, const size_t max_size) {
  CHECK_LT(value.size(), max_size) << "Too large." << std::endl;
  return value;
}
template<typename ContainerT>  // Also works for strings.
ContainerT& SizeLessThanOrDie(
    ContainerT& value, const size_t max_size) {
  CHECK_LT(value.size(), max_size) << "Too large." << std::endl;
  return value;
}
template<typename ContainerT>  // Also works for strings.
ContainerT* SizeLessThanOrDie(
    ContainerT* value, const size_t max_size) {
  CHECK_LT(value->size(), max_size) << "Too large." << std::endl;
  return value;
}

// Method to let other threads do work.
inline void Chill() {
  std::this_thread::yield();  // TODO(ereal): reconsider.
  sched_yield();
}

// Print and Flush can be used to print to stdout for debugging purposes.
// Usage:
// Print() << "my_variable = " << my_variable << stuff << "etc." << Flush();
class Flush {};
class Print {
 public:
  Print() {
    stream_ << "DEBUG: ";
  }

  template <typename PrintedT>
  Print& operator<<(const PrintedT& component) {
    stream_ << component;
    return *this;
  }

  template <>
  Print& operator<< <Flush>(const Flush& component) {
    std::cout << stream_.str() << std::endl;
    return *this;
  }

 private:
  std::ostringstream stream_;
};

// Hash-mixes a vector of numbers. The numbers must be of a type that can be
// casted to a size_t (it must be unsigned and it must have <= 64 bits).
// Intended to be used with the RandomSeedT type.
template<typename NumberT>
NumberT CustomHashMix(const std::vector<NumberT>& numbers) {
  HashMix mix;
  for (const NumberT number : numbers) {
    mix.Mix(static_cast<size_t>(number));
  }
  return mix.get();
}

// Hash-mixes two numbers. The numbers must be of a type that can be
// casted to a size_t (it must be unsigned and it must have <= 64 bits).
// Intended to be used with the RandomSeedT type.
template<typename NumberT>
NumberT CustomHashMix(NumberT first, NumberT second) {
  return CustomHashMix<NumberT>({first, second});
}

}  // namespace amlz
}  // namespace evolution
}  // namespace brain

#endif  // THIRD_PARTY_GOOGLE_RESEARCH_GOOGLE_RESEARCH_AUTOML_ZERO_DEFINITIONS_H_
