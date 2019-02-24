# aupng - [upng](https://github.com/elanthis/upng) with [APNG](https://wiki.mozilla.org/APNG_Specification) support

## Why?

upng is currently used by [RebbleOS](https://github.com/ginge/FreeRTOS-Pebble) which strives to be API-compatible to the old PebbleOS including `gbitmap_sequence`, which uses APNG images. Thus this project is for developing the basic APNG support and then implement the `gbitmap_sequence` API.

## How?

You will need cmake to build this project. Use these commands to configure, build and run the tests:

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
ctest .
```
