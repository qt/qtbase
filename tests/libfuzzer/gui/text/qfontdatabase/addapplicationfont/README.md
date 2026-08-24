# qfontdatabase/addapplicationfont fuzz test

This fuzz target loads a font into `QFontDatabase` via
`QFontDatabase::addApplicationFontFromData()` (passing the raw fuzzer input as a
`QByteArray`), renders a string with it into a throwaway `QImage`, and then
removes the application font again before the next iteration.

## Custom mutator (BrokenType)

Fonts are highly structured binary files. If libFuzzer mutates them with its
generic byte-level mutations, the overwhelming majority of inputs are rejected
by the font loader before reaching any interesting code. To avoid this, the test
plugs the [BrokenType](https://github.com/googleprojectzero/BrokenType)
`ttf-otf-mutator` in as a libFuzzer *custom mutator*
(`LLVMFuzzerCustomMutator`).

If an input is not recognizable as an SFNT font (for example the empty or random
initial inputs), the custom mutator falls back to libFuzzer's built-in mutator
(`LLVMFuzzerMutate`) so fuzzing can still make progress.

The BrokenType parsing and mutation happens entirely in memory — no temporary
files are written.

## Building

```
git clone https://github.com/googleprojectzero/BrokenType
export BROKENTYPE_PATH=/path/to/BrokenType
```

`BROKENTYPE_PATH` must point at the root of the BrokenType checkout; the tool's
sources are expected under `$BROKENTYPE_PATH/ttf-otf-mutator`.

Then build the test as described in `tests/libfuzzer/README`.

## Running

Because the custom mutator can only apply BrokenType mutations to inputs it
recognizes as fonts, you should seed the corpus with a few real `.ttf`/`.otf`
files:

```
mkdir corpus
cp /usr/share/fonts/truetype/dejavu/*.ttf corpus/
./addapplicationfont corpus/
```

libFuzzer will take those seeds and mutate them with BrokenType from there.
