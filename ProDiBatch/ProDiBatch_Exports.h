/* ==========================================================================
 * ProDiBatch - DLL / Shared Object Export Gates
 * File: ProDiBatch_Exports.h
 * Architecture: Cross-Platform Compiler Gating
 * ========================================================================== */

#ifndef PRODIBATCH_EXPORTS_H
#define PRODIBATCH_EXPORTS_H

#ifdef _WIN32
#ifdef PRODIBATCH_DLL_EXPORTS
#define PRODIBATCH_API __declspec(dllexport)
#elif defined(PRODIBATCH_DLL_IMPORTS)
#define PRODIBATCH_API __declspec(dllimport)
#else
#define PRODIBATCH_API // Leer für statisches Linken im Monolithen
#endif
#else
#if __GNUC__ >= 4
#define PRODIBATCH_API __attribute__((visibility("default")))
#else
#define PRODIBATCH_API
#endif
#endif

#endif // PRODIBATCH_EXPORTS_H