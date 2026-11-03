# Supply chain and release security

> Treat everything that goes into a build as code you run: pin each input by content, verify it before use, build releases where contributors can't reach the signing keys, and keep the evidence that ties a binary to its source.

## Mental model

A binary is the product of its source **and** of everything that built it:
compilers, the standard library, dependencies, build scripts, CI actions,
container images, package-manager registries. An attacker who changes any of
those changes the binary without touching your code.

```
source ─┐
deps ───┼─▶ build (toolchain, scripts, CI) ─▶ artifact ─▶ signature + provenance ─▶ consumer verifies
tools ──┘         ▲ isolated from                               ▲ digest, builder, inputs
                  │ signing keys
```

Each arrow is a place to pin, verify or isolate. SLSA (Supply-chain Levels for
Software Artifacts, current version 1.2) is a shared vocabulary for how far
you've gone along that chain.[^slsa]

## Pin every input by content

**MUST:** Pin each external input to an identifier that can't change under
you: a commit hash, a content hash or an image digest. A version name or a tag
is a pointer that its owner (or an attacker with their credentials) can move.

| Input | Pinned by | Not by |
|---|---|---|
| CMake `FetchContent` / `ExternalProject` (git) | `GIT_TAG <40-hex commit>` | a tag or branch name |
| `FetchContent` (archive) | `URL ... URL_HASH SHA256=<hash>` | a URL alone |
| vcpkg | `builtin-baseline` (a registry commit) plus `overrides` | the registry's latest |
| Conan 2 | a lockfile (`conan lock create`, then `--lockfile`) with recipe revisions | version ranges alone |
| GitHub Actions | `uses: owner/action@<40-hex commit>  # vX.Y.Z` | `@v4`, `@main` |
| Container images | `image@sha256:<digest>` | `image:tag` |
| Signing keys you download | the published fingerprint, checked before use | "it came over HTTPS" |

CMake's own documentation gives the reason for commit hashes: with a tag or
branch, "checking out a specific commit of the main project doesn't
necessarily pin the whole build". It also notes that `GIT_SHALLOW` works only
with branch and tag names, so a commit-pinned fetch is a full clone.[^cmake]

```cmake
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        063de7e9578f82b369302001269680b4b1553359   # v1.18.0
)
```

**MUST:** Update pins on purpose: a reviewed change that names the old and new
versions, reads the dependency's changelog and security advisories, and runs
the full test suite. An update you didn't look at is the same risk as no pin.

**SHOULD:** Keep an inventory of what you ship (an SBOM). vcpkg writes an SPDX
document (`vcpkg.spdx.json`) for the installed graph in manifest mode. Conan 2
has CycloneDX generation (`conan.tools.sbom`, experimental in 2.12). CMake 4
has an experimental `install(SBOM)`, behind
`CMAKE_EXPERIMENTAL_GENERATE_SBOM`. The two formats are SPDX 3.0 (ISO/IEC
5962) and CycloneDX 1.7 (ECMA-424).[^sbom] An SBOM is what lets you answer
"are we affected?" the day a CVE is announced.

## Build releases where contributors can't reach the keys

**MUST:** Keep signing keys and release credentials away from builds that run
untrusted code. A pull request's build runs the contributor's code, which
includes the build scripts and tests it changes. In SLSA's terms:

- **Build L2** means a hosted platform builds the artifact and signs its
  provenance.
- **Build L3** adds isolation: the user-defined build steps can't influence
  other builds or reach the provenance-signing secrets.[^slsa]

**MUST:** In CI, give each job only the permissions it needs, with read-only
as the default. Don't let a checkout leave a credential behind for later
steps.

**SHOULD:** Make builds reproducible, so anyone can rebuild from the same
inputs and compare digests:

- `-ffile-prefix-map=<build-dir>=.` removes the build path from `__FILE__`
  and the debug info.
- `SOURCE_DATE_EPOCH` fixes `__DATE__` and `__TIME__`.
- `-Wdate-time` finds every use of `__DATE__` and `__TIME__`.

All three work on GCC 15 and Clang 22 (checked locally: with
`SOURCE_DATE_EPOCH=0`, both expand `__DATE__` to `Jan  1 1970`).[^repro]

## Sign, attest and verify

**MUST:** Publish, for each release artifact, its digest, a signature and
provenance (what source, what builder, what parameters). Ways to do it:

- Sigstore's `cosign` signs keylessly: a short-lived certificate tied to the
  CI identity, recorded in a public transparency log.
