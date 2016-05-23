# nozomi, for building c++ web applications 

## Docs are still a WIP ##

Nozomi is a web server based on Proxygen from Facebook. It started as a way to
familiarize myself with several of Facebook's open source libraries and 
technologies. It papers over some of the more involved parts of the Proxygen
API and adds simple routing and async HTTP request handling. 

Ever decided "I hate myself, let's write this web app in c++!"? This might be
useful for you.

## Example ##

The server is pretty simple, and consists of a router to decide which handlers
to run.

```
int main() {
  Config config({make_tuple("0.0.0.0", 8080, Config::Protocol::HTTP)}, 2);
  auto router = make_router(
      {}, make_route("/", {Method::GET},
                     [](const HTTPRequest& request) {
                       return HTTPResponse::future(200);
                     }),
      make_route("/user", {Method::POST},
                 [](const HTTPRequest& request) {
                   /* create new user here */
                   return HTTPResponse::future(200, "Created user!");
                 }),
      make_route("/user/{{i}}", {Method::POST},
                 [](const HTTPRequest& request, int64_t userId) {
                   /* fetch user data */
                   return HTTPResponse::future(200, userData.asJson());
                 }),
      make_streaming_route("/{{s:.*}}", {Method::GET}, []() {
        return new StreamingFileHandler("public");
      }));
  Server httpServer(std::move(config), std::move(router));

  auto started = httpServer.start().get();
  string ignore;
  cout << "Press any key to stop" << endl;
  getchar();
  auto stopped = httpServer.stop().get();
  return 0;
}
```

## Base API ##

| Class / Method | Description |
| -------------- | ----------- |
| `Config()` | Creates a configuration object for the server. Basic options are:<br />- List of host/port/protocols to listen on<br />- Number of threads to use for workers |
| `Server()` | Creates a server instance. |
| `Server().start()` | Returns a future that completes once the server is up and listening. |
| `Server().stop()` | Returns a future that completes once the server has shutdown. |
| `make_router()` | Creates a router instance. Takes:<br />- A map of error codes -> request handlers that only take a `const nozomi::HTTPRequest&`<br />- A list of routes.<br />The routes will be evaluated by looking first at static routes in the order presented given to `make_router()`, then by evaluating dynamic routes in the order given to `make_router()`.<br />If an error occurs, the custom error handlers will be invoked, if available, to give a more detailed response. |
| `make_route()` | Creates a route based on a pattern to match the request path against, a list of HTTP methods that this route is valid for, and a request handler. The pattern provided will be validated against the number and type of arguments that the request handler accepts. |
| `make_streaming_route()` | Creates a route as above, only the handler provide should be a method that takes no arguments and returns a heap allocated class instance that implements `nozomi::StreamingHTTPHandler`. |
| `make_static_route()` | Behaves like `make_route`, except the handler only takes a `const nozomi::HTTPRequest&`, and the pattern is not evaluated as a regular expression. |
| `make_static_streaming_route()` | Behaves like `make_stremaing_route()`, except setArgs() on the handler should take no args and the pattern is not evaluated as a regular expression. |
| `StreamingFileHandler` | A streaming handler that takes a base directory, and will return a requested file if it exists in that base directory. The pattern for this handler must extract a string that contains the filename to look for. |
| `HTTPRequest` | A wrapper around proxygen's `HTTPMessage`. It also includes the message body. See the source for API details. |
| `HTTPResponse` | A wrapper around proxygen's `HTTPMessage`, but for sending responses. The static method `HTTPResponse::future()` will return a completed future for any of the various constructors that `HTTPResponse` has. |

I tried to add doxygen style documentation on all of the major classes and
methods, so let me know if the docs are lacking in that area.

## Request handlers ##

A request handler can be any type of callable (std::function, lambda, function 
pointer, etc) that takes a `const nozomi::HTTPRequest&` as the first parameter,
any number of parameters based on the types specified in the corresponding
route, and returns a `folly::Future<nozomi::HTTPResponse>`.

A streaming request handler is a class that implements
`nozomi::StreamingHTTPRequestHandler`. It's more useful for large responses
or if you wanted to stream a client's request body incrementally. `setArgs()` 
must take arguments that match the associated route's pattern (see below).

## Route Patterns ##

For non-static routes, regular expressions are used to match incoming requests.
Request handlers can also be passed extra arguments that are extracted from
a request's path. This behaviour is specified by using special sequences in
a route's pattern.  These are validated by `make_route` and will throw an
exception if the pattern in the route does not match the number and type of
arguments in the request handler.

| Sequence | Argument passed to request handler |
| -------------- | ---------------------------------- |
| `{{i}}` | `int64_t` |
| `{{i?:<regex>}}`| `Optional<int64_t>`. The portion of the path matched by the regex will be consumed whether an integer could be extracted or not. This is useful to consume a '/' if the integer was missing. |
| `{{d}}` | `double` |
| `{{d?:<regex>}}` | `Optional<double>`. The portion of the path matched by the regex will be consumed whether an double could be extracted or not. This is useful to consume a '/' if the double was missing. Scientific notation is not supported. |
| `{{s:<regex>}}` | `std::string` that matches the provided regex |
| `{{s?:<regex1>:<regex2>}}` | `Optional<std::string>` that matches `regex1`. If a string matching the regex is not present, an empty Optional is provided to the request handler. If another pattern is present, but does not match the regex, this route will not be matched at all. `regex2` is consumed if a string matching `regex1` is present, or if the string is complete absent. |

### Examples ###

| Route Pattern | Valid Request | Invalid Request | Handler Signature | Streaming Handler Superclass |
| ------------- | ------------- | --------------- | ----------------- | ---------------------------- |
| `/user/{{i}}` | /user/1234 | /user/ | `Future<HTTPResponse>(const HTTPRequest&, int64_t)` | `StreamingHTTPHandler<int64_t>` |
| `/user/{{s:\w+}}` | /user/john_smith | /user/some-user! | `Future<HTTPResponse>(const HTTPRequest&, std::string)` | `StreamingHTTPHandler<std::string>` |
| `/comments/{{s?:\w+:/}}activity` | /comments/john_smith, /comments/activity | /comments/bad-user!/activity | `Future<HTTPResponse>(const HTTPRequest&, Optional<std::string>)` | `StreamingHTTPHandler<Optional<std::string>>` |

# Building

All development is done using [buck](https://buckbuild.org). You can build it
with `buck build src/...` or `make`.

Some of the libraries can be time consuming to setup, so I have also added a
docker image that has proxygen / folly / wangle / buck / etc installed already.

You can fetch and run it with:

`docker run --rm -it -v "${PWD}:${PWD}" -p "src port:dst port" pjameson/buck-folly-watchman /bin/bash`

Then cd to your working directory, and run the buck command above

I've not tested how well this integrates when you're working on a project that
depends on this library, so the process is subject to change.

# Testing

To run the full set of tests run `make test-all`. To run non-integration tests,
run `buck test test/...` or `make test`

# Contributing

If you want to send me patches, go ahead, but this is currently just a toy
project for me, so I don't know that I'll be super responsive. Let me know if
you've found a use for it, though.
