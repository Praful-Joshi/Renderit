# Auto-orientation on import is metadata-driven only, not heuristic

Users land on this project frustrated that models import at the wrong scale/rotation and need manual correction. Auto-*scaling* is fully solvable (bounding-box normalization always works). Auto-*orientation* is not: it depends on whether the file format encodes an up-axis. glTF (always Y-up by spec), FBX, and Collada (explicit `<up_axis>`) can self-report. OBJ and STL have no such convention — there is no metadata to read, so any "correction" for them is a guess (e.g. PCA on the mesh to align its dominant axis to Y-up).

**Decision:** auto-orient using each format's own metadata where it exists; default to Y-up (no correction attempted) where it doesn't. The settings panel's manual rotate sliders are the correction path for formats that can't self-report, rather than building a heuristic that will sometimes be confidently wrong on a real model with no way to detect that it's wrong.

**Consequence:** OBJ/STL imports may still need manual rotation on first view — this is expected, not a bug, and matches the reality that Sketchfab's own pipeline relies on curated/manual setup for these cases too, not a general algorithm.
