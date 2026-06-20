# Phase A — Industrialization Scaffolding Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the missing engineering infrastructure to D-Router — formatting, linting, a JUCE-native unit-test target with the first regression tests, project memory, and CI — without changing app behavior.

**Architecture:** Pure additive scaffolding. A new `dcorerouter_tests` console target (JUCE `UnitTest`) compiles a small set of existing Source/ `.cpp` files plus new test files and runs under `ctest`. Config files (`.clang-format`, `.clang-tidy`, `.editorconfig`) codify the existing JUCE house style. CI runs build + tests + format-check-on-changed-files on a macOS runner. No Source/ behavior changes except the `run.sh` bundle-name bug fix.

**Tech Stack:** C++20, JUCE 8.0.4 (FetchContent), CMake ≥ 3.22, Ninja, JUCE `UnitTest` framework, GitHub Actions (macOS), clang-format, clang-tidy.

## Global Constraints

- Platform: macOS 12+ only. Builds via CMake + Ninja; JUCE fetched at 8.0.4 (CMakeLists.txt).
- Namespace `dcr::` for all project code.
- C++20 (`set(CMAKE_CXX_STANDARD 20)`).
- **DSP is out of scope** — do not add tests for or modify `Source/DSP/**`.
- Match existing JUCE house style (Allman braces, space-before-paren, `PointerAlignment: Left`). Do NOT reformat existing files.
- App target is `dcorerouter`, product/bundle name is `D-Router` (`D-Router.app`), process name `D-Router`.
- Every task ends in a commit on branch `overhaul/phase-a-scaffolding`.
- Engine/Routing/audio-path behavior is frozen in Phase A.

---

### Task 1: Housekeeping — fix `run.sh`, add `.editorconfig`

**Files:**
- Modify: `run.sh:29`, `run.sh:44`, `run.sh:47`
- Create: `.editorconfig`

**Interfaces:**
- Consumes: nothing.
- Produces: a `run.sh` that actually finds/launches/kills the real bundle; `.editorconfig` consumed by editors only.

- [ ] **Step 1: Fix the bundle path in `run.sh`**

Change line 44 from:
```bash
APP_BUNDLE="$BUILD_DIR/dcorerouter_artefacts/$CONFIG/dcorerouter.app"
```
to:
```bash
APP_BUNDLE="$BUILD_DIR/dcorerouter_artefacts/$CONFIG/D-Router.app"
```

- [ ] **Step 2: Fix the `killall` process name (two sites)**

Change both `killall dcorerouter` occurrences (lines 29 and 47) to:
```bash
killall D-Router 2>/dev/null || true
```
Leave the echo/log strings (`[dcorerouter] ...`) as-is — only the `killall` *target* is wrong.

- [ ] **Step 3: Create `.editorconfig`**

```ini
root = true

[*]
charset = utf-8
end_of_line = lf
insert_final_newline = true
trim_trailing_whitespace = true

[*.{cpp,h,mm,hpp}]
indent_style = space
indent_size = 4

[*.{yml,yaml}]
indent_style = space
indent_size = 2
```

- [ ] **Step 4: Sanity-check the script parses**

Run: `bash -n run.sh`
Expected: no output, exit 0 (syntax OK).

- [ ] **Step 5: Commit**

```bash
git add run.sh .editorconfig
git commit -m "chore: fix run.sh bundle/process name, add .editorconfig"
```

---

### Task 2: `.clang-format` matching existing JUCE style

**Files:**
- Create: `.clang-format`

**Interfaces:**
- Consumes: nothing.
- Produces: a format config that yields near-zero diff on the existing tree, used by Task 9's CI check.

- [ ] **Step 1: Create `.clang-format`**

`ColumnLimit: 0` is deliberate — it stops clang-format reflowing the hand-aligned long lines, which is the main source of churn.

```yaml
---
Language: Cpp
BasedOnStyle: LLVM
Standard: c++20
ColumnLimit: 0
IndentWidth: 4
TabWidth: 4
UseTab: Never
AccessModifierOffset: -4
NamespaceIndentation: None
PointerAlignment: Left
BreakBeforeBraces: Allman
SpaceBeforeParens: Always
SpaceBeforeCpp11BracedList: true
SpacesInAngles: Never
AlignAfterOpenBracket: Align
AlignConsecutiveAssignments: Consecutive
AlignConsecutiveDeclarations: Consecutive
AllowShortFunctionsOnASingleLine: Inline
AllowShortBlocksOnASingleLine: Always
AllowShortIfStatementsOnASingleLine: Never
KeepEmptyLinesAtTheStartOfBlocks: false
MaxEmptyLinesToKeep: 2
SortIncludes: false
...
```

