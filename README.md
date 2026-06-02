# GATIWEB 

A lightweight desktop web browser built with C++ and Qt.

GATIWEB started as a systems programming project to explore browser architecture, networking, caching, memory management, and desktop application development using modern C++.

Instead of focusing on complex web rendering engines, the project focuses on efficient navigation, resource management, tab handling, caching, download management, ad filtering, and a clean desktop experience.

---

## Features

* Multi-tab browsing
* Back / Forward navigation
* Bookmarks
* Download manager
* Private browsing mode
* Ad and tracker blocking
* Session restore
* Multiple themes
* Cache support
* Custom homepage dashboard
* Keyboard shortcuts

---

## Tech Stack

* C++17
* Qt 6
* Qt Widgets
* Qt Network
* PCRE2
* CMake

---

## Why I Built This

I built GATIWEB to gain hands-on experience with:

* Modern C++
* Desktop application architecture
* Networking
* Memory management
* Cache systems
* UI development with Qt
* Software design and project organization

The goal was not to compete with modern browsers, but to understand how browser components work together and to build a complete desktop application from scratch.

---

## Project Structure

```text
GatiWeb/
├── src/
│   ├── main.cpp
│   ├── ui/
│   ├── network/
│   ├── cache/
│   ├── themes/
│   └── browser/
├── resources/
├── CMakeLists.txt
└── README.md
```

---

## Build Requirements

* C++17 Compiler
* Qt 6
* CMake 3.20+
* PCRE2

---

## Build Instructions

```bash
git clone https://github.com/Rutvik-SWE/GatiWeb.git

cd GatiWeb

mkdir build
cd build

cmake -DCMAKE_BUILD_TYPE=Release ..

cmake --build . --config Release
```

### Run

```bash
# Linux
./GatiWeb

# Windows
GatiWeb.exe
```

---

## Testing Guide

### Dashboard Test

* Press `Ctrl + T` multiple times.
* Open the Home page repeatedly.

Expected:

* Dashboard loads correctly.
* UI remains responsive.

---

### Ad Blocking Test

Try:

```text
https://doubleclick.net
https://ads.yahoo.com
```

Expected:

* Navigation is blocked.
* Warning page appears.

---

### Cache Test

Open:

```text
https://en.wikipedia.org/wiki/C%2B%2B
```

Then:

1. Open another Wikipedia page.
2. Press Back.

Expected:

* Previously visited content restores quickly.
* Navigation remains smooth.

---

### Navigation History Test

Open:

```text
https://text.npr.org
```

Navigate through many pages and verify:

* Back/Forward navigation works correctly.
* History management behaves as expected.

---

### Session Restore Test

1. Open several tabs.
2. Close one tab.
3. Press `Ctrl + Shift + T`.

Expected:

* Recently closed tab is restored.

---

### Private Browsing Test

1. Open a private window.
2. Browse several pages.
3. Close the window.

Expected:

* Session data remains isolated.

---

### Download Test

```text
https://speed.hetzner.de/100MB.bin
```

Expected:

* Download starts successfully.
* Progress updates correctly.
* File completes without interruption.

---

## Sample URLs for Testing

```text
https://example.com
https://www.wikipedia.org
https://www.qt.io
https://news.ycombinator.com
https://text.npr.org
https://www.gnu.org
```

---

## Future Improvements

* Better cache management
* Enhanced download controls
* Additional theme options
* Improved content filtering
* Performance profiling tools
* Expanded keyboard shortcuts

---

## Developer

Rutvik Katariya

MCA Student • C++ Developer • Qt Developer • Systems Software Enthusiast
