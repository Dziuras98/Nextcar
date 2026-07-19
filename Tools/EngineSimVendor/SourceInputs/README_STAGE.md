# NC-003B temporary source-input staging

This directory is a temporary, user-assisted source staging area.

It is not part of the final NC-003B Phase 0 vendor tree.

Source branch base:

```text
c1e2d0dea471dda257f598290004c2599136c9f7
```

Pinned source inputs:

```text
Dziuras98/engine-sim
85f7c3b959a908ed5232ede4f1a4ac7eafe6b630

ange-yaghi/simple-2d-constraint-solver
e009f4ff1c9c4c5874e865e893cdb62e208fb2b3
```

Archive SHA-256:

```text
c98f9354358da2ce7c83de24f023f865b5243f6ac02f4ea1d1acc0b549913f7c
```

The exact impulse-response WAV was verified as Git blob:

```text
6d3f8688e170cb6e5f4bfec42f580f3900514d72
```

Agents must use these local files instead of accessing either source
repository.

Before final NC-003B Phase 0 publication, the complete directory:

```text
Tools/EngineSimVendor/SourceInputs
```

must be removed from the final candidate tree.

The final candidate must be squash-merged or otherwise published without
retaining this staging directory in the resulting tree.
