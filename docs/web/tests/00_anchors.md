# Anchor & GFM Test

Dieses Dokument prüft automatische Sprungmarken, Cross-Links, Emojis und
Whitelist-Tags.

## Introduction

Einleitungsabschnitt.

Springe zur [Setup](#setup)-Sektion.

Springe zum [zweiten Intro](#introduction) — sollte auf die **erste**
`Introduction` zeigen (GitHub-Verhalten).

## Setup

Zurück zur [Introduction](#introduction).

## Introduction

Zweites Intro — sollte `id="…-introduction-1"` bekommen.

## Sonderzeichen: What's New?

Der Slug sollte `sonderzeichen-whats-new` sein.

## Über uns

UTF-8-Test. Slug sollte `über-uns` sein.

## Inline `code` Heading

Slug sollte `inline-code-heading` sein.

##

Leerer Heading — wird ignoriert.

# Cross-Links

[Cross-Doc ohne Anker](docs/project/SU2.md)

[Cross-Doc mit Anker](docs/project/SU2.md#overview)

[Absoluter Link](https://github.com/onkel83/prophysics)

[Ungültiger Anker](#gibt-es-nicht) — bleibt unverändert.

# Entities & Escapes

Text mit Entity: &copy; 2026 · &amp; · &#10003; · &#x2713;

Escaped: \*kein Kursiv\* und \_kein Unterstrich\_ und \`kein Code\`.

# Autolinks

Inline-Autolink: <https://example.com/path?a=1&b=2>

Mail: <mailto:koehne83@googlemail.com>

# Emojis

Statustext: :check: erfolgreich, :warning: Achtung, :cross: fehlgeschlagen.

Physik: :atom: :zap: :infinity:

# Formatierung

**Fett**, *kursiv*, ~~durchgestrichen~~, `inline code`.

[Reference-Link][ref1] und [ref1]-Kurzform.

[ref1]: https://example.com "Titel des Referenz-Links"

# Whitelist-Tags

Roh erlaubt: H<sub>2</sub>O · E = mc<sup>2</sup> · <kbd>Ctrl</kbd>+<kbd>C</kbd>
· <mark>hervorgehoben</mark>

Roh verboten (wird escaped): <script>alert(1)</script> und
<iframe src="evil.html"></iframe>

# Tabelle

| Modul | Status | Prio |
| :---- | :----: | ---: |
| SU2 | Aktiv | 1 |
| Dirac | Stabil | 2 |
| Tensor | Beta | 3 |

# Task-Liste

- [x] Manifest
- [x] md_parser Anker
- [ ] Crosslink-Config testen
- [ ] Emoji-Config finalisieren

# Details-Block

<details>
<summary>Mehr Infos</summary>

Der Inhalt im Details-Block.

</details>

# Setext Test
==============

Level-1 via Setext.

Sub-Überschrift via Setext
---------------------------

Level-2 via Setext.