- [ ] **Step 2: Confirm a clang-format is available and its version**

Run: `clang-format --version`
Expected: a version string (Apple clang-format or LLVM ≥ 12). If missing: `brew install clang-format`.

- [ ] **Step 3: Measure the diff on three representative files**

Run:
```bash
for f in Source/Routing/RoutingMatrix.cpp Source/Engine/RingBuffer.h Source/Main.cpp; do
  echo "== $f =="; clang-format "$f" | diff -u "$f" - | head -40
done
```
Expected: only small, local differences (or none). If clang-format wants to reflow whole blocks or strip the column alignment, adjust the config (most commonly toggle `AlignConsecutiveAssignments`/`AlignConsecutiveDeclarations` to `None`, or `AllowShortBlocksOnASingleLine: Never`) and re-run until the diff is minimal and clearly cosmetic. Do not edit the source files.

- [ ] **Step 4: Commit**

```bash
git add .clang-format
git commit -m "chore: add .clang-format matching existing JUCE house style"
```

---

### Task 3: `.clang-tidy` (curated, warnings-only)

**Files:**
- Create: `.clang-tidy`

**Interfaces:**
- Consumes: nothing.
- Produces: a curated check set used by Task 9's non-blocking lint job.

- [ ] **Step 1: Create `.clang-tidy`**

```yaml
---
# Curated, high-signal checks only. Warnings, not errors, in Phase A.
# DSP is out of scope; CI runs tidy on non-DSP Source files only.
Checks: >
  -*,
  bugprone-*,
  -bugprone-easily-swappable-parameters,
  -bugprone-narrowing-conversions,
  performance-*,
  misc-misplaced-const,
  misc-redundant-expression,
  modernize-use-override,
  modernize-use-nullptr,
  cppcoreguidelines-pro-type-member-init,
  cppcoreguidelines-slicing
WarningsAsErrors: ''
HeaderFilterRegex: 'Source/(Engine|Routing|UI|Persistence|Diagnostics)/.*'
FormatStyle: file
```

- [ ] **Step 2: Verify the YAML parses**

Run: `clang-tidy --dump-config --config-file=.clang-tidy >/dev/null && echo OK`
Expected: `OK` (no parse error). If `clang-tidy` is absent: `brew install llvm` (it ships `clang-tidy`).

- [ ] **Step 3: Commit**

```bash
git add .clang-tidy
git commit -m "chore: add curated .clang-tidy (warnings-only, non-DSP)"
```

---

### Task 4: Test target + runner + first FloatRingBuffer test

This is the foundational task: it stands up `dcorerouter_tests`, proves the harness compiles/links/runs, and lands the first regression test. Fold all harness setup here.

**Files:**
- Create: `tests/CMakeLists.txt`
- Create: `tests/TestMain.cpp`
- Create: `tests/RingBufferTests.cpp`
- Modify: `CMakeLists.txt` (append a test-target hook at the end)

**Interfaces:**
- Consumes: `dcr::FloatRingBuffer` from `Source/Engine/RingBuffer.h` (header-only): `FloatRingBuffer(size_t)`, `size_t write(const float*, size_t)`, `size_t read(float*, size_t)`, `size_t readAvailable() const`, `size_t writeAvailable() const`, `size_t capacity() const`, `void clear()`.
- Produces: a `dcorerouter_tests` CMake target registered with `ctest`; a `juce::UnitTest`-based pattern (static test instances + a runner `main`) that Tasks 5–7 extend by adding one `.cpp` per area to `tests/CMakeLists.txt`.

- [ ] **Step 1: Write the test runner `tests/TestMain.cpp`**

```cpp
// Console runner for the D-Router unit tests. Runs every statically-registered
// juce::UnitTest and returns non-zero if any test failed (so ctest/CI fails).
#include <juce_core/juce_core.h>

int main()
{
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);
    runner.runAllTests();

    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
        if (auto* r = runner.getResult (i))
            failures += r->failures;

    juce::Logger::writeToLog (failures == 0 ? "ALL TESTS PASSED"
                                            : "TESTS FAILED: " + juce::String (failures));
    return failures == 0 ? 0 : 1;
}
```

