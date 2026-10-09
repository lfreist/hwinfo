// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Marketing names of Macs by model identifier. Not part of the public API.
//
// Apple Silicon Macs report their marketing name in the I/O Registry ("product-name"), Intel Macs do not. This table
// covers all model identifiers of the old "<Product><major>,<minor>" scheme, which is closed: Macs released since 2022
// use "Mac<major>,<minor>" identifiers and are all Apple Silicon.
// Source: Apple's "Identify your Mac model" support articles. Identifiers shared by several models (distinguishable
// only by serial number) get a combined name.

#pragma once

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>
#include <utility>

namespace hwinfo::internal::apple {

// Sorted by identifier.
inline constexpr std::array<std::pair<std::string_view, std::string_view>, 99> mac_marketing_names{{
    {"MacBook10,1", "MacBook (Retina, 12-inch, 2017)"},
    {"MacBook5,2", "MacBook (13-inch, 2009)"},
    {"MacBook6,1", "MacBook (13-inch, Late 2009)"},
    {"MacBook7,1", "MacBook (13-inch, Mid 2010)"},
    {"MacBook8,1", "MacBook (Retina, 12-inch, Early 2015)"},
    {"MacBook9,1", "MacBook (Retina, 12-inch, Early 2016)"},
    {"MacBookAir10,1", "MacBook Air (M1, 2020)"},
    {"MacBookAir2,1", "MacBook Air (Mid 2009)"},
    {"MacBookAir3,1", "MacBook Air (11-inch, Late 2010)"},
    {"MacBookAir3,2", "MacBook Air (13-inch, Late 2010)"},
    {"MacBookAir4,1", "MacBook Air (11-inch, Mid 2011)"},
    {"MacBookAir4,2", "MacBook Air (13-inch, Mid 2011)"},
    {"MacBookAir5,1", "MacBook Air (11-inch, Mid 2012)"},
    {"MacBookAir5,2", "MacBook Air (13-inch, Mid 2012)"},
    {"MacBookAir6,1", "MacBook Air (11-inch, Mid 2013 or Early 2014)"},
    {"MacBookAir6,2", "MacBook Air (13-inch, Mid 2013 or Early 2014)"},
    {"MacBookAir7,1", "MacBook Air (11-inch, Early 2015)"},
    {"MacBookAir7,2", "MacBook Air (13-inch, Early 2015 or 2017)"},
    {"MacBookAir8,1", "MacBook Air (Retina, 13-inch, 2018)"},
    {"MacBookAir8,2", "MacBook Air (Retina, 13-inch, 2019)"},
    {"MacBookAir9,1", "MacBook Air (Retina, 13-inch, 2020)"},
    {"MacBookPro10,1", "MacBook Pro (Retina, 15-inch, Mid 2012 or Early 2013)"},
    {"MacBookPro10,2", "MacBook Pro (Retina, 13-inch, Late 2012 or Early 2013)"},
    {"MacBookPro11,1", "MacBook Pro (Retina, 13-inch, Late 2013 or Mid 2014)"},
    {"MacBookPro11,2", "MacBook Pro (Retina, 15-inch, Late 2013 or Mid 2014)"},
    {"MacBookPro11,3", "MacBook Pro (Retina, 15-inch, Late 2013 or Mid 2014)"},
    {"MacBookPro11,4", "MacBook Pro (Retina, 15-inch, Mid 2015)"},
    {"MacBookPro11,5", "MacBook Pro (Retina, 15-inch, Mid 2015)"},
    {"MacBookPro12,1", "MacBook Pro (Retina, 13-inch, Early 2015)"},
    {"MacBookPro13,1", "MacBook Pro (13-inch, 2016, Two Thunderbolt 3 ports)"},
    {"MacBookPro13,2", "MacBook Pro (13-inch, 2016, Four Thunderbolt 3 ports)"},
    {"MacBookPro13,3", "MacBook Pro (15-inch, 2016)"},
    {"MacBookPro14,1", "MacBook Pro (13-inch, 2017, Two Thunderbolt 3 ports)"},
    {"MacBookPro14,2", "MacBook Pro (13-inch, 2017, Four Thunderbolt 3 ports)"},
    {"MacBookPro14,3", "MacBook Pro (15-inch, 2017)"},
    {"MacBookPro15,1", "MacBook Pro (15-inch, 2018 or 2019)"},
    {"MacBookPro15,2", "MacBook Pro (13-inch, 2018 or 2019, Four Thunderbolt 3 ports)"},
    {"MacBookPro15,3", "MacBook Pro (15-inch, 2019)"},
    {"MacBookPro15,4", "MacBook Pro (13-inch, 2019, Two Thunderbolt 3 ports)"},
    {"MacBookPro16,1", "MacBook Pro (16-inch, 2019)"},
    {"MacBookPro16,2", "MacBook Pro (13-inch, 2020, Four Thunderbolt 3 ports)"},
    {"MacBookPro16,3", "MacBook Pro (13-inch, 2020, Two Thunderbolt 3 ports)"},
    {"MacBookPro16,4", "MacBook Pro (16-inch, 2019)"},
    {"MacBookPro17,1", "MacBook Pro (13-inch, M1, 2020)"},
    {"MacBookPro18,1", "MacBook Pro (16-inch, 2021)"},
    {"MacBookPro18,2", "MacBook Pro (16-inch, 2021)"},
    {"MacBookPro18,3", "MacBook Pro (14-inch, 2021)"},
    {"MacBookPro18,4", "MacBook Pro (14-inch, 2021)"},
    {"MacBookPro4,1", "MacBook Pro (Early 2008)"},
    {"MacBookPro5,1", "MacBook Pro (15-inch, Late 2008)"},
    {"MacBookPro5,2", "MacBook Pro (17-inch, 2009)"},
    {"MacBookPro5,3", "MacBook Pro (15-inch, Mid 2009)"},
    {"MacBookPro5,5", "MacBook Pro (13-inch, Mid 2009)"},
    {"MacBookPro6,1", "MacBook Pro (17-inch, Mid 2010)"},
    {"MacBookPro6,2", "MacBook Pro (15-inch, Mid 2010)"},
    {"MacBookPro7,1", "MacBook Pro (13-inch, Mid 2010)"},
    {"MacBookPro8,1", "MacBook Pro (13-inch, 2011)"},
    {"MacBookPro8,2", "MacBook Pro (15-inch, 2011)"},
    {"MacBookPro8,3", "MacBook Pro (17-inch, 2011)"},
    {"MacBookPro9,1", "MacBook Pro (15-inch, Mid 2012)"},
    {"MacBookPro9,2", "MacBook Pro (13-inch, Mid 2012)"},
    {"MacPro4,1", "Mac Pro (Early 2009)"},
    {"MacPro5,1", "Mac Pro (Mid 2010 or Mid 2012)"},
    {"MacPro6,1", "Mac Pro (Late 2013)"},
    {"MacPro7,1", "Mac Pro (2019)"},
    {"Macmini3,1", "Mac mini (2009)"},
    {"Macmini4,1", "Mac mini (Mid 2010)"},
    {"Macmini5,1", "Mac mini (Mid 2011)"},
    {"Macmini5,2", "Mac mini (Mid 2011)"},
    {"Macmini6,1", "Mac mini (Late 2012)"},
    {"Macmini6,2", "Mac mini (Late 2012)"},
    {"Macmini7,1", "Mac mini (Late 2014)"},
    {"Macmini8,1", "Mac mini (2018)"},
    {"Macmini9,1", "Mac mini (M1, 2020)"},
    {"iMac10,1", "iMac (Late 2009)"},
    {"iMac11,2", "iMac (21.5-inch, Mid 2010)"},
    {"iMac11,3", "iMac (27-inch, Mid 2010)"},
    {"iMac12,1", "iMac (21.5-inch, Mid 2011)"},
    {"iMac12,2", "iMac (27-inch, Mid 2011)"},
    {"iMac13,1", "iMac (21.5-inch, Late 2012)"},
    {"iMac13,2", "iMac (27-inch, Late 2012)"},
    {"iMac14,1", "iMac (21.5-inch, Late 2013)"},
    {"iMac14,2", "iMac (27-inch, Late 2013)"},
    {"iMac14,4", "iMac (21.5-inch, Mid 2014)"},
    {"iMac15,1", "iMac (Retina 5K, 27-inch, Late 2014 or Mid 2015)"},
    {"iMac16,1", "iMac (21.5-inch, Late 2015)"},
    {"iMac16,2", "iMac (Retina 4K, 21.5-inch, Late 2015)"},
    {"iMac17,1", "iMac (Retina 5K, 27-inch, Late 2015)"},
    {"iMac18,1", "iMac (21.5-inch, 2017)"},
    {"iMac18,2", "iMac (Retina 4K, 21.5-inch, 2017)"},
    {"iMac18,3", "iMac (Retina 5K, 27-inch, 2017)"},
    {"iMac19,1", "iMac (Retina 5K, 27-inch, 2019)"},
    {"iMac19,2", "iMac (Retina 4K, 21.5-inch, 2019)"},
    {"iMac20,1", "iMac (Retina 5K, 27-inch, 2020)"},
    {"iMac20,2", "iMac (Retina 5K, 27-inch, 2020)"},
    {"iMac21,1", "iMac (24-inch, M1, 2021)"},
    {"iMac21,2", "iMac (24-inch, M1, 2021)"},
    {"iMac9,1", "iMac (Early 2009)"},
    {"iMacPro1,1", "iMac Pro (2017)"},
}};

static_assert(std::ranges::is_sorted(mac_marketing_names, {}, &std::pair<std::string_view, std::string_view>::first));

// Marketing name for a model identifier (e.g. "MacBookPro16,1" -> "MacBook Pro (16-inch, 2019)").
constexpr std::optional<std::string_view> mac_marketing_name(std::string_view identifier) noexcept {
  const auto it = std::ranges::lower_bound(mac_marketing_names, identifier, {},
                                           &std::pair<std::string_view, std::string_view>::first);
  if (it == mac_marketing_names.end() || it->first != identifier) {
    return std::nullopt;
  }
  return it->second;
}

}  // namespace hwinfo::internal::apple
