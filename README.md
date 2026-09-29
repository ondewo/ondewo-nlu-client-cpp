<div align="center">
  <table>
    <tr>
      <td>
        <a href="https://ondewo.com/">
            <img width="400px" src="https://raw.githubusercontent.com/ondewo/ondewo-logos/master/ondewo_we_automate_your_phone_calls.png"/>
        </a>
      </td>
    </tr>
    <tr>
       <td align="center">
          <a href="https://www.linkedin.com/company/ondewo"><img width="40px" src="https://cdn-icons-png.flaticon.com/512/3536/3536505.png"></a>
          <a href="https://www.facebook.com/ondewo"><img width="40px" src="https://cdn-icons-png.flaticon.com/512/733/733547.png"></a>
          <a href="https://twitter.com/ondewo"><img width="40px" src="https://cdn-icons-png.flaticon.com/512/733/733579.png"></a>
          <a href="https://www.instagram.com/ondewo.ai/"><img width="40px" src="https://cdn-icons-png.flaticon.com/512/174/174855.png"></a>
       </td>
    </tr>
  </table>
  <h1 align="center">
    ONDEWO NLU Client C++
  </h1>
</div>

## Overview

`ondewo-nlu-client-cpp` is the C++ gRPC client library for the
[ONDEWO NLU API](https://github.com/ondewo/ondewo-nlu-api) - ONDEWO's Natural Language Understanding service. It is a
compiled version of that API, generated with the
[ONDEWO PROTO COMPILER](https://github.com/ondewo/ondewo-proto-compiler). The API
[documentation](https://ondewo.github.io) describes every service and message in detail.

ONDEWO APIs use [Protocol Buffers](https://github.com/google/protobuf) version 3 (proto3) as their Interface
Definition Language (IDL) to define the API interface and the structure of the payload messages. The same
interface definition is used for the gRPC versions of the API in all languages.

There is **no hand-written code** in this repository. Everything it ships is generated:

| Path                            | What it is                                                               |
| ------------------------------- | ------------------------------------------------------------------------ |
| `api/`                          | the generated stubs - `*.pb.h` / `*.pb.cc` and `*.grpc.pb.h` / `*.grpc.pb.cc` |
| `public-api.h`                  | umbrella header that `#include`s every generated header                  |
| `CMakeLists.txt`                | builds the stubs into a static library and installs a CMake package      |
| `ondewo-client-config.cmake.in` | template for the installed `<library>-config.cmake`                      |
| `ondewo-nlu-api/`                   | submodule - the `.proto` source of truth                                 |
| `ondewo-proto-compiler/`        | submodule - the code generator, pinned to a release tag                  |

The generated sources are committed deliberately: C++ has no package registry, so a git tag is this client's
distribution channel and a plain `git clone` has to yield a buildable CMake project.

## Requirements

To **consume** the library:

- CMake >= 3.22 and a C++17 compiler
- `libprotobuf-dev` and `libgrpc++-dev` (plus `libgrpc-dev`, which ships `gRPCConfig.cmake`)

protobuf C++ gives
[no cross-version guarantee](https://protobuf.dev/support/cross-version-runtime-guarantee/) between generated
code and runtime - they must match **exactly**. The stubs in `api/` are generated against the protobuf and gRPC
versions pinned by `ondewo-proto-compiler/cpp/Dockerfile` (`ARG PROTOBUF_VERSION` / `ARG GRPC_VERSION`), which
are the versions Debian stable and Ubuntu 24.04 ship. If your distribution ships a different protobuf, the
committed stubs do not compile against it, and `make build` does not change that: it always regenerates them with
the compiler image's pinned `protoc` and compiles them in the `Dockerfile.utils` image (Ubuntu 24.04) against the
same pinned versions.

To **regenerate, build, test or release** the client from this repository you need only `make`, `git`, Docker
and `perl`: code generation runs in the `ondewo-cpp-proto-compiler` image, and CMake, protobuf/gRPC,
GoogleTest and `gh` come from the `Dockerfile.utils` image (`ondewo-nlu-client-utils-cpp:<version>`, built by
`make build_utils_docker_image`), which runs as your user with the repository mounted.

## Setup

### Install the published release archive

C++ has no package registry, so every [GitHub release](https://github.com/ondewo/ondewo-nlu-client-cpp/releases)
carries the built package as an asset: `ondewo_nlu_client-<version>-<platform>.tar.gz`, plus a
`.sha256` next to it. The archive is the CMake install tree - the static library, the public headers and the
package-config files - so consuming it is one `find_package`, with no compiler run and no Docker.

```shell
version=7.2.0
platform=linux-x86_64     ## uname -s | tr A-Z a-z, then uname -m
archive=ondewo_nlu_client-${version}-${platform}.tar.gz

gh release download "${version}" --repo ondewo/ondewo-nlu-client-cpp --pattern "${archive}*"
sha256sum -c "${archive}.sha256"          ## macOS: shasum -a 256 -c
tar -xzf "${archive}" -C /opt             ## anywhere - the package is relocatable
```

Then point CMake at the extracted directory and consume it exactly as you would a system-installed package:

```cmake
find_package(ondewo_nlu_client CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE ondewo::ondewo_nlu_client)
```

```shell
cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/ondewo_nlu_client-${version}-${platform}
```

`#include "public-api.h"` then reaches the whole API surface; the exported target carries the include
directory and re-finds `Protobuf` and `gRPC` on your machine.

Two things the archive does **not** do, both by construction:

- It is a **binary** artifact for one platform, and protobuf gives
  [no cross-version guarantee](https://protobuf.dev/support/cross-version-runtime-guarantee/) between generated
  code and runtime. `PACKAGE-INFO.txt` inside the archive records the platform, the `protoc`, the
  `libprotobuf`/`libgrpc++` and the compiler it was built with - if your protobuf differs, build from source
  with one of the two options below instead.
- It is not registered with vcpkg, Conan or any other package manager, and there is nothing to `install` from a
  registry. The release asset and the git tag are the whole distribution story.

### Build from source

Using CMake `FetchContent` - no Docker, no install step:

```cmake
include(FetchContent)
# The library, target and package name. Set it BEFORE MakeAvailable: the built-in default is the
# generic `ondewo_grpc_client`, which collides if you pull in more than one ONDEWO C++ client.
set(ONDEWO_LIBRARY_NAME ondewo_nlu_client CACHE STRING "" FORCE)
FetchContent_Declare(
  ondewo_nlu_client
  GIT_REPOSITORY https://github.com/ondewo/ondewo-nlu-client-cpp.git
  GIT_TAG        7.2.0)
FetchContent_MakeAvailable(ondewo_nlu_client)

# Note the UNqualified target name: the `ondewo::` namespace is created by the install/export step
# below, so it does not exist when the project is pulled in with add_subdirectory/FetchContent.
target_link_libraries(my_app PRIVATE ondewo_nlu_client)
```

`FetchContent_MakeAvailable` runs `add_subdirectory`, so this client's `install()` rules become part of your
project's install as well. Pass `EXCLUDE_FROM_ALL` to `FetchContent_Declare` (CMake >= 3.28) if that is not
what you want.

Using a system-wide install:

```shell
git clone https://github.com/ondewo/ondewo-nlu-client-cpp.git
cd ondewo-nlu-client-cpp
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DONDEWO_LIBRARY_NAME=ondewo_nlu_client \
  -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build --parallel
sudo cmake --install build
```

Then, in the consuming project:

```cmake
find_package(ondewo_nlu_client CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE ondewo::ondewo_nlu_client)
```

### Develop on this repository

Setting up a development checkout of this repository itself:

```shell
git clone https://github.com/ondewo/ondewo-nlu-client-cpp.git   ## Clone repository
cd ondewo-nlu-client-cpp                                        ## Change into repo directory
make setup_developer_environment_locally              ## Submodules + pre-commit hooks
make build                                            ## Regenerate the stubs and build the library
make test                                             ## Verify the result
```

## Usage

Every message and service stub is reachable through the umbrella header. Include it, or include the single
generated header you need (`api/ondewo/nlu/<file>.grpc.pb.h`) to keep compile times down.

Requests are authenticated with a bearer token passed as gRPC call metadata. The key is the lowercase
`authorization` - gRPC rejects metadata keys containing uppercase characters.

```cpp
#include <cstdlib>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "public-api.h"

int main() {
  // Use grpc::InsecureChannelCredentials() only against a local, unencrypted deployment.
  auto channel = grpc::CreateChannel("nlu.ondewo.com:443", grpc::SslCredentials({}));

  // Replace Services/Request/Response with the service you need - see the API documentation.
  auto stub = ondewo::nlu::Services::NewStub(channel);

  grpc::ClientContext context;
  context.AddMetadata("authorization", "Bearer " + std::string(getenv("ONDEWO_TOKEN")));

  ondewo::nlu::Request request;
  ondewo::nlu::Response response;

  const grpc::Status status = stub->SomeRpc(&context, request, &response);
  if (!status.ok()) {
    return 1;
  }
  return 0;
}
```

## Regenerating the stubs

Generation runs entirely inside the `ondewo-cpp-proto-compiler` docker image, so no protoc, no gRPC plugin and
no network access are needed on the host once the image exists.

```shell
make build
```

That is the whole pipeline, and each step is also a target of its own:

1. `make update_submodules` - `git submodule update --init --recursive`
2. `make checkout_defined_submodule_versions` - check out the tags pinned in the Makefile's Variables chapter
3. `make build_compiler` - build the image from the `ondewo-proto-compiler` submodule
4. `make generate_ondewo_protos` - run the image over the `.proto` tree
5. `make build_utils_docker_image` - build the toolchain image from `Dockerfile.utils`
6. `make build_library_via_docker_image` - configure, compile and install the library with CMake inside that
   image (`make build_library` does the same natively, on a machine that has the toolchain installed)

Step 4 is the contract with the compiler image, and it is a single `docker run`:

```shell
docker run --rm \
  --user $(id -u):$(id -g) \
  -e HOME=/tmp \
  -e TEMP_SRC_DIRECTORY=/tmp/ondewo-src \
  -e BUILD_DIRECTORY=/tmp/ondewo-build \
  -e INSTALL_DIRECTORY=/tmp/ondewo-install \
  -v $(pwd):/input-volume \
  -v $(pwd):/output-volume \
  ondewo-cpp-proto-compiler ondewo-nlu-api ondewo ondewo_nlu_client
```

The three positional arguments are the proto root relative to the input volume, the sub-directory of that root
whose protos are the compilation entry points, and the CMake target / package / archive name. Imports are
resolved transitively, so `google/` must not be listed - the well-known types already inside `libprotobuf` are
excluded on purpose, since generating them again would break the link on duplicate symbols.

Notes on the volumes:

- The image copies the input volume into an internal scratch directory and compiles there, so the mounted
  `.proto` sources are never modified.
- In the output volume it deletes only what it owns - `api/`, `include/ondewo_nlu_client/`,
  `lib/libondewo_nlu_client.a` and `lib/cmake/ondewo_nlu_client/` - so a proto that was renamed or
  deleted upstream leaves no orphan header behind, and nothing else in the repository is touched.
  `public-api.h` is wholly generated and is simply overwritten.
- `CMakeLists.txt` and `ondewo-client-config.cmake.in` are never overwritten once they exist. The versions the
  compiler image ships are written to `api/CMakeLists.txt.generated` and
  `api/ondewo-client-config.cmake.in.generated` instead (both gitignored), so you can diff and adopt them after
  a compiler bump.
- The container runs as your user, so everything it writes is owned by you and nothing needs `sudo`
  afterwards. The image's scripts default their scratch, build and install trees to the root-owned
  `/image-data`; the three `*_DIRECTORY` variables move them to `/tmp` inside the container.

There is no `-it` anywhere in the codegen invocation - it breaks every non-interactive caller with
`cannot attach stdin to a TTY-enabled container because stdin is not a terminal`. Keep it only for the
interactive `--entrypoint /bin/bash` debug command.

## Testing

```shell
make test
```

`make test` runs `check_stubs` and `check_build` on the host (they only read files) and then `unit_test`,
`smoke_test` and `publish_dry_run` inside the utils image, against the library `make build` installed. Called on
their own, those three run natively and need CMake, protobuf/gRPC and GoogleTest on the machine. CI
(`.github/workflows/ci.yml`) runs `check_stubs`, `build_library`, `coverage` and `smoke_test` natively on
Ubuntu 24.04, plus the pre-commit hooks, on every push and pull request.

- `check_build` asserts that every `.proto` under `ondewo-nlu-api/ondewo` produced a matching `.pb.h`.
- `smoke_test` writes a throwaway project that does `find_package(ondewo_nlu_client CONFIG REQUIRED)`,
  includes `public-api.h` and links `ondewo::ondewo_nlu_client`, then compiles and runs it. That is the
  one check that proves the exported CMake package, the umbrella header and the link line all work together
  the way a downstream application uses them.
- `publish_dry_run` builds the release archive and then consumes it: it extracts the tarball into a throwaway
  tree and builds `tests/package-consume` against nothing but that - `find_package()` is asserted to resolve
  inside the extracted archive, and the whole static library is forced into the link, so an incomplete archive
  fails here rather than after it has been published. It needs no credentials and uploads nothing, and every
  release runs it before it pushes anything.

## Release

See `RELEASE.md` for the release history and the Makefile's Release, Package and GitHub chapters for the
automation. A release runs entirely on the releasing machine with `make ondewo_release` - no CI workflow builds or
publishes a release or holds a credential; `.github/workflows/ci.yml` only tests and lints. There is no package registry
for C++, so a release is a git tag plus a GitHub release carrying the built package. It is normally started from
[ondewo-nlu-api](https://github.com/ondewo/ondewo-nlu-api) with `make release_cpp_client` (or
`make release_all_clients`), which sets the version in this Makefile, adds the `RELEASE.md` entry and then runs
`make ondewo_release` here:

1. `make update_readme_version` stamps the version into the two install snippets above, and `make spc` refuses
   to go on when `release/<version>` or the tag already exists.
2. The only credential, `GITHUB_GH_TOKEN`, is read from `account_github.env` in the `ondewo-devops-accounts`
   repository. It is kept nowhere else. The repository is cloned into this checkout (gitignored) for the run
   and removed after a successful one - delete `ondewo-devops-accounts/` yourself after a failed run.
3. `make release` checks that the token is set and that GitHub accepts it with push access to this repository
   (`make validate_release_credentials`, read-only), and that `RELEASE.md` has an entry for the version.
4. `make build` and `make test` regenerate, compile and test the client and build and consume the release
   archive (`make publish_dry_run`), all in Docker, before anything is pushed.
5. The release commit is pushed to `master`, followed by the `release/<version>` branch and the tag.
6. Last, `make push_to_gh`, inside the utils image, creates the GitHub release with the archive and its
   `.sha256` attached. `gh release create` uploads the files to a draft and publishes it only once both are
   attached, so a published release always carries its package.

The host needs only `make`, `git` (with SSH access to GitHub and Bitbucket), Docker and `perl`. The release also
runs the pre-commit hooks when `pre-commit` is installed, and skips them otherwise.

| Target                              | What it does                                                               |
| ----------------------------------- | -------------------------------------------------------------------------- |
| `make build_package`                | stages the CMake install tree into `dist/<library>-<version>-<platform>.tar.gz` + `.sha256` |
| `make verify_package`               | extracts that archive and consumes it from an unrelated CMake project      |
| `make publish_dry_run`              | both of the above - the credential-free packaging gate that `make test` runs |
| `make release_to_github_via_docker` | only step 6 (the GitHub release with its files), inside the utils image    |

If a release stops after the tag was pushed, `spc` refuses to run it again. Finish it from a checkout of the tag
with the token from the devops repository - loaded in a subshell, so it does not stay in your shell's environment.
`gh` normally deletes its draft when an upload fails; if a draft release for the version is still listed on GitHub,
delete it first:

```shell
make build test
make clone_devops_accounts
(set -a; . ./ondewo-devops-accounts/account_github.env; set +a; make release_to_github_via_docker)
rm -rf ondewo-devops-accounts
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Commit messages follow
[Conventional Commits](https://www.conventionalcommits.org/) (`feat: …`, `fix(scope): …`, `docs: …`); do **not**
write the JIRA ticket prefix by hand - the `giticket` pre-commit hook reads it from the branch name and prepends
`[<ticket>]` on commit.

## License

Apache License 2.0 — see [LICENSE](LICENSE).

[//]: # (Generated with the ONDEWO proto compiler - see https://github.com/ondewo/ondewo-proto-compiler)
