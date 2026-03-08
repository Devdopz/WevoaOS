# ADR-0001: BIOS + Custom Stage1/Stage2 Boot Path

## Status
Accepted

## Decision
Wevoa boots through:
1. Stage1 MBR boot sector at LBA0.
2. Stage2 loader at LBA1..N.
3. Kernel binary loaded by stage2 and entered in x86_64 long mode.

## Rationale
- Keeps ownership of the entire boot chain.
- Avoids dependency on external bootloaders.
- Aligns with original "from scratch" requirement.

