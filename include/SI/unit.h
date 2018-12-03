#pragma once

#include "detail.h"
#include <ratio>
#include <type_traits>

namespace SI {

/**
 * @brief base template class for holding values of type _Type to be multiplied
 *with a ratio _Ratio
 *
 * @tparam _Symbol dimension symbol of the unit (see
 *https://en.wikipedia.org/wiki/SI_base_unit)
 * @tparam _Ratio Ratio in relation to the base unit of a SI unit
 * @tparam _Type internal representation of the value
 * @tparam _Exponent the exponent to the unit (i.e. length ==  m^1, area == m^2,
 *volume = m^3)
 **/
template <
    char _Symbol, char _Exponent = 1, class _Ratio = std::ratio<1>,
    typename _Type = long long int,
    typename std::enable_if<_Ratio::num == 1 || _Ratio::den == 1>::type * =
        nullptr,
    typename std::enable_if<std::is_arithmetic<_Type>::value>::type * = nullptr>

struct unit_t {

  static_assert(_Exponent != 0, "Exponent is non-zero");
  static_assert(detail::is_ratio<_Ratio>::value, "_Ratio is a std::ratio");
  typedef _Ratio ratio;
  typedef _Type internal_type;
  typedef std::integral_constant<char, _Exponent> exponent;
  typedef std::integral_constant<char, _Symbol> symbol;

  /// Construct with value v
  constexpr unit_t(_Type v) : value_{v} {}

  /// returns the stored value as raw type
  constexpr internal_type raw_value() const { return value_; }

  /// Comparison operator takes considers different ratios, i.e. 1000 micros ===
  /// 1 milli
  template <class _rhs_Ratio = std::ratio<1>>
  constexpr bool
  operator==(const unit_t<symbol::value, exponent::value, _rhs_Ratio,
                          internal_type> &rhs) const {
    typedef typename std::remove_reference<decltype(rhs)>::type rhs_t;
    return (rhs.value_ * rhs_t::ratio::num / rhs_t::ratio::den) ==
           (value_ * ratio::num / ratio::den);
  }
  /// multiply with a non-unit scalar
  constexpr unit_t operator*(const _Type f) const { return {value_ * f}; }

  /// multiply with a same unit
  /// resulting unit is the same as 'this'/left hand side of operation
  /// @todo use unit_cast to get correct value
  template <char _rhs_Exponent, typename _rhs_Ratio>
  constexpr auto operator*(const unit_t<symbol::value, _rhs_Exponent,
                                        _rhs_Ratio, internal_type> &rhs) const {
    typedef typename std::remove_reference<decltype(rhs)>::type rhs_t;
    constexpr auto conversion_ratio = detail::ratio_to<ratio, _rhs_Ratio>();
    return unit_t<symbol::value, exponent::value + rhs_t::exponent::value,
                  ratio, internal_type>{
        value_ * (rhs.raw_value() * decltype(conversion_ratio)::num /
                  decltype(conversion_ratio)::den)};
  }

  /// multiply with a same unit
  /// resulting unit is the same as 'this'/left hand side of operation
  /// @todo use unit_cast to get correct value
  template <char _rhs_exponent, typename _rhs_Ratio,
            typename std::enable_if<_rhs_exponent != exponent::value>::type * =
                nullptr>
  constexpr auto operator/(const unit_t<symbol::value, _rhs_exponent,
                                        _rhs_Ratio, internal_type> &rhs) const {
    typedef typename std::remove_reference<decltype(rhs)>::type rhs_t;
    constexpr auto conversion_ratio = detail::ratio_to<ratio, _rhs_Ratio>();

    return unit_t<symbol::value, exponent::value - rhs_t::exponent::value,
                  ratio, internal_type>{
        value_ / (rhs.raw_value() * decltype(conversion_ratio)::num /
                  decltype(conversion_ratio)::den)};
  }

  /// if the same units of the same exponent are divided then the result is a
  /// scalar
  template <char _rhs_exponent, typename _rhs_Ratio,
            typename std::enable_if<_rhs_exponent == exponent::value>::type * =
                nullptr>
  constexpr _Type
  operator/(const unit_t<symbol::value, _rhs_exponent, _rhs_Ratio,
                         internal_type> &rhs) const {
    constexpr auto conversion_ratio = detail::ratio_to<ratio, _rhs_Ratio>();

    return raw_value() / rhs.raw_value();
  }

  /// negate operation
  constexpr unit_t operator-() { return {-value_}; }

  internal_type value_;
};

/// operator to divide
/// @todo return divided raw_value and handle different ratios
template <char _Symbol>
constexpr auto operator/(long long int, const unit_t<_Symbol> &rhs) {
  return unit_t<_Symbol,
                -std::remove_reference<decltype(rhs)>::type::exponent::value>{
      0};
}

/// helper template to check if a type is a unit_t (false for all other types)
template <typename _Tp> struct is_unit_t : std::false_type {};

/// template specialisation to check if a type is a unit_t (true if unit_t)
template <char _Symbol, char _Exponent, class _Ratio, typename _Type>
struct is_unit_t<const unit_t<_Symbol, _Exponent, _Ratio, _Type>>
    : std::true_type {};

/// non-const specialisation of check above
template <char _Symbol, char _Exponent, class _Ratio, typename _Type>
struct is_unit_t<unit_t<_Symbol, _Exponent, _Ratio, _Type>> : std::true_type {};

/// function to cast between two units of the same type
template <
    typename _T, typename _rhs_T,
    typename std::enable_if<
        is_unit_t<_rhs_T>::value ||
        std::is_base_of<
            unit_t<_rhs_T::symbol::value, _rhs_T::exponent::value,
                   typename _rhs_T::ratio, typename _rhs_T::internal_type>,
            _rhs_T>::value>::type * = nullptr>
constexpr auto unit_cast(const _rhs_T &rhs) {
  constexpr auto conversion_ratio =
      detail::ratio_to<typename _T::ratio, typename _rhs_T::ratio>();
  return _T{(rhs.raw_value() * decltype(conversion_ratio)::num /
             decltype(conversion_ratio)::den)};
}

} // namespace SI