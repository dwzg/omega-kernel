# Releasing

Releases are built and published by GitHub Actions
(`.github/workflows/release.yml`) when a version tag is pushed.

## Versioning

The kernel follows [Semantic Versioning](https://semver.org/):

- **major**: changes that break compatibility with saves, settings or NOR
  contents, or remove features;
- **minor**: new features;
- **patch**: bug fixes.

The version is in the `VERSION` file and shown under **About**, together with
the git revision. Pre-releases use a suffix (`2.1.0-rc.1`) and are published
as GitHub pre-releases.

## Steps

1. Make sure CI is green on the main branch and the release was tested on
   hardware.
2. Update `VERSION`.
3. In `CHANGELOG.md`, rename *Unreleased* to the new version with the date
   (`## [2.1.0] - 2026-11-01`) and start a new empty *Unreleased* section.
4. Commit: `git commit -am "Release 2.1.0"`.
5. Tag and push:

   ```sh
   git tag v2.1.0
   git push origin main v2.1.0
   ```

The workflow then:

1. checks that the tag matches `VERSION`;
2. builds the kernel in the devkitARM container;
3. creates a GitHub release with `ezkernel.bin`, a copy named with the version,
   and `SHA256SUMS`;
4. uses the version's section of `CHANGELOG.md` as release notes
   (`tools/changelog_section.py`).

If a step fails, fix the problem, delete the tag
(`git push --delete origin v2.1.0 && git tag -d v2.1.0`) and tag again.