- [ ] **Step 2: Write `tests/RingBufferTests.cpp`**

```cpp
#include <juce_core/juce_core.h>
#include "Engine/RingBuffer.h"

using dcr::FloatRingBuffer;

struct RingBufferTests : juce::UnitTest
{
    RingBufferTests() : juce::UnitTest ("FloatRingBuffer") {}

    void runTest() override
    {
        beginTest ("capacity is requestedSize rounded to pow2 minus one");
        {
            FloatRingBuffer rb (100);          // rounds buffer to 128, usable = 127
            expectEquals ((int) rb.capacity(), 127);
        }

        beginTest ("write then read returns the same samples in order");
        {
            FloatRingBuffer rb (16);
            const float in[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
            expectEquals ((int) rb.write (in, 4), 4);
            expectEquals ((int) rb.readAvailable(), 4);

            float out[4] = { 0, 0, 0, 0 };
            expectEquals ((int) rb.read (out, 4), 4);
            for (int i = 0; i < 4; ++i)
                expectEquals (out[i], in[i]);
            expectEquals ((int) rb.readAvailable(), 0);
        }

        beginTest ("write is capped at writeAvailable (no overflow)");
        {
            FloatRingBuffer rb (4);            // usable capacity 7 (pow2 of 4+1 = 8)
            std::vector<float> big (100, 9.0f);
            const auto written = rb.write (big.data(), big.size());
            expect (written <= rb.capacity());
            expectEquals ((int) rb.writeAvailable() + (int) written, (int) rb.capacity());
        }

        beginTest ("read is capped at readAvailable (no underflow)");
        {
            FloatRingBuffer rb (16);
            const float in[2] = { 5.0f, 6.0f };
            rb.write (in, 2);
            float out[8] = {};
            expectEquals ((int) rb.read (out, 8), 2);   // only 2 available
        }

        beginTest ("wrap-around preserves order across the buffer seam");
        {
            FloatRingBuffer rb (8);            // usable 7
            float scratch[5] = {};
            for (int round = 0; round < 4; ++round)
            {
                float in[5];
                for (int i = 0; i < 5; ++i) in[i] = (float) (round * 5 + i);
                expectEquals ((int) rb.write (in, 5), 5);
                expectEquals ((int) rb.read (scratch, 5), 5);
                for (int i = 0; i < 5; ++i)
                    expectEquals (scratch[i], in[i]);
            }
        }

        beginTest ("clear resets to empty");
        {
            FloatRingBuffer rb (16);
            const float in[3] = { 1, 2, 3 };
            rb.write (in, 3);
            rb.clear();
            expectEquals ((int) rb.readAvailable(), 0);
        }
    }
};

static RingBufferTests ringBufferTests;
```

- [ ] **Step 3: Write `tests/CMakeLists.txt`**

The test target links the app's full JUCE module set + `AudioToolbox` because some Source headers (e.g. `Engine/EngineSettings.h`, transitively `Persistence/SnapshotStore.h`) include them. Tasks 5–7 append their `.cpp` and the Source files they exercise to `TEST_SOURCES`.

```cmake
juce_add_console_app(dcorerouter_tests PRODUCT_NAME "dcorerouter_tests")

juce_generate_juce_header(dcorerouter_tests)

set(TEST_SOURCES
    TestMain.cpp
    RingBufferTests.cpp
)

target_sources(dcorerouter_tests PRIVATE ${TEST_SOURCES})

# Source/ is on the include path so tests include "Engine/RingBuffer.h" etc.
target_include_directories(dcorerouter_tests PRIVATE ${CMAKE_SOURCE_DIR}/Source)

target_compile_definitions(dcorerouter_tests PRIVATE
    JUCE_PLUGINHOST_AU=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_UNIT_TESTS=1
)

target_link_libraries(dcorerouter_tests PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_opengl
    "-framework AudioToolbox"
    "-framework CoreAudio"
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_test(NAME dcorerouter_tests COMMAND dcorerouter_tests)
```

- [ ] **Step 4: Hook the test target into the root `CMakeLists.txt`**

Append to the very end of `CMakeLists.txt`:
```cmake

# ---- Tests --------------------------------------------------------------
enable_testing()
add_subdirectory(tests)
```

- [ ] **Step 5: Configure and build the test target**

