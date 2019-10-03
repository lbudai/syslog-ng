3.24.1
======

## Highlights

 * Add a new template function called `format-flat-json()`, which generates
   flattened json output. This is useful for destinations, where the json
   parsing does not handle nested json format. (#2890)
 * Add ISO 8601 compliant week numbering. Use it with the `${ISOWEEK}` macro
   and the timestamp prefixes `P_, S_, R_, C_` (#2878)
 * Send Server Name Identification (SNI) information with `transport(tls)`.
   Enable it by setting the `sli(yes)` option in the `tls` block in your
   `destination`. (#2930)

## Features

 * Timezone related `rewrite` rules (#2818)
 * `templates`: change the `$LOGHOST` macro to honour `use-fqdn()` (#2894)
 * `syslog-ng-sysconfdir` define (#2932)
 * Dqtool assign to persist (#2872)

## Bugfixes

 * Fix backtick substitution (#2906, #2909)
 * Fix SCL block parameter substitution  (#2901)
 * `logthrsource/fetcher`: crash after failed reload (#2907)
 * `logsource`: add explicit (un)initialized state to `WindowSizeCounter` (#2893, #2895)
 * Invalidate `LEGACY_MSGHDR` when `PID` or `PROGRAM` is `unset` (#2896)
 * on 32bit platform `diskq` ftruncate could fail due to size 32/64 interface (#2892)
 * Fix zoneinfo parser (#2898)
 * `afinet`: fix multicast ip detection (#2905)
 * `host-resolve`: try `getaddrinfo()` with different flags (#2933)
 * Fix `wildcard-file()` option validation (#2922)
 * `kafka-c`: Fix multiple memleaks (#2944)

## Other changes

 * `geoip`: remove deprecated module, `geoip2` database location detection (#2780)
 * `persist-state`: add `g_assert` to `persist_state_map_entry` (#2945)
 * Fix persist versions (#2914)
 * Handle legacy `sql` qfile names (#2949)
 * `afsocket-dest`: rename old persist entries (#2928)
 * Timeutils refactor and performance improvements (#2935)
 * various refactor, build issue fixes (#2902)

## Notes to the developers

 * `LightRunWithStrace`: Run syslog-ng behind strace (#2921)
 * `LightVerboseLogOnError`: Increase default pytest verbosity on error (#2919)
 * Dbld image caching (#2858)
 * Dbld gradle caching (#2857)
 * `logreader,logsource`: move `scratch-buffer` mark and reclaim into `LogSource` (#2903)

## Credits

syslog-ng is developed as a community project, and as such it relies
on volunteers, to do the work necessarily to produce syslog-ng.

Reporting bugs, testing changes, writing code or simply providing
feedback are all important contributions, so please if you are a user
of syslog-ng, contribute.

We would like to thank the following people for their contribution:

Andras Mitzki, Antal Nemes, Attila Szakacs, Balazs Scheidler, Bertrand Jacquin,
Gabor Nagy, Henrik Grindal Bakken, Kerin Millar, kjhee43, Laszlo Budai,
Laszlo Szemere, László Várady, Péter Kókai, Raghunath Adhyapak, Zoltan Pallagi.
