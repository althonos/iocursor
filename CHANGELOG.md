# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).


## [Unreleased]

[Unreleased]: https://github.com/althonos/iocursor/compare/v0.1.4...HEAD


## [v0.1.4] - 2022-11-09

[v0.1.4]: https://github.com/althonos/iocursor/compare/v0.1.3...v0.1.4

### Added
- Wheels for Windows platform.


## [v0.1.3] - 2022-11-09

[v0.1.3]: https://github.com/althonos/iocursor/compare/v0.1.2...v0.1.3

### Added
- Wheels for Python 3.11.

### Fixed
- `void*` pointers preventing build on Windows ([#1](https://github.com/althonos/iocursor/issues/1)).
- Missing terminator in `Cursor.__exit__` keyword argument list causing issues on Windows.



## [v0.1.2] - 2021-12-14

[v0.1.2]: https://github.com/althonos/iocursor/compare/v0.1.1...v0.1.2

### Added
- Wheels for Python 3.10.

### Fixed
- Broken docstrings for `Cursor.write`, `Cursor.tell` and `Cursor.readlines`.


## [v0.1.1] - 2021-02-10

[v0.1.1]: https://github.com/althonos/iocursor/compare/v0.1.0...v0.1.1

### Added
- Missing documentation for `Cursor` methods.
- Contribution guidelines in `CONTRIBUTING.md` file.

### Fixed
- Wheels not being built for Python 3.9.


## [v0.1.0] - 2021-02-06

[v0.1.0]: https://github.com/althonos/iocursor/compare/f47f405...v0.1.0

Initial release.