Run:
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target dcorerouter_tests -j 8
```
Expected: configures (first run fetches JUCE 8.0.4 — slow), compiles, links `dcorerouter_tests`. If `cmake`/`ninja` are missing: `brew install cmake ninja`.

- [ ] **Step 6: Run the tests and verify they pass**

Run: `ctest --test-dir build --output-on-failure`
Expected: `100% tests passed, 0 tests failed out of 1`.

- [ ] **Step 7: Verify the harness has teeth (temporary mutation)**

Temporarily change one assertion in `RingBufferTests.cpp` (e.g. `expectEquals ((int) rb.capacity(), 127)` → `126`), rebuild, run `ctest`, and confirm it now FAILS. Then revert the change and confirm it passes again. This proves the runner actually surfaces failures.

- [ ] **Step 8: Commit**

```bash
git add tests/ CMakeLists.txt
git commit -m "test: add JUCE UnitTest harness + FloatRingBuffer tests"
```

---

### Task 5: RoutingMatrix tests

**Files:**
- Create: `tests/RoutingMatrixTests.cpp`
- Modify: `tests/CMakeLists.txt` (add the test file + `Source/Routing/RoutingMatrix.cpp` to `TEST_SOURCES`)

**Interfaces:**
- Consumes: `dcr::RoutingMatrix` from `Source/Routing/RoutingMatrix.h`: `resize(int,int)`, `setCrosspoint/getCrosspoint(out,in,gain)`, `setInputMute/getInputMute`, `setOutputMute/getOutputMute`, `setInputSolo/getInputSolo`, `setBlocked/isBlocked`, `getDirtyGeneration()`, `takeSnapshot(RoutingMatrix::Snapshot&)`. Note the snapshot type is the **nested** `RoutingMatrix::Snapshot` (distinct from `dcr::Snapshot` in SnapshotStore.h).
- Produces: nothing consumed downstream.

- [ ] **Step 1: Write `tests/RoutingMatrixTests.cpp`**

```cpp
#include <juce_core/juce_core.h>
#include "Routing/RoutingMatrix.h"

using dcr::RoutingMatrix;

struct RoutingMatrixTests : juce::UnitTest
{
    RoutingMatrixTests() : juce::UnitTest ("RoutingMatrix") {}

    void runTest() override
    {
        beginTest ("resize sets sizes and defaults (trims unity, crosspoints zero)");
        {
            RoutingMatrix m;
            m.resize (3, 2);
            expectEquals (m.getNumInputs(), 3);
            expectEquals (m.getNumOutputs(), 2);
            expectEquals (m.getInputTrim (0), 1.0f);
            expectEquals (m.getOutputTrim (1), 1.0f);
            expectEquals (m.getCrosspoint (0, 0), 0.0f);
        }

        beginTest ("every mutator bumps the dirty generation");
        {
            RoutingMatrix m;
            m.resize (2, 2);
            auto g0 = m.getDirtyGeneration();
            m.setCrosspoint (0, 1, 0.5f);
            auto g1 = m.getDirtyGeneration();
            expect (g1 > g0);
            m.setInputMute (0, true);
            expect (m.getDirtyGeneration() > g1);
        }

        beginTest ("snapshot reflects current values and computes anySoloActive");
        {
            RoutingMatrix m;
            m.resize (2, 1);
            m.setCrosspoint (0, 0, 0.25f);
            m.setInputSolo (1, true);

            RoutingMatrix::Snapshot s;
            m.takeSnapshot (s);
            expectEquals (s.numIns, 2);
            expectEquals (s.numOuts, 1);
            expectEquals (s.at (0, 0), 0.25f);
            expect (s.anySoloActive);
            expect (s.inputSolo[1] != 0);
        }

        beginTest ("snapshot anySoloActive is false when no solo set");
        {
            RoutingMatrix m;
            m.resize (2, 2);
            RoutingMatrix::Snapshot s;
            m.takeSnapshot (s);
            expect (! s.anySoloActive);
        }

        beginTest ("a blocked crosspoint is forced to silence and stays silent");
        {
            RoutingMatrix m;
            m.resize (1, 1);
            m.setCrosspoint (0, 0, 1.0f);
            expectEquals (m.getCrosspoint (0, 0), 1.0f);

            m.setBlocked (0, 0, true);                 // forces the cell to 0
            expect (m.isBlocked (0, 0));
            expectEquals (m.getCrosspoint (0, 0), 0.0f);

            m.setCrosspoint (0, 0, 0.9f);              // stray set must not re-enable
            expectEquals (m.getCrosspoint (0, 0), 0.0f);
        }

        beginTest ("out-of-range getters return 0 / false, never crash");
        {
            RoutingMatrix m;
            m.resize (1, 1);
            expectEquals (m.getCrosspoint (5, 5), 0.0f);
            expect (! m.getInputMute (9));
            expect (! m.isBlocked (9, 9));
        }
    }
};

