# Contributing

Thanks for helping! Bug reports, game compatibility reports and pull requests
are all welcome.

## Reporting bugs

Use the bug report form. Please include the kernel and firmware versions
(Settings → About), the game and its game code (on the game page), how you
started it (Play, Play with add-ons, NOR) and which add-ons were enabled.

## Making changes

1. Read [Architecture](docs/architecture.md), especially the code placement
   rules if you touch hardware code.
2. Build: `make` (or `tools/docker-build.sh`), see [Building](docs/building.md).
3. Test: `make test`. Add unit tests for logic in `core/` or `patch/`, and a
   simulator scenario for new screens ([Development](docs/development.md)).
4. Format and lint: `make format lint`.
5. For changes to hardware access, loading or patching, test on a cartridge.
6. Add a line to the *Unreleased* section of [CHANGELOG.md](CHANGELOG.md) for
   anything users notice.

Keep pull requests focused, and explain the *why* in the description and in
commit messages.

## Compatibility

Formats on the SD card and in the cartridge's flash are shared with the
original kernel and with existing cartridges ([SD card layout](docs/sd-card-layout.md)).
Changes to them need a very good reason, a migration, and a major version.

## License

By contributing you agree that your contributions are licensed under the
Apache License 2.0, like the rest of the project.
