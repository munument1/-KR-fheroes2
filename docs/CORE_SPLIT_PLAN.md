# Core split plan

The next cleanup checkpoint is to maintain two reviewable layers:

1. A generic UTF-8 / large-alphabet engine layer with no Korean locale or translation data.
2. A Korean demonstration layer that registers Korean, builds `ko.mo`, and generates the Hangul repertoire.

The working Korean prototype remains the integration test. No upstream pull request is created by this branch.