static RoutingMatrixTests routingMatrixTests;
```

- [ ] **Step 2: Add the sources to `tests/CMakeLists.txt`**

Extend `TEST_SOURCES` so it reads:
```cmake
set(TEST_SOURCES
    TestMain.cpp
    RingBufferTests.cpp
    RoutingMatrixTests.cpp
    ${CMAKE_SOURCE_DIR}/Source/Routing/RoutingMatrix.cpp
)
```

- [ ] **Step 3: Build and run**

Run:
```bash
cmake --build build --target dcorerouter_tests -j 8
ctest --test-dir build --output-on-failure
```
Expected: all tests pass; the `RoutingMatrix` suite appears in the output.

- [ ] **Step 4: Commit**

```bash
git add tests/RoutingMatrixTests.cpp tests/CMakeLists.txt
git commit -m "test: add RoutingMatrix snapshot/dirty/blocked tests"
```

---

### Task 6: SnapshotStore round-trip test

**Files:**
- Create: `tests/SnapshotStoreTests.cpp`
- Modify: `tests/CMakeLists.txt` (add the test file + `Source/Persistence/SnapshotStore.cpp`)

**Interfaces:**
- Consumes: `dcr::Snapshot` and `dcr::SnapshotStore` from `Source/Persistence/SnapshotStore.h`: `static juce::ValueTree toValueTree(const Snapshot&)`, `static Snapshot fromValueTree(const juce::ValueTree&)`. `Snapshot` fields used: `engineSampleRate`, `engineBlockSize`, `inputTrim`, `outputTrim`, `inputMute`, `crosspoints` (vector of `{outputCh,inputCh,gain}`), `outputGroups` (vector of `Group{name,layoutName,memberChannels,faderDb,muted}`).
- Produces: nothing downstream. This is the regression guard Phase B's persistence work builds on.

- [ ] **Step 1: Write `tests/SnapshotStoreTests.cpp`**

```cpp
#include <juce_core/juce_core.h>
#include "Persistence/SnapshotStore.h"

using dcr::Snapshot;
using dcr::SnapshotStore;

struct SnapshotStoreTests : juce::UnitTest
{
    SnapshotStoreTests() : juce::UnitTest ("SnapshotStore") {}

    void runTest() override
    {
        beginTest ("toValueTree -> fromValueTree preserves scalar + vector state");
        {
            Snapshot s;
            s.engineSampleRate = 44100.0;
            s.engineBlockSize  = 256;
            s.inputTrim  = { 1.0f, 0.5f };
            s.outputTrim = { 0.25f };
            s.inputMute  = { 0, 1 };
            s.crosspoints.push_back ({ /*out*/ 0, /*in*/ 1, /*gain*/ 0.75f });

            auto tree = SnapshotStore::toValueTree (s);
            auto r    = SnapshotStore::fromValueTree (tree);

            expectEquals (r.engineSampleRate, 44100.0);
            expectEquals (r.engineBlockSize, 256);
            expectEquals ((int) r.inputTrim.size(), 2);
            expectEquals (r.inputTrim[1], 0.5f);
            expectEquals ((int) r.outputTrim.size(), 1);
            expectEquals (r.outputTrim[0], 0.25f);
            expectEquals ((int) r.inputMute.size(), 2);
            expect (r.inputMute[1] != 0);
            expectEquals ((int) r.crosspoints.size(), 1);
            expectEquals (r.crosspoints[0].outputCh, 0);
            expectEquals (r.crosspoints[0].inputCh, 1);
            expectEquals (r.crosspoints[0].gain, 0.75f);
        }

        beginTest ("group state round-trips (name, layout, members, fader, mute)");
        {
            Snapshot s;
            Snapshot::Group g;
            g.name = "Mains"; g.layoutName = "Stereo";
            g.memberChannels = { 0, 1 }; g.faderDb = -6.0f; g.muted = true;
            s.outputGroups.push_back (g);

            auto r = SnapshotStore::fromValueTree (SnapshotStore::toValueTree (s));
            expectEquals ((int) r.outputGroups.size(), 1);
            expectEquals (r.outputGroups[0].name, juce::String ("Mains"));
            expectEquals (r.outputGroups[0].layoutName, juce::String ("Stereo"));
            expectEquals ((int) r.outputGroups[0].memberChannels.size(), 2);
            expectEquals (r.outputGroups[0].faderDb, -6.0f);
            expect (r.outputGroups[0].muted);
        }

        beginTest ("empty snapshot round-trips without error");
        {
            Snapshot s;
            auto r = SnapshotStore::fromValueTree (SnapshotStore::toValueTree (s));
            expectEquals ((int) r.inputTrim.size(), 0);
            expectEquals ((int) r.crosspoints.size(), 0);
        }
    }
};

