# GATIWEB Engine 🌐⚡

**GATIWEB** is a high-performance, native C++ web browser engineered for extreme memory efficiency, zero-latency UI responsiveness, and secure text-based web navigation. 

Built entirely on the Qt6 framework, GATIWEB strips away heavy, modern JavaScript execution engines in favor of lightning-fast raw HTML parsing, making it incredibly lightweight, stable, and secure.

**Developer:** Rutvik Katariya  
**Tech Stack:** C++17, Qt 6 (Core, Gui, Widgets, Network), CMake

---

## 🧠 Core Architectural Optimizations

* **Memory-Efficient Resource Management:** Aggressive pointer cleanup on tab closures and a hard-capped 50-URL session history per tab prevent RAM bloat during massive, long-term browsing sessions.
* **O(1) Regex Ad-Block Shield:** Replaced standard array-iteration loops with a statically compiled, natively optimized PCRE2 Regular Expression matching engine to filter tracking domains with near-zero CPU overhead.
* **Intelligent Network Caching:** Integrated a 150MB local `QNetworkDiskCache` layer that saves static website assets directly to the OS temp directory, drastically reducing latency and network payloads.
* **CPU-Rescued Meta-Refresh Parsing:** Prevents CPU spiking on massive HTML pages by strictly truncating DOM keyword scans to the first 4,096 bytes (the `<head>` container) when handling automated redirections.
* **Hardware-Accelerated UI:** Uses `QVariantAnimation` and `QPropertyAnimation` natively handled by the window manager for buttery-smooth sidebar transitions and layout resizing without locking up the main UI thread.

---

## ✨ Feature Set

* **Zero-Latency Native Dashboard:** A hardcoded C++ local start page utilizing a fluid CSS Grid layout, completely bypassing network I/O for instant startup.
* **Live Theme Synchronization:** A centralized CSS stylesheet runtime pipeline featuring 20 culturally rich Bharatiya accent color profiles (e.g., Kesari, Peacock, Marigold).
* **Tab Rescue System:** Accidental closure safety protection (`Ctrl+Shift+T`) backed by an optimized 15-item memory stack.
* **Incognito Isolation:** Dedicated private window allocations with enforced DNT (Do Not Track) tracking header bits.

---

## 🛠️ Build & Run Instructions

This project uses CMake. Ensure you have a C++17 compatible compiler (GCC/Clang/MSVC) and Qt6 installed on your system.

```bash
# 1. Clone the workspace files
git clone [https://github.com/yourusername/gatiweb.git](https://github.com/yourusername/gatiweb.git)
cd gatiweb

# 2. Setup a separate build folder
mkdir build && cd build

# 3. Generate Makefiles and Build with Link-Time Optimization (LTO)
cmake ..
cmake --build . --config Release

# 4. Run the engine binary
./GatiWeb    # Linux / macOS
GatiWeb.exe  # Windows MinGW/MSVC