- GitHub's `actions/attest-build-provenance` produces signed SLSA
  provenance, which `gh attestation verify` checks. GitHub states this meets
  Build L2 alone, and L3 from a reusable workflow. It's available for public
  repositories, and for private ones on Enterprise Cloud.[^sign]

**MUST:** Verify before you use: the consumer (your deploy step, your
packaging, your users) checks the signature, the digest, **and** that the
provenance names the builder and source repository you expect. Unverified
provenance is decoration.

**MUST NOT** read an SBOM, a signature or a reproducible build as "free of
vulnerabilities". They prove where an artifact came from and what it holds.
Whether that code is safe is the job of the rest of this section
([verification](verification.md)).

## Keep the evidence for every release

**SHOULD:** For each release, keep, next to the tag:

- the source commit and the dependency inventory (lockfile, SBOM);
- the toolchain and standard library versions, and the build flags actually
  used ([hardening](hardening.md#check-the-binary-not-the-build-file));
- test, sanitizer, fuzzing and analysis results, with counts;
- artifact digests, signatures and provenance;
- accepted exceptions (a finding left open, a check disabled), each with its
  owner and end date ([policy](policy.md)).

## How this repo does it

The repo ships documentation, not binaries, so it has nothing to sign. Its
build inputs are still pinned and checked:

- **Actions** are pinned to full commit SHAs, workflows run only when
  dispatched by hand, and every workflow is read-only (`permissions:
  contents: read`, `persist-credentials: false`). `tools/check_workflows.py`
  enforces all three.
- **Container images** (`gcc:N`) are pinned by digest.
- **GoogleTest** is pinned to its commit hash.
- **The apt.llvm.org key** is checked against its fingerprint (`6084 F3CF 814B
  57C1 CF12 EFD5 15CF 4D18 AF4F 7421`) before CI trusts it.
- **Release evidence:** every release has notes with the local test counts and
  a table of the consented CI runs (`docs/releases/`). Each run's results JSON
  and metadata are archived in `docs/platform-support/runs/`, so a release's
  claims can be checked after GitHub deletes the logs.

`tests/security/supply-chain_test.cpp` checks the pins.

## See also

- [SLSA specification](https://slsa.dev/spec/v1.2/) ·
  [OpenSSF Scorecard](https://github.com/ossf/scorecard) (checks pinning,
  permissions and more for a repository)
- [Hardening](hardening.md) · [verification](verification.md) ·
  [deployment](deployment.md) · [policy](policy.md)
- [Build and packaging](../guidelines/build-and-packaging.md)
- Tests: [`tests/security/supply-chain_test.cpp`](../../tests/security/supply-chain_test.cpp)

[^slsa]: SLSA v1.2, [Build track basics](https://slsa.dev/spec/v1.2/build-track-basics) (levels L0–L3 and what each requires) and [the specification index](https://slsa.dev/spec/v1.2/), which also defines a Source track.
[^cmake]: CMake 4.4 documentation, [ExternalProject](https://cmake.org/cmake/help/latest/module/ExternalProject.html) `GIT_TAG` and `GIT_SHALLOW` (FetchContent takes the same options); read with `cmake --help-module ExternalProject`.
[^sbom]: vcpkg: [Versioning reference](https://learn.microsoft.com/en-us/vcpkg/users/versioning) and its SBOM output; Conan: [`conan.tools.sbom`](https://docs.conan.io/2/reference/tools/sbom.html), [lockfiles](https://docs.conan.io/2/tutorial/versioning/lockfiles.html); CMake: `cmake --help-command install` (`install(SBOM)`, CMake 4.4); [ISO/IEC 5962](https://www.iso.org/standard/93810.html) (SPDX); [ECMA-424](https://ecma-international.org/publications-and-standards/standards/ecma-424/) (CycloneDX 1.7).
[^repro]: [reproducible-builds.org: SOURCE_DATE_EPOCH](https://reproducible-builds.org/docs/source-date-epoch/) and [build path](https://reproducible-builds.org/docs/build-path/); GCC [`-ffile-prefix-map`](https://gcc.gnu.org/onlinedocs/gcc/Overall-Options.html).
[^sign]: Sigstore, [cosign signing overview](https://docs.sigstore.dev/cosign/signing/overview/); GitHub Docs, [Using artifact attestations](https://docs.github.com/en/actions/how-tos/secure-your-work/use-artifact-attestations/use-artifact-attestations) and [actions/attest-build-provenance](https://github.com/actions/attest-build-provenance).