static SnapshotStoreTests snapshotStoreTests;
```

- [ ] **Step 2: Add the sources to `tests/CMakeLists.txt`**

`TEST_SOURCES` becomes:
```cmake
set(TEST_SOURCES
    TestMain.cpp
    RingBufferTests.cpp
    RoutingMatrixTests.cpp
    SnapshotStoreTests.cpp
    ${CMAKE_SOURCE_DIR}/Source/Routing/RoutingMatrix.cpp
    ${CMAKE_SOURCE_DIR}/Source/Persistence/SnapshotStore.cpp
)
```

- [ ] **Step 3: Build and run**

Run:
```bash
cmake --build build --target dcorerouter_tests -j 8
ctest --test-dir build --output-on-failure
```
Expected: all pass. If a link error names `AudioEngine` symbols, it means `SnapshotStore.cpp` references engine code beyond the inline `DeviceSpec` struct — stop and report; do not pull `AudioEngine.cpp` in without confirming (it would drag the whole engine + threads into the test binary).

- [ ] **Step 4: Commit**

```bash
git add tests/SnapshotStoreTests.cpp tests/CMakeLists.txt
git commit -m "test: add SnapshotStore ValueTree round-trip tests"
```

---

### Task 7: SettingsStore round-trip test

**Files:**
- Create: `tests/SettingsStoreTests.cpp`
- Modify: `tests/CMakeLists.txt` (add the test file + `Source/Persistence/SettingsStore.cpp`)

**Interfaces:**
- Consumes: `dcr::EngineSettings` (`Source/Engine/EngineSettings.h`) and `dcr::SettingsStore` (`Source/Persistence/SettingsStore.h`): `static bool save(const EngineSettings&)`, `static EngineSettings load()`, `static juce::File getFile()`. `save()`/`load()` use a fixed on-disk path; the test brackets it by snapshotting and restoring whatever file exists so it never clobbers a real user file.
- Produces: nothing downstream. Regression guard for Phase B's "persist the dropped threshold fields" + version work.

- [ ] **Step 1: Write `tests/SettingsStoreTests.cpp`**

This test documents CURRENT behavior. The `stalledWarnRatio` round-trip is expected to FAIL today (the field is not persisted) — Phase B fixes it. So this version asserts only the fields that ARE persisted, plus an explicit `logMessage` noting the known gap, to keep Phase A green. Phase B will flip the threshold assertion on.

```cpp
#include <juce_core/juce_core.h>
#include "Persistence/SettingsStore.h"

using dcr::EngineSettings;
using dcr::SettingsStore;

struct SettingsStoreTests : juce::UnitTest
{
    SettingsStoreTests() : juce::UnitTest ("SettingsStore") {}

    void runTest() override
    {
        // Protect any real user settings file: back it up, restore in scope exit.
        const juce::File file = SettingsStore::getFile();
        const bool       had  = file.existsAsFile();
        const juce::String backup = had ? file.loadFileAsString() : juce::String();

        beginTest ("persisted audio-path fields survive save -> load");
        {
            EngineSettings s;
            s.engineSampleRate    = 96000.0;
            s.engineBlockSize     = 512;
            s.outputPreFillBlocks = 12;
            s.gainSmoothingMs     = 40;

            expect (SettingsStore::save (s));
            EngineSettings r = SettingsStore::load();

            expectEquals (r.engineSampleRate, 96000.0);
            expectEquals (r.engineBlockSize, 512);
            expectEquals (r.outputPreFillBlocks, 12);
            expectEquals (r.gainSmoothingMs, 40);
        }

        // KNOWN GAP (fixed in Phase B): warn/crit threshold fields are not yet
        // persisted. Documented here, asserted in Phase B.
        logMessage ("NOTE: stalledWarnRatio/cpuWarnRatio persistence is a known "
                    "Phase B gap; not asserted in Phase A.");

        // Restore the user's original file (or remove the one we created).
        if (had) file.replaceWithText (backup);
        else     file.deleteFile();
    }
};

