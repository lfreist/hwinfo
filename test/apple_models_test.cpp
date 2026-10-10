// Copyright Leon Freist
// Authored by Claude (Sonnet 5.5)

#include "internal/apple_models.h"

#include <gtest/gtest.h>

using hwinfo::internal::apple::mac_marketing_name;

TEST(AppleModels, KnownIdentifiers) {
  static_assert(mac_marketing_name("MacBookPro16,1") == "MacBook Pro (16-inch, 2019)");
  static_assert(mac_marketing_name("iMacPro1,1") == "iMac Pro (2017)");
  static_assert(mac_marketing_name("Macmini8,1") == "Mac mini (2018)");
  static_assert(mac_marketing_name("MacPro7,1") == "Mac Pro (2019)");
  static_assert(mac_marketing_name("MacBookAir10,1") == "MacBook Air (M1, 2020)");
  static_assert(mac_marketing_name("MacBookPro15,1") == "MacBook Pro (15-inch, 2018 or 2019)");  // shared identifier
}

TEST(AppleModels, SharedIdentifiersByProcessor) {
  static_assert(mac_marketing_name("MacBookPro15,1", "Intel(R) Core(TM) i7-8850H CPU @ 2.60GHz") ==
                "MacBook Pro (15-inch, 2018)");
  static_assert(mac_marketing_name("MacBookPro15,1", "Intel(R) Core(TM) i9-9980HK CPU @ 2.40GHz") ==
                "MacBook Pro (15-inch, 2019)");
  static_assert(mac_marketing_name("MacBookPro15,2", "Intel(R) Core(TM) i5-8279U CPU @ 2.40GHz") ==
                "MacBook Pro (13-inch, 2019, Four Thunderbolt 3 ports)");
  static_assert(mac_marketing_name("MacBookAir7,2", "Intel(R) Core(TM) i5-5250U CPU @ 1.60GHz") ==
                "MacBook Air (13-inch, Early 2015)");
  // sold in both years / unknown processor: combined name
  static_assert(mac_marketing_name("MacBookAir7,2", "Intel(R) Core(TM) i7-5650U CPU @ 2.20GHz") ==
                "MacBook Air (13-inch, Early 2015 or 2017)");
  static_assert(mac_marketing_name("MacBookPro15,1", "Some future CPU") == "MacBook Pro (15-inch, 2018 or 2019)");
  // the processor only matters for shared identifiers
  static_assert(mac_marketing_name("MacBookPro16,1", "Intel(R) Core(TM) i7-8850H CPU @ 2.60GHz") ==
                "MacBook Pro (16-inch, 2019)");
}

TEST(AppleModels, UnknownIdentifiers) {
  static_assert(!mac_marketing_name(""));
  static_assert(!mac_marketing_name("Mac14,2"));  // new scheme: reported by the system itself
  static_assert(!mac_marketing_name("MacBookPro16"));
  static_assert(!mac_marketing_name("MacBookPro99,1"));
}
