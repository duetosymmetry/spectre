// Distributed under the MIT License.
// See LICENSE.txt for details.

#include <catch.hpp>

#include "DataStructures/Tensor/EagerMath/DivideBy.hpp"
#include "tests/Unit/TestHelpers.hpp"
#include "tests/Unit/TestingFramework.hpp"

SPECTRE_TEST_CASE("Unit.DataStructures.Tensor.EagerMath.DivideBy",
                  "[DataStructures][Unit]") {
  const size_t npts = 2;
  const DataVector one(npts, 1.0);
  const DataVector two(npts, 2.0);
  const DataVector three(npts, 3.0);
  const DataVector four(npts, 4.0);
  const DataVector five(npts, 5.0);
  const DataVector twelve(npts, 12.0);
  const DataVector thirteen(npts, 13.0);

  const Tensor<DataVector, Symmetry<1>,
               tmpl::list<SpatialIndex<1, UpLo::Lo, Frame::Grid>>>
      one_d_covector{{{two}}};
  const auto normalized_one_d_covector = divide_by(one_d_covector, two);
  for (size_t s = 0; s < npts; ++s) {
    CHECK(get<0>(normalized_one_d_covector)[s] == 1.0);
  }

  const Tensor<DataVector, Symmetry<1>,
               tmpl::list<SpacetimeIndex<1, UpLo::Up, Frame::Grid>>>
      two_d_vector{{{three, four}}};
  const auto normalized_two_d_vector = divide_by(two_d_vector, five);
  for (size_t s = 0; s < npts; ++s) {
    CHECK(get<0>(normalized_two_d_vector)[s] == 0.6);
    CHECK(get<1>(normalized_two_d_vector)[s] == 0.8);
  }

  const Tensor<DataVector, Symmetry<1>,
               tmpl::list<SpatialIndex<2, UpLo::Up, Frame::Grid>>>
      two_d_spatial_vector{{{five, twelve}}};
  const auto normalized_two_d_spatial_vector =
      divide_by(two_d_spatial_vector, thirteen);
  for (size_t s = 0; s < npts; ++s) {
    CHECK(get<0>(normalized_two_d_spatial_vector)[s] == 5.0 / 13.0);
    CHECK(get<1>(normalized_two_d_spatial_vector)[s] == 12.0 / 13.0);
  }

  const Tensor<DataVector, Symmetry<1>,
               tmpl::list<SpatialIndex<3, UpLo::Lo, Frame::Grid>>>
      three_d_covector{{{three, twelve, four}}};
  const auto normalized_three_d_covector =
      divide_by(three_d_covector, thirteen);
  for (size_t s = 0; s < npts; ++s) {
    CHECK(get<0>(normalized_three_d_covector)[s] == 3.0 / 13.0);
    CHECK(get<1>(normalized_three_d_covector)[s] == 12.0 / 13.0);
    CHECK(get<2>(normalized_three_d_covector)[s] == 4.0 / 13.0);
  }

  const Tensor<DataVector, Symmetry<1>,
               tmpl::list<SpacetimeIndex<4, UpLo::Lo, Frame::Grid>>>
      five_d_covector{{{two, twelve, four, one, two}}};
  const auto normalized_five_d_covector = divide_by(five_d_covector, thirteen);
  for (size_t s = 0; s < npts; ++s) {
    CHECK(get<0>(normalized_five_d_covector)[s] == 2.0 / 13.0);
    CHECK(get<1>(normalized_five_d_covector)[s] == 12.0 / 13.0);
    CHECK(get<2>(normalized_five_d_covector)[s] == 4.0 / 13.0);
    CHECK(get<3>(normalized_five_d_covector)[s] == 1.0 / 13.0);
    CHECK(get<4>(normalized_five_d_covector)[s] == 2.0 / 13.0);
  }
}