static SettingsStoreTests settingsStoreTests;
```

- [ ] **Step 2: Add the sources to `tests/CMakeLists.txt`**

`TEST_SOURCES` becomes:
```cmake
set(TEST_SOURCES
    TestMain.cpp
    RingBufferTests.cpp
    RoutingMatrixTests.cpp
    SnapshotStoreTests.cpp
    SettingsStoreTests.cpp
    ${CMAKE_SOURCE_DIR}/Source/Routing/RoutingMatrix.cpp
    ${CMAKE_SOURCE_DIR}/Source/Persistence/SnapshotStore.cpp
    ${CMAKE_SOURCE_DIR}/Source/Persistence/SettingsStore.cpp
)
```

- [ ] **Step 3: Build and run**

Run:
```bash
cmake --build build --target dcorerouter_tests -j 8
ctest --test-dir build --output-on-failure
```
Expected: all pass; `SettingsStore` suite runs and the NOTE line appears.

- [ ] **Step 4: Commit**

```bash
git add tests/SettingsStoreTests.cpp tests/CMakeLists.txt
git commit -m "test: add SettingsStore save/load round-trip (documents Phase B gap)"
```

---

### Task 8: `CLAUDE.md` project memory

**Files:**
- Create: `CLAUDE.md`

**Interfaces:**
- Consumes: the build/test commands established in Tasks 4–7.
- Produces: project memory loaded each session.

- [ ] **Step 1: Create `CLAUDE.md`**

```markdown
# CLAUDE.md — D-Router

Real-time NxM CoreAudio routing matrix for macOS. C++20, JUCE 8 (FetchContent).

## Build / run / test
- Build + launch:  `./run.sh`  (Release)   ·  `./run.sh --debug`  ·  `./run.sh --clean`
- Configure:       `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug`
- Build app:       `cmake --build build -j`
- Build tests:     `cmake --build build --target dcorerouter_tests -j`
- Run tests:       `ctest --test-dir build --output-on-failure`
- App bundle:      `build/dcorerouter_artefacts/<cfg>/D-Router.app` (process name `D-Router`)

## Conventions (do not violate)
- Namespace `dcr::` for all project code.
- **AU plugins are created/destroyed on the message thread ONLY.** The async
  plugin-load queue in MainComponent exists for this; never instantiate/destroy
  an AU from a worker or audio thread.
- **Never clobber per-channel output trims** to fade for a restart — use
  `MatrixProcessor::setMasterGainTarget()`. (A prior bug leaked a stuck -60 dB.)
- `MatrixProcessor`: member `pool` MUST be declared before `thread` (reverse
  destruction order keeps the worker pool alive while the matrix thread unwinds).
- Ring buffers (`FloatRingBuffer`) are strict SPSC: one producer (device IO
  callback), one consumer (matrix thread). Never share an end across threads.
- `RoutingMatrix` is the authoritative routing state (atomic arrays + `dirtyGen`).
  The audio side rebuilds its sparse route cache only when `dirtyGen` changes.
- `pendingSnap` (MainComponent) is single-threaded by discipline — drained on the
  message thread. Keep it that way.

## Scope rules
- **DSP is out of scope** for current refactors (`Source/DSP/**`). Don't touch it.
- Match existing JUCE house style (see `.clang-format`); don't reformat files.
- Code changes go on a branch; small commits; test-first for edge-case logic.

## Layout
- `Source/Engine/`      device workers, per-channel SRC, `MatrixProcessor`, rings.
- `Source/Routing/`     `RoutingMatrix`, output/input group managers.
- `Source/DSP/`         AU + built-in plugin hosts and the built-in suite (OUT OF SCOPE).
- `Source/UI/`          `MatrixView`, `CrosspointGrid`, panels, dialogs, look-and-feel.
- `Source/Persistence/` ValueTree/XML snapshots, settings, crash guard.
- `Source/Diagnostics/` logger, perf monitor, crash handler.
- `tests/`              JUCE UnitTest target `dcorerouter_tests`.
- `docs/superpowers/`   design specs + implementation plans.
```

- [ ] **Step 2: Commit**

```bash
git add CLAUDE.md
git commit -m "docs: add CLAUDE.md project memory (conventions, build/test, scope)"
```

---

### Task 9: GitHub Actions CI

**Files:**
- Create: `.github/workflows/ci.yml`

**Interfaces:**
- Consumes: the build (`cmake -B build ...`), test (`ctest ...`), and `.clang-format` from earlier tasks.
- Produces: a green CI pipeline on push/PR.

- [ ] **Step 1: Create `.github/workflows/ci.yml`**

The format job checks only files changed vs the base ref — best practice and it avoids any legacy-diff noise. clang-tidy is a separate non-blocking job.

```yaml
name: CI
on:
  push:
    branches: [main]
  pull_request:

