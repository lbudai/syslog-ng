3.16.1

<!-- Wed, 13 Jun 2018 23:47:01 +0200 -->

## Features

 * telegram destination + urlencode template function (#2085)
 * afsql: added ignore_tns_config sql config option (#2054)
 * Add per-source counters to "src.host" and "src.sender" (#2013)
 * Error reporting on misspelled block args (#1952)

## Bugfixes

 * Fix file source location information in internal logs (#2028)
 * nvtable: deserialize sdata invalid if fixed up (#2067)
 * KW_FILE token in a global namespace (#2076)
 * memleak fixes: appmodel and varargs (#2086)
 * Fix a bug in the old LogMessage deserialization (#2103)
 * confgen: fix reading the output of the program (#2108)
 * secret-storage: add safer mem_zero() (#2049)
 * syslog-ng-ctl: fix shifting of the raw_query_params (#2043)
 * Fix lloc tracking for multi line blockrefs (#2035)
 * DefaultNetworkDrivers: added missing 'else {};' statement to legacy p… (#2027)
 * configure.ac: Fix mixed linking (#2022)
 * eventlog: Extend evtlog.h to use evt_tag_inaddr/inaddr6 functions on … (#2015)
 * mainloop-worker: fix thread_id allocation for more than 32 CPUs (#2008)
 * errno safe evtlog (#1999)
 * floating point warning fixes (#1996)
 * plugin discovery: partial revert to bring back valgrind (#1995)
 * Fix connection close in network sources (#1991)
 * F/delete wildcard file (#1974)
 * dnscache: Disable the DNS cache if use-dns(no) is used (#1923)
 * mock-transport: fix compiler error for gcc 4.4 (#2044)
 * autotools: fixing emitted warnings due to -no-pie detection for gcc 4.4 (#2037)
 * tests/functional/log.py: fix date format (#2036)
 * Dbld: Add fixes to functions.sh (#2034)

## Other changes

 * Do not ship rabbitmq-c with syslog-ng (remove as submodule) (#2052)
 * Add debug message to program source/destination about successful start (#2046)
 * cfg-parser: Report memory exhaustion errors during config parsing (#2033)
 * LogPipe: Improved debug logs (#2032)
 * Dbld coverage (#2031)
 * LogTransportMock enhancement (#2017)
 * ivykis: update to 0.42.3 (#2012)
 * CMake modernization pt. 1 (#2007)
 * Modify the loggen.c license from GPL to LGPL [skip-ci] (#2006)
 * Loggen refactor (#1987)
 * rpm generation process update (#1980)
 * cmake: support for ENABLE_EXTRA_WARNINGS (#2072)
 * Rewrite some unit tests based on criterion (#2026, #2058, #2039, #2018, #2003)
 * Lexer test coverage improvements (#2062)

## Notes to the developers

 * Multitransport (#2057)
 * logreader+logwriter: timeout support (#2056)
 * logreader: add log_reader_close_proto() (#2075)
 * requirements.txt: drop explicit version numbers (#2050)
 * Assert clone exist (#2019)
 * Small java code refactor (#2066)

## Credits

syslog-ng is developed as a community project, and as such it relies
on volunteers, to do the work necessarily to produce syslog-ng.

Reporting bugs, testing changes, writing code or simply providing
feedback are all important contributions, so please if you are a user
of syslog-ng, contribute.

We would like to thank the following people for their contribution:
Andras Mitzki, Antal Nemes, Balazs Scheidler, Gabor Nagy, Gergely Nagy,
German Service Network, Jakub Wilk, Laszlo Budai, Laszlo Varady,
Mehul Prajapati, Norbert Takacs, Peter Czanik, Peter Kokai, Viktor Juhasz
