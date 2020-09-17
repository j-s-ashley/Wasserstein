//------------------------------------------------------------------------
// This file is part of Wasserstein, a C++ library with a Python wrapper
// that computes the Wasserstein/EMD distance. If you use it for academic
// research, please cite or acknowledge the following works:
//
//   - Komiske, Metodiev, Thaler (2019) arXiv:1902.02346
//       https://doi.org/10.1103/PhysRevLett.123.041801
//   - Boneel, van de Panne, Paris, Heidrich (2011)
//       https://doi.org/10.1145/2070781.2024192
//   - LEMON graph library https://lemon.cs.elte.hu/trac/lemon
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//------------------------------------------------------------------------

#ifndef WASSERSTEIN_PAIRWISEDISTANCE_HH
#define WASSERSTEIN_PAIRWISEDISTANCE_HH

// C++ standard library
#include <cmath>

#include "EMDUtils.hh"

BEGIN_EMD_NAMESPACE

////////////////////////////////////////////////////////////////////////////////
// PairwiseDistanceBase - implements (theta_ij/R)^beta between particles i and j
////////////////////////////////////////////////////////////////////////////////

template <class PairwiseDistance, class ParticleCollection, class Value>
struct PairwiseDistanceBase {
  typedef typename ParticleCollection::value_type Particle;
  typedef typename ParticleCollection::const_iterator ParticleIterator;

  PairwiseDistanceBase(Value R, Value beta) :
    R_(R), R2_(R*R), beta_(beta), halfbeta_(beta_/2)
  {

    // check that we properly have passed a derived class
    static_assert(std::is_base_of<PairwiseDistanceBase<PairwiseDistance, ParticleCollection, Value>, PairwiseDistance>::value, 
                  "Template parameter must be a derived class of PairwiseDistanceBase.");

    // check parameters
    if (beta_ < 0) throw std::invalid_argument("beta must be non-negative.");
    if (R_ <= 0) throw std::invalid_argument("R must be positive.");
  }

  // description of class
  std::string description() const {
    std::ostringstream oss;
    oss << "  " << PairwiseDistance::name() << '\n'
        << "    R - " << R() << '\n'
        << "    beta - " << beta() << '\n'
        << '\n';
    return oss.str();
  }

  // access parameters
  Value R() const { return R_; }
  Value beta() const { return beta_; }

  // computes pairwise distances between two particle collections
  void fill_distances(const ParticleCollection & ps0, const ParticleCollection & ps1,
                      std::vector<Value> & dists, ExtraParticle extra) {

    //static_cast<const PairwiseDistance &>(*this).init(ps0, ps1);
    std::size_t k(0);

    if (extra == ExtraParticle::Neither) {
      dists.resize(ps0.size() * ps1.size());
      for (ParticleIterator p0 = ps0.begin(), end0 = ps0.end(), end1 = ps1.end(); p0 != end0; ++p0)
        for (ParticleIterator p1 = ps1.begin(); p1 != end1; ++p1)
          dists[k++] = distance(p0, p1);
    }
    else if (extra == ExtraParticle::Zero) {
      dists.resize((ps0.size() + 1) * ps1.size());
      for (ParticleIterator p0 = ps0.begin(), end0 = ps0.end(), end1 = ps1.end(); p0 != end0; ++p0)
        for (ParticleIterator p1 = ps1.begin(); p1 != end1; ++p1)
          dists[k++] = distance(p0, p1);
      for (std::size_t j = 0; j < ps1.size(); j++)
        dists[k++] = 1;
    }

    // extra == ExtraParticle::One
    else {
      dists.resize(ps0.size() * (ps1.size() + 1));
      for (ParticleIterator p0 = ps0.begin(), end0 = ps0.end(), end1 = ps1.end(); p0 != end0; ++p0) {
        for (ParticleIterator p1 = ps1.begin(); p1 != end1; ++p1)
          dists[k++] = distance(p0, p1);
        dists[k++] = 1;
      }
    }
  }

  // returns the distance divided by R, all to beta power
  Value distance(const ParticleIterator & p0, const ParticleIterator & p1) const {
    Value pd(PairwiseDistance::plain_distance_(p0, p1));
    return (beta_ == 1.0 ? std::sqrt(pd)/R_ : (beta_ == 2.0 ? pd/R2_ : std::pow(pd/R2_, halfbeta_)));
  }

  // return the plain distance, without the square root
  static Value plain_distance_(const ParticleIterator & p0, const ParticleIterator & p1) {
    return PairwiseDistance::plain_distance(*p0, *p1);
  }

private:

  Value R_, R2_, beta_, halfbeta_;

}; // PairwiseDistanceBase

////////////////////////////////////////////////////////////////////////////////
// EuclideanArrayDistance - euclidean distance between two particle arrays
////////////////////////////////////////////////////////////////////////////////

template<typename V = double>
struct EuclideanArrayDistance : PairwiseDistanceBase<EuclideanArrayDistance<V>, ArrayParticleCollection<V>, V> {
  typedef ArrayParticleCollection<V> ParticleCollection;
  typedef typename ParticleCollection::value_type Particle;
  typedef typename ParticleCollection::const_iterator ParticleIterator;
  typedef V Value;

  EuclideanArrayDistance(Value R, Value beta) :
    PairwiseDistanceBase<EuclideanArrayDistance<V>, ArrayParticleCollection<V>, V>(R, beta)
  {}
  static std::string name() { return "EuclideanArrayDistance"; }
  static Value plain_distance_(const ParticleIterator & p0, const ParticleIterator & p1) {
    Value d(0);
    if (p0.stride() == 2) {
      Value dx((*p0)[0] - (*p1)[0]), dy((*p0)[1] - (*p1)[1]);
      d = dx*dx + dy*dy;
    }
    else 
      for (int i = 0; i < p0.stride(); i++) {
        Value dx((*p0)[i] - (*p1)[i]);
        d += dx*dx;
      }
    return d;
  }
}; // EuclideanArrayDistance

////////////////////////////////////////////////////////////////////////////////
// GenericDistance - base class for a pairwise distance between two "particles"
////////////////////////////////////////////////////////////////////////////////

template<class P>
struct GenericDistance : public PairwiseDistanceBase<GenericDistance<P>, std::vector<P>, typename P::Value> {
  typedef P Particle;
  typedef typename Particle::Value Value;

  GenericDistance(Value R, Value beta) :
    PairwiseDistanceBase<GenericDistance, std::vector<P>, Value>(R, beta)
  {}
  static std::string name() { return Particle::distance_name(); }
  static Value plain_distance(const Particle & p0, const Particle & p1) {
    return Particle::plain_distance(p0, p1);
  }
}; // GenericDistance

////////////////////////////////////////////////////////////////////////////////
// EuclideanDistance - between double-precision euclidean particles
////////////////////////////////////////////////////////////////////////////////

// euclidean distances with double-precision particles
using EuclideanDistance2D = GenericDistance<EuclideanParticle2D<>>;
using EuclideanDistance3D = GenericDistance<EuclideanParticle3D<>>;
template<unsigned N>
using EuclideanDistanceND = GenericDistance<EuclideanParticleND<N>>;

END_EMD_NAMESPACE

#endif // WASSERSTEIN_PAIRWISEDISTANCE_HH