jobs:
  build-and-test:
    runs-on: macos-14
    steps:
      - uses: actions/checkout@v4
      - name: Install build tools
        run: brew install cmake ninja
      - name: Cache JUCE FetchContent
        uses: actions/cache@v4
        with:
          path: build/_deps
          key: juce-8.0.4-${{ runner.os }}
      - name: Configure
        run: cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
      - name: Build app
        run: cmake --build build -j
      - name: Build tests
        run: cmake --build build --target dcorerouter_tests -j
      - name: Run tests
        run: ctest --test-dir build --output-on-failure

  format-check:
    runs-on: macos-14
    steps:
      - uses: actions/checkout@v4
        with:
          fetch-depth: 0
      - name: Install clang-format
        run: brew install clang-format
      - name: clang-format changed files
        run: |
          base="${{ github.event.pull_request.base.sha }}"
          if [ -z "$base" ]; then base="$(git rev-parse HEAD~1)"; fi
          changed=$(git diff --name-only "$base" HEAD -- '*.cpp' '*.h' '*.mm' \
                    | grep -v '^Source/DSP/' || true)
          if [ -z "$changed" ]; then echo "no non-DSP C++ files changed"; exit 0; fi
          echo "$changed"
          clang-format --dry-run --Werror $changed

  lint:
    runs-on: macos-14
    continue-on-error: true   # non-blocking in Phase A
    steps:
      - uses: actions/checkout@v4
      - name: Install tools
        run: brew install cmake ninja llvm
      - name: Configure (compile commands)
        run: cmake -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug
      - name: clang-tidy non-DSP sources
        run: |
          files=$(git ls-files 'Source/**/*.cpp' | grep -v '^Source/DSP/' || true)
          [ -z "$files" ] && exit 0
          $(brew --prefix llvm)/bin/clang-tidy -p build $files || true
```

- [ ] **Step 2: Validate the workflow YAML locally**

Run: `python3 -c "import yaml,sys; yaml.safe_load(open('.github/workflows/ci.yml')); print('YAML OK')"`
Expected: `YAML OK`.

- [ ] **Step 3: Commit**

```bash
git add .github/workflows/ci.yml
git commit -m "ci: add macOS build+test, format-check, non-blocking clang-tidy"
```

- [ ] **Step 4: (After push) confirm CI is green**

Once the branch is pushed and a PR opened, confirm the `build-and-test` and `format-check` jobs pass and `lint` runs (may report findings; non-blocking). Pushing is a separate, user-authorized step — do not push without asking.

---

## Self-Review

**Spec coverage:** A1 CLAUDE.md → Task 8. A2 .clang-format → Task 2. A3 .clang-tidy → Task 3. A4 test target + ring/matrix/persistence tests → Tasks 4–7. A5 CI build+test+lint, no artifact → Task 9. A6 run.sh + .editorconfig → Task 1. The spec's group-manager tests were intentionally deferred to Phase C (where that code is modified); noted in the design and here. All Phase A spec items covered.

**Placeholder scan:** No TBD/TODO; every code and config step shows full content; every command lists expected output. The one "documents current behavior" test (Task 7) is explicit about what it asserts vs. the known Phase B gap.

**Type consistency:** Test code uses the real APIs verified from headers — `FloatRingBuffer` (RingBuffer.h), `RoutingMatrix` + nested `RoutingMatrix::Snapshot` with `.at(out,in)` and `.anySoloActive` (RoutingMatrix.h/.cpp), `dcr::Snapshot`/`SnapshotStore::to/fromValueTree` and `Snapshot::Group`/`Crosspoint` fields (SnapshotStore.h), `EngineSettings` fields + `SettingsStore::save/load/getFile` (EngineSettings.h/SettingsStore.h). `TEST_SOURCES` grows monotonically and consistently across Tasks 4–7. Target name `dcorerouter_tests` and bundle name `D-Router` used consistently.
