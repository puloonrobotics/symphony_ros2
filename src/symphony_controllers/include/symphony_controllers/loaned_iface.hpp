#ifndef SYMPHONY_CONTROLLERS__LOANED_IFACE_HPP_
#define SYMPHONY_CONTROLLERS__LOANED_IFACE_HPP_

#include <limits>
#include <optional>

#include "hardware_interface/loaned_command_interface.hpp"
#include "hardware_interface/loaned_state_interface.hpp"

namespace symphony_controllers
{

inline void set_command(
  hardware_interface::LoanedCommandInterface & iface, double value)
{
  [[maybe_unused]] const bool ok = iface.set_value(value);
}

inline double get_double(
  const hardware_interface::LoanedStateInterface & iface)
{
  const std::optional<double> v = iface.get_optional();
  return v.value_or(std::numeric_limits<double>::quiet_NaN());
}

inline double get_double(
  const hardware_interface::LoanedCommandInterface & iface)
{
  const std::optional<double> v = iface.get_optional();
  return v.value_or(std::numeric_limits<double>::quiet_NaN());
}

}  // namespace symphony_controllers

#endif  // SYMPHONY_CONTROLLERS__LOANED_IFACE_HPP_
