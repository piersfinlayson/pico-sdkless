# Changelog

pico-sdkless follows [semantic versioning](https://semver.org).  Releases
before 1.0.0 may change the API in a minor version.

## 0.1.1 - unreleased

- The example reset handler in [reset.c](examples/common/src/reset.c) copied a
  quarter of the `.ramfunc` section into RAM, so a routine placed there ran
  into whatever RAM held.
