
The `python-http-header` plugin makes it possible for users to implement HTTP header plugins in Python language. It is built on top of signal-slot mechanism: currently HTTP module defines only one signal, that is `signal_http_header_request` and `python-http-header` plugin implements a python binding for this signal. This means that when the `signal_http_header_request` signal is emitted then the connected slot executes the Python code.

The Python interface is :
```
def get_headers(self, body, headers):
```

It should return string List. The headers that will be appended to the request's header.
 
Original code was written by Ferenc Sipos.

<details>
  <summary>Example config, click to expand!</summary>

```

@version: 3.25

python {
from syslogng import Logger

logger = Logger()

class TestCounter():
    def __init__(self, options):
        self.header = options["header"]
        self.counter = int(options["counter"])
        logger.debug(f"TestCounter class instantiated; options={options}")

    def get_headers(self, body, headers):
        logger.debug(f"get_headers() called, received body={body}, headers={headers}")
       
        response = ["{}: {}".format(self.header, self.counter)]
        self.counter += 1
        return response

    def __del__(self):
        logger.debug("Deleting TestCounter class instance")
};

source s_network {
  network(port(5555));
};

destination d_http {
    http(
        python_http_header(
            class("TestCounter")
            options("header", "X-Test-Python-Counter")
            options("counter", 11)
        )
        url("http://127.0.0.1:8888")
    );
};

log {
    source(s_network);
    destination(d_http);
    flags(flow-control);
};
```
</details>


