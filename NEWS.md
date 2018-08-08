3.17.1
======

## Features

 * Client side failback mode (#2183)
 * New linux-audit() source as SCL (#2186)
 * LogThreadedDestDriver batching (#2063)
 * Decorating generated configuration (#2218)
 * diskq: dqtool cat print backlog items (#2213)
 * Introduce syslog-ng() source (#2199)
 * scl/ewmm: rename syslog-ng() source to ewmm() (#2209)  -- a syslog-ng() source nem lett release-elve az átnevezés előtt
 * cisco: add parsing of Cisco unified call manager (#2134)
 * cfg-block: mandatory parameters (#2088)

## Bugfixes

 * test_python: flush before stopping syslog-ng (#2216)
 * Rewind backlog in case of stateless LogProtoClient (#2214)
 * sudo: filter out incorrectly parsed logs (#2208)
 * client-lib-dir, loggen, eventlog: minor fixes (#2204)
 * Minor stats-query fixes and refactor (#2193)
 * Reliable disk buffer non-empty backlog (#2192)
 * dbld: fix pip package versions on older Distro releases (#2188)
 * Fix a groupset/groupunset and map-value-pairs() crash (#2184)
 * Make g_atomic_counter_set() atomic and update minimum glib version to 2.26 (#2182)
 * regression - scl: aligning java related scl-s with mandatory parameters (#2160)
 * Loggen minor fixes (#2150)
 * grab-logging should be installed as a header (#2151)
 * logqueue-fifo: memory_usage - possible underflow (afsql) (#2140)
 * Fixed selinux module version inconsistency (#2133)
 * Fix CMake unit test compilation (no-pie) (#2137)
 * Fix possible crash when using log_msg_clear() (#2128)
 * logwriter: disable ack for mark mode (#2129)
 * telegram: fixing bug with bot id (#2142)
 * All drivers should support inner dst or src plugins (#2143)
 * file-perms: Default to -1 for file and directory owners (#2119)
 * Fix global "center;;received" counter when systemd-journal() is used (#2121)
 * build: Link everything to libsecret-storage (#2100)
 * transport test: add missing CRITERION directive (#2144)
 * dns: Inform about the right dns-cache() configuration (#2145)
 * afinter: adjusting window size for internal mark mode (#2146)
 * DiskQDestPlugin was not freed (#2153)
 * undefined behavior fix (#2154)
 * autotools: linking mode static (#2155)
 * regression - internal mark mode: infinite loop with old ivykis (#2157)
 * Fix missing normalize flags (#2162)
 * Once java always java (#2164)
 * Fix suspend race cond (#2167)
 * scl/cim: add requires json-plugin (#2181)
 * system-source: added exclude_kmsg option instead of container detection (#2166)
 * Fix padding template function (#2174)
 * Leak & invalid memory access (#2173)
 * FreeBSD 11.2 builderror SOCK_STREAM (#2170)
 * afsocket: Add ref-counted TLSVerifier struct (#2168)

## Other changes

 * test_filters_netmask6, test_findeom: port into criterion (#2217)
 * java: warning message correction (#2211)
 * Dbld add sqlite3 and riemann to devshell (#2210)
 * Loggen: improve file reader parse function (#2205)
 * syslog-ng-debun improvements (#2201)
 * Goodbye "goto relex" (refactor) (#2198)
 * WildcardFileReader: the callback registration mechanism is refactored (#2185)
 * csv_parser test: revwrite based on criterion (#2175)
 * Extended Linux capabilities detection (pkg-config) (#2169)
 * Add atomic gssize (#2159)
 * Libtest refactors (#2149)
 * pragma: lower the require message level to debug (#2147)
 * OSX warning elimination (#2139)
 * RewriteExpr: Remove misleading print of result from log_rewrite_queue() (#2132)
 * test_thread_wakeup: activate test (#2130)
 * Minor updates to SELinux policy installer script (#2127)
 * cmake: glib detection more robust (#2125)
 * Logthreaded nonfunctional changes (#2123)
 * Confgen and pragma improvements (#2122)
 * mock-transport: make it inheritable (#2120)
 * Rewrite patternDB test cases based on criterion (#2118)
 * tarball: missing files (#2114)
 * Better python binary detection (#2092)

## Credits

Andras Mitzki, Antal Nemes, Balazs Scheidler, Bernie Harris, Bertrand Jacquin, Gabor Nagy, Gergely Nagy, German Service Network, Janos SZIGETVARI, Kokan, Laszlo Budai, Laszlo Szemere, László Várady, Peter Czanik, Szigetvari Janos, Terez Nemes, Viktor Juhasz, norberttakacs.

