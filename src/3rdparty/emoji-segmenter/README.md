Emoji Segmenter
===

This repository contains a Ragel grammar and generated C code for segmenting
runs of text into text-presentation and emoji-presentation runs. It is currently
used in projects such as Chromium and Pango for deciding which preferred
presentation, color or text, a run of text should have.

The goal is to stay very close to the grammar definitions in [Unicode Technical
Standard #51](http://www.unicode.org/reports/tr51/).

API
===

By including the `emoji_presentation_scanner.c` file, you will be able to call
the following API

```
scan_emoji_presentation (emoji_text_iter_t p,
    const emoji_text_iter_t pe,
    bool* is_emoji,
    bool* has_vs)
```

This API call will scan `emoji_text_iter_t p` for the next grammar-token and
return an iterator that points to the end of the next token. An end iterator
needs be specified as `pe` so that the scanner can compare against this and
knows where to stop. In the reference parameter `is_emoji` it returns whether
this token has emoji-presentation text-presentation, `has_vs` is set to true
if the token contains a variation selector.

A grammar token is either a combination of an emoji plus variation selector 15
for text presentation, an emoji presentation sequence (emoji + VS16), an emoji
presentation emoji or emoji sequence, or a single text presentation character.

`emoji_text_iter_t` is an iterator type over a buffer of the character classes
that are defined at the beginning of the the Ragel file, e.g. `EMOJI`,
`EMOJI_TEXT_PRESENTATION`, `REGIONAL_INDICATOR`, `KEYCAP_BASE`, etc.

By typedef'ing `emoji_text_iter_t` to your own iterator type, you can implement
an adapter class that iterates over an input text buffer in any encoding, and on
dereferencing returns the correct Ragel class by implementing something similar
to the following Unicode character class to Ragel class mapping, example taken
from Chromium:

```
char EmojiSegmentationCategory(UChar32 codepoint) {
  // Specific ones first.
  if (codepoint == kCombiningEnclosingKeycapCharacter)
    return COMBINING_ENCLOSING_KEYCAP;
  if (codepoint == kCombiningEnclosingCircleBackslashCharacter)
    return COMBINING_ENCLOSING_CIRCLE_BACKSLASH;
  if (codepoint == kZeroWidthJoinerCharacter)
    return ZWJ;
  if (codepoint == kVariationSelector15Character)
    return VS15;
  if (codepoint == kVariationSelector16Character)
    return VS16;
  if (codepoint == 0x1F3F4)
    return TAG_BASE;
  if ((codepoint >= 0xE0030 && codepoint <= 0xE0039) ||
      (codepoint >= 0xE0061 && codepoint <= 0xE007A))
    return TAG_SEQUENCE;
  if (codepoint == 0xE007F)
    return TAG_TERM;
  if (Character::IsEmojiModifierBase(codepoint))
    return EMOJI_MODIFIER_BASE;
  if (Character::IsModifier(codepoint))
    return EMOJI_MODIFIER;
  if (Character::IsRegionalIndicator(codepoint))
    return REGIONAL_INDICATOR;
  if (Character::IsEmojiKeycapBase(codepoint))
    return KEYCAP_BASE;

  if (Character::IsEmojiEmojiDefault(codepoint))
    return EMOJI_EMOJI_PRESENTATION;
  if (Character::IsEmojiTextDefault(codepoint))
    return EMOJI_TEXT_PRESENTATION;
  if (Character::IsEmoji(codepoint))
    return EMOJI;

  // Ragel state machine will interpret unknown category as "any".
  return kMaxEmojiScannerCategory;
}
```

Update/Build requisites
===

You need to have ragel installed if you want to modify the grammar and generate a new C file as output.

`apt-get install ragel`

then run

`make`

to update the `emoji_presentation_scanner.c` and `emoji_presentation_scanner_vs.c` output C source file.

Contributing
===

See the CONTRIBUTING.md file for how to contribute.
