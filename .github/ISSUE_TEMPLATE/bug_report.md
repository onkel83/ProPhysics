---
name: Bug Report
about: Einen Fehler im Kernel oder in der Build-Infrastruktur melden
title: "[Bug] "
labels: bug
assignees: ''
---

## Was passiert

<!-- Kurze Beschreibung des Fehlers. -->

## Was erwartet wird

<!-- Was sollte stattdessen passieren. -->

## Reproduktion

1. `git clone https://github.com/onkel83/prophysics.git`
2. `cd prophysics\build\main`
3. `build.cmd -Mode all -Rebuild`
4. `cd ..\..\tools`
5. `run_alpha_tests.cmd -Prio 8` <!-- oder der betroffene Test -->
6. …

## Umgebung

- Windows-Version: <!-- z.B. Windows 11 23H2 -->
- Visual Studio: <!-- z.B. VS 2022 Community 17.10 -->
- Kernel-Version: <!-- aus ProPhysics_Version.h -->
- Commit: <!-- git rev-parse --short HEAD -->

## Log

<!-- Inhalt aus bin\logs\<timestamp>_<test>.log oder Konsolen-Ausgabe. -->

## R-Konformität

<!-- Falls bekannt: Welche Regel (R1–R7) ist betroffen? -->