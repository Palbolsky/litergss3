#ifndef LiteRGSS_VfsLock_H
#define LiteRGSS_VfsLock_H

// `LiteRGSS.vfs_lock=(monitor)` / `LiteRGSS.vfs_lock`
//
// Inject a Monitor-like Ruby object that LiteCGSS will use to serialise every
// internal PHYSFS_* call. Intended use: pass `PhysFS.monitor` from the physfs
// gem so LiteCGSS's direct PhysFS calls share the same GVL-aware lock as the
// physfs gem's own entry points. Without this injection, a thread inside
// LiteCGSS can race against a thread inside physfs_gem on PhysFS's pthread
// stateLock — see physfs/ext/physfs/PhysFSLock.h for the full deadlock
// shape.
//
// The setter validates that the object responds to `#enter` and `#exit` so
// misuse fails loudly at install time rather than at the first PhysFS read.
//
// On builds without LITECGSS_USE_PHYSFS, both methods are simply not defined
// (LiteCGSS has no PhysFS code to lock, and the physfs gem is not present).

void Init_VfsLock();

#endif
