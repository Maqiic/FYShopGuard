# FYShopGuard

**FYShopGuard** is an experimental iOS runtime library that filters advertising and shopping-related content from supported short-form video feeds.

It uses multiple on-device signals to identify promotional posts, including ad flags, product metadata, shopping anchors, and commerce-related model properties.

> [!IMPORTANT]
> FYShopGuard relies on undocumented application internals. Compatibility depends on the host application's version and implementation. Filtering behavior may change after app updates.

## Features

- **Multi-signal detection:** Checks ad metadata, product references, shopping anchors, and commerce-related fields rather than relying on a single advertising flag.
- **Feed-level filtering:** Attempts to remove matching entries before they are presented in the feed.
- **Display-time fallback:** Attempts to advance past matching entries that reach the visible feed.
- **On-device operation:** Does not require a proxy, network interception, or an external service.
- **Standalone library:** Builds as an ARM64 Mach-O dynamic library (`FYShopGuard.dylib`).

## Requirements

For local builds:

- macOS with Xcode and an iPhoneOS SDK
- [Theos](https://theos.dev/)
- `ldid` available on your `PATH`
- GNU Make

The build targets **ARM64** with an **iOS 14.0 deployment target**. This is a build setting, not a guarantee of compatibility with every iOS or host application version.

## Building

### Option 1: GitHub Actions

1. Fork or clone the repository to your GitHub account.
2. Open the repository's **Actions** tab.
3. Select the build workflow and choose **Run workflow**.
4. Open the successful workflow run and download the **`FYShopGuard-arm64-dylib`** artifact.
5. Extract **`FYShopGuard.dylib`** from the artifact archive.

The workflow builds the library on a macOS runner and publishes the compiled file as a workflow artifact.

### Option 2: Local build

After installing the required tools, run:

```sh
export THEOS="$HOME/theos"
make clean
make
```

To locate the compiled library:

```sh
find .theos -name 'FYShopGuard.dylib'
```

If `ldid` is missing on macOS, install it with:

```sh
brew install ldid
```

## Integration

`FYShopGuard.dylib` is a runtime library, **not** a standalone application. It must be loaded into a compatible iOS client through an appropriate library-loading and code-signing workflow.

Keep a working backup of your original installation. Additional runtime modifications may interfere with the same methods, so test compatibility carefully.

## How it works

1. **Model inspection:** Checks feed models and available source metadata for explicit advertising and shopping indicators.
2. **Collection filtering:** Removes matching models from supported feed collections when possible.
3. **Playback fallback:** When a matching item is already displayed, attempts to move to the next item.

The filter operates locally and does not modify user accounts, payment information, or server-side content.

## Limitations

- Detection is heuristic; some promotional entries may remain visible.
- Regular posts containing product links may also be filtered.
- Internal class names, selectors, and data structures may change between application releases.
- Conflicts with other modifications that hook the same runtime methods are possible.
- A successful build does not by itself guarantee runtime compatibility.

## Project structure

```text
FYShopGuard/
├── .github/
│   └── workflows/
│       └── build.yml      # Automated build workflow
├── Makefile               # Theos build configuration
├── Tweak.c                # Runtime detection and filtering logic
└── README.md
```

## Contributing

Issues and pull requests are welcome. For reproducible bug reports, include the relevant iOS and host application versions, a description of the observed behavior, and non-sensitive diagnostic information.

Do not share private certificates, credentials, personal account data, or proprietary application files in issues or pull requests.

## Disclaimer

This project is provided as-is, without warranties. Use it responsibly and in accordance with applicable laws and the terms of any software you modify.
