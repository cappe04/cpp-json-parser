# C++ Json Parser

A lightweight C++ library designed to parse [JSON files](https://www.json.org/json-en.html) into native data types.

> Note: As of now, only the header files with documentation has been added.


#### TODO:
- [x] Documentation
- [ ] License
- [ ] Rest of code
- [ ] Tests

## Usage
The libraries main functianality is split into 3 diffrent classes / structs, all under the namespace `json` and all in the header `json_parser/json.hpp`.

- `json::Parser`    (_class_)

This class is for parsing the JSON file and provieds a log of errors for incorrectly formatted JSON files. The parser will generate a `Document` instance when done.

- `json::Document`  (_class_)

This class manages the parsed JSON in memory and is sole owner of it. The parsed data can be accessed through `View` instances.

- `json::View`      (_struct_)

A `View` is the only way to interact with the parsed JSON data and does not take any ownership of the memory, this means that a `View` cannot be used without the `Document` instance being alive.

The `View` struct allows you to cast data to a specific types and provides access to JSON object fields or array items.

## Examples
Let this be the input file:
```json
{
    "foo": {
        "bar": [1, 2, 3]
    }
}
```

Then you would be able to read values form the array like this:

```cpp 
#include "json_parser/json.hpp" // include header

// ...

Parser parser(/* Input: JSON file as a string */)
parser.parse()
if(!parser.get_status()) { // In case of incorrectly formated input
    std::cout << parser.get_error() << std::endl;
} else {
    Document doc = parser.get_document()
    View view = doc.top_level()
    std::cout << view["foo"]["bar"][0].as<int>() << std::endl; // output: 1
}

// ...
```

## Documentation
The documentation have been written in a doxygen-compatible format and can be viewed either normaly in the source code or through the doxygen generated HTML currently at `docs/html/index.html` (this might be hosted on a website later). If the file doens't exist it can be generated with by running:
```
doxygen Doxyfile
```
> Note: this requires that `doxygen` is installed.
